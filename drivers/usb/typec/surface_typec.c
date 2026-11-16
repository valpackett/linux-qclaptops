// SPDX-License-Identifier: GPL-2.0+
/*
 * USB Type-C port driver for Microsoft Surface devices where the
 * Surface System Aggregator Module (SSAM) is fully managing the ports.
 *
 * Copyright (C) 2026, Val Packett <val@packett.cool>
 */

#include <linux/fwnode.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <drm/bridge/aux-bridge.h>

#include <linux/surface_aggregator/device.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec_altmode.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/usb/typec_retimer.h>

#define STCM_NUM_PORTS			2

#define STCM_EVENT_CID			0x02

enum stcm_cable_orientation {
	STCM_ORIENTATION_NORMAL		= 0,
	STCM_ORIENTATION_REVERSE	= 1,
	STCM_ORIENTATION_DISCONNECTED	= 2,
};

enum stcm_alt_mode {
	STCM_ALT_MODE_NONE		= 0,
	STCM_ALT_MODE_USB		= 1,
	STCM_ALT_MODE_DP		= 2,
	STCM_ALT_MODE_USB_DP		= 3,
};

struct stcm_event {
	__le32 sequence_number;
	u8 port_id;
	u8 cable_orientation;
	u8 alt_mode;
	u8 unknown1;
	u8 pin_assignment;
	u8 dp_hpd;
	u8 unknown2;
} __packed;

struct stcm_ack_cmd {
	__le32 sequence_number;
	__le32 port_id;
	__le32 flag;
} __packed;

SSAM_DEFINE_SYNC_REQUEST_W(__ssam_typec_ack, struct stcm_ack_cmd, {
	.target_category = SSAM_SSH_TC_USC,
	.target_id       = SSAM_SSH_TID_SAM,
	.instance_id     = 0x01,
	.command_id      = 0x03,
});

struct surface_typec_port {
	struct device		*dev;
	struct typec_capability	cap;
	struct typec_port	*port;
	struct typec_partner	*partner;
	struct typec_altmode	*dp_alt;
	struct device		*dp_bridge;
	struct typec_mux	*mux;
	struct typec_retimer	*retimer;
	u8			alt_mode;
};

struct surface_typec {
	struct ssam_device *sdev;
	struct ssam_event_notifier notif;

	struct surface_typec_port ports[STCM_NUM_PORTS];
};

static enum typec_orientation
surface_typec_orientation(struct device *dev, u8 orientation)
{
	switch (orientation) {
	case STCM_ORIENTATION_NORMAL:
		return TYPEC_ORIENTATION_NORMAL;
	case STCM_ORIENTATION_REVERSE:
		return TYPEC_ORIENTATION_REVERSE;
	case STCM_ORIENTATION_DISCONNECTED:
		return TYPEC_ORIENTATION_NONE;
	default:
		dev_warn(dev, "unrecognized cable orientation %u\n", orientation);
		return TYPEC_ORIENTATION_NONE;
	}
}

static void surface_typec_set_state(struct surface_typec_port *port,
				    struct typec_altmode *alt,
				    unsigned long mode, void *data)
{
	struct typec_mux_state mux_state = {
		.alt = alt,
		.mode = mode,
		.data = data,
	};
	struct typec_retimer_state retimer_state = {
		.alt = alt,
		.mode = mode,
		.data = data,
	};
	int ret;

	if (port->mux) {
		ret = typec_mux_set(port->mux, &mux_state);
		if (ret)
			dev_err(port->dev, "failed to set mux state %lu: %d\n",
				mode, ret);
	}

	if (port->retimer) {
		ret = typec_retimer_set(port->retimer, &retimer_state);
		if (ret)
			dev_err(port->dev, "failed to set retimer state %lu: %d\n",
				mode, ret);
	}
}

static void surface_typec_unregister_partner(struct surface_typec_port *port)
{
	if (!port->partner)
		return;

	typec_unregister_partner(port->partner);
	port->partner = NULL;
}

static int surface_typec_register_partner(struct surface_typec_port *port)
{
	struct typec_partner_desc desc = {};

	if (port->partner)
		return 0;

	port->partner = typec_register_partner(port->port, &desc);
	if (IS_ERR(port->partner)) {
		port->partner = NULL;
		return dev_err_probe(port->dev, PTR_ERR(port->partner),
				    "failed to register partner\n");
	}

	return 0;
}

static void surface_typec_update_hpd(struct surface_typec_port *port, bool hpd)
{
	if (!port->dp_bridge)
		return;

	drm_aux_hpd_bridge_notify(port->dp_bridge,
				  hpd ? connector_status_connected :
					connector_status_disconnected);
}

static void surface_typec_update_dp(struct surface_typec_port *port,
				    const struct stcm_event *evt)
{
	struct typec_displayport_data dp_data = {
		.status = DP_STATUS_ENABLED,
	};
	unsigned long mode;
	u8 pin_assignment;

	if (evt->alt_mode == STCM_ALT_MODE_DP) {
		mode = TYPEC_DP_STATE_C;
		pin_assignment = DP_PIN_ASSIGN_C;
	} else {
		mode = TYPEC_DP_STATE_D;
		pin_assignment = DP_PIN_ASSIGN_D;
	}

	if (evt->dp_hpd)
		dp_data.status |= DP_STATUS_HPD_STATE;

	dp_data.conf = DP_CONF_SET_PIN_ASSIGN(BIT(pin_assignment));

	typec_altmode_update_active(port->dp_alt, true);
	surface_typec_set_state(port, port->dp_alt, mode, &dp_data);
	surface_typec_update_hpd(port, evt->dp_hpd);
}

static u32 surface_typec_notify(struct ssam_event_notifier *nf,
				const struct ssam_event *event)
{
	struct surface_typec *stcm = container_of(nf, struct surface_typec, notif);
	struct surface_typec_port *port;
	const struct stcm_event *evt;
	struct stcm_ack_cmd ack;
	enum typec_orientation orientation;
	int ret;

	if (event->command_id != STCM_EVENT_CID)
		return 0;

	if (event->length != sizeof(*evt)) {
		dev_warn(&stcm->sdev->dev, "unexpected event length %u\n",
			 event->length);
		return SSAM_NOTIF_HANDLED;
	}

	evt = (const struct stcm_event *)event->data;
	if (evt->port_id >= ARRAY_SIZE(stcm->ports)) {
		dev_err(&stcm->sdev->dev, "port %u out of range\n", evt->port_id);
		return SSAM_NOTIF_HANDLED;
	}

	port = &stcm->ports[evt->port_id];
	if (!port->port) {
		dev_warn(port->dev, "event for unregistered port %u\n",
			 evt->port_id);
		return SSAM_NOTIF_HANDLED;
	}

	orientation = surface_typec_orientation(port->dev,
						evt->cable_orientation);

	if (orientation == TYPEC_ORIENTATION_NONE ||
	    evt->alt_mode == STCM_ALT_MODE_NONE) {
		surface_typec_unregister_partner(port);

		if (port->dp_alt)
			typec_altmode_update_active(port->dp_alt, false);

		surface_typec_set_state(port, NULL, TYPEC_STATE_SAFE, NULL);
		surface_typec_update_hpd(port, false);
		typec_set_orientation(port->port, orientation);
	} else {
		typec_set_orientation(port->port, orientation);

		ret = surface_typec_register_partner(port);
		if (ret)
			goto ack;

		switch (evt->alt_mode) {
		case STCM_ALT_MODE_DP:
		case STCM_ALT_MODE_USB_DP:
			surface_typec_update_dp(port, evt);
			break;
		case STCM_ALT_MODE_USB:
			if (port->dp_alt)
				typec_altmode_update_active(port->dp_alt, false);

			surface_typec_set_state(port, NULL, TYPEC_STATE_USB,
						NULL);
			surface_typec_update_hpd(port, false);
			break;
		default:
			dev_warn(port->dev, "unrecognized alt mode %u\n",
				 evt->alt_mode);
			surface_typec_set_state(port, NULL, TYPEC_STATE_SAFE,
						NULL);
			break;
		}
	}

ack:
	ack.sequence_number = evt->sequence_number;
	ack.port_id = cpu_to_le32(evt->port_id);
	ack.flag = 0;

	ret = __ssam_typec_ack(stcm->sdev->ctrl, &ack);
	if (ret)
		dev_err(&stcm->sdev->dev, "failed to ACK USB-C event: %d\n", ret);

	return SSAM_NOTIF_HANDLED;
}

static void surface_typec_port_release(void *data)
{
	struct surface_typec_port *port = data;

	surface_typec_unregister_partner(port);

	if (port->dp_alt) {
		typec_unregister_altmode(port->dp_alt);
		port->dp_alt = NULL;
	}

	if (port->port) {
		typec_unregister_port(port->port);
		port->port = NULL;
	}

	typec_mux_put(port->mux);
	port->mux = NULL;
	typec_retimer_put(port->retimer);
	port->retimer = NULL;
}

static int surface_typec_register_port(struct surface_typec *stcm,
				       unsigned int index,
				       struct fwnode_handle *connector)
{
	struct surface_typec_port *port = &stcm->ports[index];
	struct device *dev = &stcm->sdev->dev;
	struct typec_altmode_desc desc = {
		.svid = USB_TYPEC_DP_SID,
		.mode = USB_TYPEC_DP_MODE,
		.vdo = DP_CAP_DFP_D | DP_CAP_RECEPTACLE |
		       DP_CONF_SET_PIN_ASSIGN(BIT(DP_PIN_ASSIGN_C) |
					      BIT(DP_PIN_ASSIGN_D) |
					      BIT(DP_PIN_ASSIGN_E)),
	};
	int ret;

	port->dev = dev;

	if (connector) {
		ret = typec_get_fw_cap(&port->cap, connector);
		if (ret)
			return dev_err_probe(dev, ret,
					     "failed to read connector %u capabilities\n",
					     index);

		port->dp_bridge = drm_dp_hpd_bridge_register(dev,
							     to_of_node(connector));
		if (IS_ERR(port->dp_bridge))
			return dev_err_probe(dev, PTR_ERR(port->dp_bridge),
					     "failed to register DP HPD bridge for port %u\n",
					     index);

		port->mux = fwnode_typec_mux_get(connector);
		if (IS_ERR(port->mux))
			return dev_err_probe(dev, PTR_ERR(port->mux),
					     "failed to get mux for port %u\n",
					     index);

		port->retimer = fwnode_typec_retimer_get(connector);
		if (IS_ERR(port->retimer)) {
			ret = PTR_ERR(port->retimer);
			port->retimer = NULL;
			typec_mux_put(port->mux);
			port->mux = NULL;
			return dev_err_probe(dev, ret,
					     "failed to get retimer for port %u\n",
					     index);
		}
	} else {
		/*
		 * The connector graph is optional. Keep the ports useful even
		 * without it, matching the default role of the Surface Pro X
		 * connectors.
		 */
		port->cap.type = TYPEC_PORT_SNK;
		port->cap.data = TYPEC_PORT_DFP;
	}

	port->cap.revision = USB_TYPEC_REV_1_2;
	port->cap.orientation_aware = true;
	port->cap.driver_data = stcm;

	port->port = typec_register_port(dev, &port->cap);
	if (IS_ERR(port->port)) {
		ret = PTR_ERR(port->port);
		port->port = NULL;
		goto err_put;
	}

	port->dp_alt = typec_port_register_altmode(port->port, &desc);
	if (IS_ERR(port->dp_alt)) {
		ret = PTR_ERR(port->dp_alt);
		port->dp_alt = NULL;
		typec_unregister_port(port->port);
		port->port = NULL;
		goto err_put;
	}

	ret = devm_add_action_or_reset(dev, surface_typec_port_release, port);
	if (ret)
		return ret;

	return 0;

err_put:
	typec_mux_put(port->mux);
	port->mux = NULL;
	typec_retimer_put(port->retimer);
	port->retimer = NULL;
	return ret;
}

static void surface_typec_notifier_unregister(void *data)
{
	struct surface_typec *stcm = data;

	ssam_device_notifier_unregister(stcm->sdev, &stcm->notif);
}

static int surface_typec_probe(struct ssam_device *sdev)
{
	struct surface_typec *stcm;
	struct device *ctrl_dev = ssam_controller_device(sdev->ctrl);
	bool has_connector[STCM_NUM_PORTS] = {};
	unsigned int i;
	int ret;

	stcm = devm_kzalloc_obj(&sdev->dev, *stcm);
	if (!stcm)
		return -ENOMEM;

	stcm->sdev = sdev;
	ssam_device_set_drvdata(sdev, stcm);

	device_for_each_named_child_node_scoped(ctrl_dev, connector, "connector") {
		u32 reg;

		ret = fwnode_property_read_u32(connector, "reg", &reg);
		if (ret)
			return dev_err_probe(&sdev->dev, ret,
					     "connector is missing reg property\n");

		if (reg >= ARRAY_SIZE(stcm->ports))
			return dev_err_probe(&sdev->dev, -EINVAL,
					     "connector port %u out of range\n",
					     reg);

		if (has_connector[reg])
			return dev_err_probe(&sdev->dev, -EEXIST,
					     "duplicate connector port %u\n",
					     reg);

		ret = surface_typec_register_port(stcm, reg, connector);
		if (ret)
			return ret;

		has_connector[reg] = true;
	}

	for (i = 0; i < ARRAY_SIZE(stcm->ports); i++) {
		if (has_connector[i])
			continue;

		ret = surface_typec_register_port(stcm, i, NULL);
		if (ret)
			return ret;
	}

	stcm->notif.base.priority = 1;
	stcm->notif.base.fn = surface_typec_notify;
	stcm->notif.event.reg = SSAM_EVENT_REGISTRY_SAM;
	stcm->notif.event.id.target_category = SSAM_SSH_TC_USC;
	stcm->notif.event.id.instance = sdev->uid.instance;
	stcm->notif.event.mask = SSAM_EVENT_MASK_NONE;
	stcm->notif.event.flags = SSAM_EVENT_SEQUENCED;

	ret = ssam_device_notifier_register(sdev, &stcm->notif);
	if (ret)
		return ret;

	return devm_add_action_or_reset(&sdev->dev,
					surface_typec_notifier_unregister,
					stcm);
}

static const struct ssam_device_id surface_typec_match[] = {
	{ SSAM_SDEV(USC, SAM, 0x01, 0x00) },
	{ },
};
MODULE_DEVICE_TABLE(ssam, surface_typec_match);

static struct ssam_device_driver surface_typec_driver = {
	.probe = surface_typec_probe,
	.match_table = surface_typec_match,
	.driver = {
		.name = "surface_typec",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};
module_ssam_device_driver(surface_typec_driver);

MODULE_AUTHOR("Val Packett <val@packett.cool>");
MODULE_DESCRIPTION("USB Type-C port driver for Surface System Aggregator Module");
MODULE_LICENSE("GPL");
