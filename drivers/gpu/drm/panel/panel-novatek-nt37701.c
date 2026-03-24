// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, The Linux Foundation. All rights reserved.
 * Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
 *   Copyright (c) 2013, The Linux Foundation. All rights reserved.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct nt37701_panel;

struct nt37701_panel_mode {
	const struct drm_display_mode mode;
	u8 mode_id;
	u8 tianma_e3_cmd_bytes[5];
	bool is_low_hz;
};

struct nt37701_panel_desc {
	int (*panel_on)(struct nt37701_panel *ctx, const struct nt37701_panel_mode *mode);
};

struct nt37701_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct regulator *supply;
	struct gpio_desc *reset_gpio;
	const struct nt37701_panel_desc *desc;
};

static const struct nt37701_panel_mode nt37701_panel_modes[5];

struct nt37701_panel_state {
	struct drm_panel_state base;
	const struct nt37701_panel_mode *mode_data;
};

static inline
struct nt37701_panel *to_nt37701_panel(struct drm_panel *panel)
{
	return container_of(panel, struct nt37701_panel, panel);
}

static inline
struct nt37701_panel_state *to_nt37701_panel_state(struct drm_panel_state *state)
{
	return container_of(state, struct nt37701_panel_state, base);
}

static void nt37701_panel_reset(struct nt37701_panel *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

#define nt37701_panel_switch_page(ctx, page) \
	mipi_dsi_dcs_write_seq_multi((ctx), 0xf0, 0x55, 0xaa, 0x52, 0x08, (page))

static int nt37701_panel_on(struct nt37701_panel *ctx, const struct nt37701_panel_mode *mode)
{
	return ctx->desc->panel_on(ctx, mode);
}

static int nt37701_tianma_panel_on(struct nt37701_panel *ctx, const struct nt37701_panel_mode *mode)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	/* Voltage */
	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x00, 0x18, 0x4f);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x11);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2,
				     0x60, 0x50, 0x66, 0x91, 0x86, 0x91);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x00, 0x08, 0x01, 0x5f, 0x01, 0x5f, 0x02,
				     0xa4, 0x02, 0xa4, 0x03, 0xbb);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x03, 0xbb, 0x05, 0x2f, 0x05, 0x2f, 0x06,
				     0x91, 0x06, 0x91, 0x06, 0x92);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x06, 0x92, 0x0a, 0x2f, 0x0a, 0x2f, 0x0d,
				     0xb9, 0x0d, 0xb9, 0x0f, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x58, 0x00);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc3, 0x04);

	nt37701_panel_switch_page(&dsi_ctx, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x11, 0x00, 0x09, 0x51);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc,
				     0x00, 0x09, 0x51, 0x00, 0x09, 0x51, 0x00,
				     0x0b, 0x41);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x0d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbc, 0x00, 0x11, 0x51);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc6,
				     0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
				     0x66, 0x66, 0x66, 0x66, 0x66);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x1c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0,
				     0x15, 0x0a, 0x38, 0x27, 0x49);

	/* DSC */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x03, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x90, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x91,
				     0xab, 0x28, 0x00, 0x0c, 0xc2, 0x00, 0x03,
				     0x1c, 0x01, 0x7e, 0x00, 0x0f, 0x08, 0xbb,
				     0x04, 0x3d, 0x10, 0xf0);

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_MEMORY_START,
				     0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x88, 0x02, 0x1c, 0x08, 0x73);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5f, 0x00);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x0000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x20);
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_set_column_address_multi(&dsi_ctx, 0x0000, 0x0437);
	mipi_dsi_dcs_set_page_address_multi(&dsi_ctx, 0x0000, 0x095f);

	mipi_dsi_dcs_write_var_seq_multi(&dsi_ctx, 0x2f, mode->mode_id);
	nt37701_panel_switch_page(&dsi_ctx, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe1,
				     0x00, 0x03, 0x03, 0x03, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_var_seq_multi(&dsi_ctx, 0xe3,
					mode->tianma_e3_cmd_bytes[0], mode->tianma_e3_cmd_bytes[1],
					mode->tianma_e3_cmd_bytes[2], mode->tianma_e3_cmd_bytes[3],
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					mode->tianma_e3_cmd_bytes[4]);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe4,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe5,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe6,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe7,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x00);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0, 0x0e, 0xb3, 0x21);

	nt37701_panel_switch_page(&dsi_ctx, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x03);
	if (mode->is_low_hz) {
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x02);
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce,
					     0x22, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	} else {
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc1, 0x03);
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xce,
					     0x22, 0x00, 0x00, 0x22, 0x00, 0x00, 0x02,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	}
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	nt37701_panel_switch_page(&dsi_ctx, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe1, 0x63);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);

	return dsi_ctx.accum_err;
}

static int nt37701_csot_panel_on(struct nt37701_panel *ctx, const struct nt37701_panel_mode *mode)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	/* Voltage */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb5, 0x4b, 0x22, 0x4f);

	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb2,
				     0x60, 0x50, 0x65, 0xe9, 0x85, 0xe9, 0x2f,
				     0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x00, 0x01, 0x01, 0x59, 0x01, 0x59, 0x02,
				     0x1a, 0x02, 0x1a, 0x03, 0x0a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x0c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x03, 0x0a, 0x04, 0x00, 0x04, 0x00, 0x05,
				     0xe9, 0x05, 0xe9, 0x05, 0xea);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3,
				     0x05, 0xea, 0x09, 0x08, 0x09, 0x08, 0x0d,
				     0xbb, 0x0d, 0xbb, 0x0f, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x58, 0x00);
	nt37701_panel_switch_page(&dsi_ctx, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x1c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc0,
				     0x03, 0x14, 0x25, 0xa9, 0x78);
	
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0xaa, 0x55, 0xa5, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6f, 0x1d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x05);
	
	/* DSC */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x03, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x90, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x91,
				     0xab, 0x28, 0x00, 0x0c, 0xc2, 0x00, 0x03,
				     0x1c, 0x01, 0x7e, 0x00, 0x0f, 0x08, 0xbb,
				     0x04, 0x3d, 0x10, 0xf0);

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_MEMORY_START,
				     0x00);
	mipi_dsi_dcs_set_display_brightness_multi(&dsi_ctx, 0x0000);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x20);
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_set_column_address_multi(&dsi_ctx, 0x0000, 0x0437);
	mipi_dsi_dcs_set_page_address_multi(&dsi_ctx, 0x0000, 0x095f);

	mipi_dsi_dcs_write_var_seq_multi(&dsi_ctx, 0x2f, mode->mode_id);

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);

	return dsi_ctx.accum_err;
}


static int nt37701_panel_disable(struct drm_panel *panel)
{
	struct nt37701_panel *ctx = to_nt37701_panel(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 50);

	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 100);

	return dsi_ctx.accum_err;
}

static int nt37701_panel_atomic_check(struct drm_panel *panel,
				     struct drm_atomic_commit *commit)
{
	struct nt37701_panel_state *panel_state = to_nt37701_panel_state(drm_atomic_get_new_panel_state(commit, panel));
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state_for_panel(commit, panel);
	int i;

	if (!panel_state || !crtc_state)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(nt37701_panel_modes); i++) {
		if (drm_mode_match(&crtc_state->mode,
				   &nt37701_panel_modes[i].mode,
				   DRM_MODE_MATCH_TIMINGS | DRM_MODE_MATCH_CLOCK)) {
			panel_state->mode_data = &nt37701_panel_modes[i];
			return 0;
		}
	}

	return -EINVAL;
}

static int nt37701_panel_atomic_prepare(struct drm_panel *panel,
				       struct drm_atomic_commit *commit)
{
	struct nt37701_panel *ctx = to_nt37701_panel(panel);
	struct nt37701_panel_state *panel_state = to_nt37701_panel_state(drm_atomic_get_new_panel_state(commit, panel));
	struct drm_dsc_picture_parameter_set pps;
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!panel_state)
		return -EINVAL;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(dev, "Failed to enable power supply: %d\n", ret);
		return ret;
	}

	nt37701_panel_reset(ctx);

	ret = nt37701_panel_on(ctx, panel_state->mode_data);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		goto err;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(panel->dev, "failed to transmit PPS: %d\n", ret);
		goto err;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		goto err;
	}

	msleep(28);

	return 0;

err:
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return ret;
}

static int nt37701_panel_atomic_unprepare(struct drm_panel *panel,
					 struct drm_atomic_commit *commit)
{
	struct nt37701_panel *ctx = to_nt37701_panel(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_disable(ctx->supply);

	return 0;
}

static int nt37701_panel_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	int count = 0;

	for (int i = 0; i < ARRAY_SIZE(nt37701_panel_modes); i++)
		count += drm_connector_helper_get_modes_fixed(connector,
						    &nt37701_panel_modes[i].mode);

	connector->display_info.bpc = 10;

	return count;
}

static struct drm_panel_state *
nt37701_atomic_create_state(struct drm_panel *panel)
{
	struct nt37701_panel_state *state;

	state = kzalloc_obj(*state);
	if (!state)
		return ERR_PTR(-ENOMEM);

	__drm_atomic_helper_panel_state_init(&state->base, panel);

	return &state->base;
}

static struct drm_panel_state *
nt37701_atomic_duplicate_state(struct drm_panel *panel)
{
	struct drm_panel_state *old_state = drm_panel_get_current_state(panel);
	struct nt37701_panel_state *state;

	if (!old_state)
		return NULL;

	state = kzalloc_obj(*state);
	if (!state)
		return NULL;

	__drm_atomic_helper_panel_duplicate_state(panel, &state->base);
	state->mode_data = to_nt37701_panel_state(old_state)->mode_data;

	return &state->base;
}

static void nt37701_atomic_destroy_state(struct drm_panel *panel,
					 struct drm_panel_state *state)
{
	kfree(to_nt37701_panel_state(state));
}

static const struct drm_panel_funcs nt37701_panel_funcs = {
	.atomic_check = nt37701_panel_atomic_check,
	.atomic_prepare = nt37701_panel_atomic_prepare,
	.atomic_unprepare = nt37701_panel_atomic_unprepare,
	.disable = nt37701_panel_disable,
	.get_modes = nt37701_panel_get_modes,
	.atomic_create_state = nt37701_atomic_create_state,
	.atomic_duplicate_state = nt37701_atomic_duplicate_state,
	.atomic_destroy_state = nt37701_atomic_destroy_state,
};

static int nt37701_panel_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static int nt37701_panel_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops nt37701_panel_bl_ops = {
	.update_status = nt37701_panel_bl_update_status,
	.get_brightness = nt37701_panel_bl_get_brightness,
};

static struct backlight_device *
nt37701_panel_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 1757, // Middle of the non-extended range (0-3514)
		.max_brightness = 4095,
		.scale = BACKLIGHT_SCALE_NON_LINEAR,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &nt37701_panel_bl_ops, &props);
}

static int nt37701_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct nt37701_panel *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, __typeof(*ctx), panel,
				  &nt37701_panel_funcs,
				  DRM_MODE_CONNECTOR_DSI);

	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->supply = devm_regulator_get(dev, "power");
	if (IS_ERR(ctx->supply))
		return dev_err_probe(dev, PTR_ERR(ctx->supply),
				     "Failed to get power-supply\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	ctx->desc = of_device_get_match_data(&dsi->dev);
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB101010;
	dsi->mode_flags = MIPI_DSI_MODE_NO_EOT_PACKET | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = nt37701_panel_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;

	/* TODO: Pass slice_per_pkt = 1 */
	ctx->dsc.slice_height = 12;
	ctx->dsc.slice_width = 1080;
	ctx->dsc.slice_count = 1;
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 10 << 4;
	ctx->dsc.block_pred_enable = true;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void nt37701_panel_remove(struct mipi_dsi_device *dsi)
{
	struct nt37701_panel *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

#define NT37701_MODE(hz) { \
		.clock = (1080 + 32 + 16 + 32) * (2400 + 12 + 4 + 4) * (hz) / 1000, \
		.hdisplay = 1080, \
		.hsync_start = 1080 + 32, \
		.hsync_end = 1080 + 32 + 16, \
		.htotal = 1080 + 32 + 16 + 32, \
		.vdisplay = 2400, \
		.vsync_start = 2400 + 12, \
		.vsync_end = 2400 + 12 + 4, \
		.vtotal = 2400 + 12 + 4 + 4, \
		.width_mm = 69, \
		.height_mm = 152, \
		.type = DRM_MODE_TYPE_DRIVER, \
	}

static const struct nt37701_panel_mode nt37701_panel_modes[] = {
	{
		.mode = NT37701_MODE(144),
		.mode_id = 0x04,
		.tianma_e3_cmd_bytes = { 0x48, 0xf3, 0x49, 0xc0, 0xfc },
	},
	{
		.mode = NT37701_MODE(120),
		.mode_id = 0x02,
		.tianma_e3_cmd_bytes = { 0x94, 0x36, 0x53, 0x40, 0xff },
	},
	{
		.mode = NT37701_MODE(90),
		.mode_id = 0x07,
		.tianma_e3_cmd_bytes = { 0x25, 0xfe, 0xe1, 0x4f, 0x92 },
	},
	{
		.mode = NT37701_MODE(60),
		.mode_id = 0x03,
		.tianma_e3_cmd_bytes = { 0x46, 0xcc, 0x11, 0x40, 0xf0 },
	},
	{
		.mode = NT37701_MODE(48),
		.mode_id = 0x08,
		.tianma_e3_cmd_bytes = { 0xc0, 0x0e, 0xf3, 0x5d, 0x98 },
		.is_low_hz = true,
	},
};

static const struct nt37701_panel_desc tianma_desc = {
	.panel_on = nt37701_tianma_panel_on,
};

static const struct nt37701_panel_desc csot_desc = {
	.panel_on = nt37701_csot_panel_on,
};

static const struct of_device_id nt37701_panel_of_match[] = {
	{ .compatible = "tianma,nt37701-dubai", .data = &tianma_desc },
	{ .compatible = "csot,nt37701-dubai", .data = &csot_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nt37701_panel_of_match);

static struct mipi_dsi_driver nt37701_panel_driver = {
	.probe = nt37701_panel_probe,
	.remove = nt37701_panel_remove,
	.driver = {
		.name = "panel-novatek-nt37701",
		.of_match_table = nt37701_panel_of_match,
	},
};
module_mipi_dsi_driver(nt37701_panel_driver);

MODULE_AUTHOR("Val Packett <val@packett.cool>");
MODULE_DESCRIPTION("Driver for Novatek NT37701 based DSI panels");
MODULE_LICENSE("GPL");
