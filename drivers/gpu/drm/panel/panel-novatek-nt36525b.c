// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Val Packett <val@packett.cool>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	void (*init)(struct mipi_dsi_multi_context *dsi_ctx);
};

struct nt36525b {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct panel_desc *desc;

	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static const struct regulator_bulk_data nt36525b_supplies[] = {
	{ .supply = "vddi", .init_load_uA = 62000, },
	{ .supply = "avdd", .init_load_uA = 100000, },
	{ .supply = "avee", .init_load_uA = 100000, },
};

static inline struct nt36525b *to_nt36525b(struct drm_panel *panel)
{
	return container_of(panel, struct nt36525b, panel);
}

static void nt36525b_reset(struct nt36525b *ctx)
{
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(10000, 20000);

	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000, 20000);

	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(10000, 20000);
}

static void tianma_cebu_init_sequence(struct mipi_dsi_multi_context *dsi_ctx)
{
	/* Select manufacturer page 0x23 and unlock it */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xff, 0x23);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfb, 0x01);

	/* Power, timing and gamma parameters */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x10, 0x40);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x11, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x12, 0xb8);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x15, 0xe3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x16, 0x0b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x29, 0x10);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2a, 0x20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2b, 0x30);

	/* First gamma curve */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x30, 0xff);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x31, 0xff);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x32, 0xff);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x33, 0xfc);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x34, 0xf9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x35, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x36, 0xee);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x37, 0xeb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x38, 0xe8);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x39, 0xe6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x3a, 0xe4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x3b, 0xe2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x3d, 0xe0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x3f, 0xdf);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x40, 0xdd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x41, 0xdc);

	/* Second gamma curve */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x58, 0xff);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x59, 0xfc);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5a, 0xf7);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5b, 0xef);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5c, 0xe6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5d, 0xde);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5e, 0xd6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x5f, 0xd0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0xc8);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x61, 0xc0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x62, 0xba);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x63, 0xb6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x64, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x65, 0xae);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x66, 0xac);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x67, 0xaa);

	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xff, 0x10);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfb, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xba, 0x03);

	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_SET_TEAR_ON, 0x00);

	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	/* POWER_SAVE is CABC: 0 == off, 1 == UI, 3 == movie */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x01);

}

static int nt36525b_on(struct nt36525b *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ctx->desc->init(&dsi_ctx);

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 100);

	return dsi_ctx.accum_err;
}

static int nt36525b_off(struct nt36525b *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 20);

	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int nt36525b_prepare(struct drm_panel *panel)
{
	struct nt36525b *ctx = to_nt36525b(panel);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(nt36525b_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(ctx->panel.dev, "failed to enable regulators: %d\n", ret);
		return ret;
	}

	msleep(20);

	nt36525b_reset(ctx);

	ret = nt36525b_on(ctx);
	if (ret < 0) {
		dev_err(ctx->panel.dev, "failed to initialise panel: %d\n", ret);
		goto err;
	}

	return 0;

err:
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	regulator_bulk_disable(ARRAY_SIZE(nt36525b_supplies), ctx->supplies);
	return ret;
}

static int nt36525b_unprepare(struct drm_panel *panel)
{
	struct nt36525b *ctx = to_nt36525b(panel);
	int ret;

	ret = nt36525b_off(ctx);
	if (ret < 0)
		dev_err(ctx->panel.dev, "failed to disable panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	regulator_bulk_disable(ARRAY_SIZE(nt36525b_supplies), ctx->supplies);

	return 0;
}

static int nt36525b_get_modes(struct drm_panel *panel,
			      struct drm_connector *connector)
{
	struct nt36525b *ctx = to_nt36525b(panel);

	return drm_connector_helper_get_modes_fixed(connector, ctx->desc->mode);
}

static const struct drm_panel_funcs nt36525b_panel_funcs = {
	.prepare = nt36525b_prepare,
	.unprepare = nt36525b_unprepare,
	.get_modes = nt36525b_get_modes,
};

static int nt36525b_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return ret;
}

static const struct backlight_ops nt36525b_bl_ops = {
	.update_status = nt36525b_bl_update_status,
};

static struct backlight_device *nt36525b_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type		= BACKLIGHT_RAW,
		.brightness	= 1024,
		.max_brightness	= 2047,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &nt36525b_bl_ops, &props);
}

static const struct drm_display_mode tianma_cebu_mode = {
	.clock = (720 + 136 + 10 + 130) * (1640 + 16 + 2 + 14) * 60 / 1000,

	.hdisplay    = 720,
	.hsync_start = 720 + 136,
	.hsync_end   = 720 + 136 + 10,
	.htotal      = 720 + 136 + 10 + 130,

	.vdisplay    = 1640,
	.vsync_start = 1640 + 16,
	.vsync_end   = 1640 + 16 + 2,
	.vtotal      = 1640 + 16 + 2 + 14,

	.width_mm  = 69,
	.height_mm = 158,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct panel_desc tianma_cebu_desc = {
	.mode       = &tianma_cebu_mode,
	.lanes      = 4,
	.format     = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO,
	.init       = tianma_cebu_init_sequence,
};

static int nt36525b_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct panel_desc *desc;
	struct nt36525b *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct nt36525b, panel,
				   &nt36525b_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	desc = of_device_get_match_data(dev);
	ctx->desc = desc;

	ret = devm_regulator_bulk_get_const(dev, ARRAY_SIZE(nt36525b_supplies),
					    nt36525b_supplies, &ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						 GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "failed to get reset gpio\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = desc->lanes;
	dsi->format = desc->format;
	dsi->mode_flags = desc->mode_flags;

	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return ret;

	if (!ctx->panel.backlight) {
		ctx->panel.backlight = nt36525b_create_backlight(dsi);
		if (IS_ERR(ctx->panel.backlight))
			return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
					     "failed to create backlight\n");
	}

	ret = devm_drm_panel_add(dev, &ctx->panel);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to add panel\n");

	return devm_mipi_dsi_attach(dev, dsi);
}

static const struct of_device_id nt36525b_of_match[] = {
	{ .compatible = "tianma,nt36525b-cebu", .data = &tianma_cebu_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, nt36525b_of_match);

static struct mipi_dsi_driver nt36525b_dsi_driver = {
	.probe = nt36525b_probe,
	.driver = {
		.name = "panel-novatek-nt36525b",
		.of_match_table = nt36525b_of_match,
	},
};
module_mipi_dsi_driver(nt36525b_dsi_driver);

MODULE_AUTHOR("Val Packett <val@packett.cool>");
MODULE_DESCRIPTION("Driver for Novatek NT36525B based MIPI DSI LCD panels");
MODULE_LICENSE("GPL");
