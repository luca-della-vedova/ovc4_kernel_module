/*
 * ovc4cam.c - ovc4cam sensor driver
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <media/camera_common.h>
#include "ovc4_driver/ovc4cam_mode_tbls.h"
#define CREATE_TRACE_POINTS
#include <trace/events/imx185.h>

static const struct of_device_id ovc4cam_of_match[] = {
	{ .compatible = "nvidia,ovc4cam",},
	{ },
};
MODULE_DEVICE_TABLE(of, ovc4cam_of_match);

// Dummy, remove?
static const u32 ctrl_cid_list[] = {
  TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

struct ovc4cam {
	struct i2c_client	*i2c_client;
	struct v4l2_subdev	*subdev;
	u32				frame_length;
	s64 last_wdr_et_val;
	struct camera_common_data	*s_data;
	struct tegracam_device		*tc_dev;
};

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
	.use_single_rw = true,
};

static int test_mode;
module_param(test_mode, int, 0644);

static inline int ovc4cam_read_reg(struct camera_common_data *s_data,
				u16 addr, u8 *val)
{
  // unimplemented
  return 0;
}

static int ovc4cam_write_reg(struct camera_common_data *s_data,
				u16 addr, u8 val)
{
  // unimplemented
  return 0;
}

static struct tegracam_ctrl_ops ovc4cam_ctrl_ops = {
	.numctrls = ARRAY_SIZE(ctrl_cid_list),
	.ctrl_cid_list = ctrl_cid_list,
};

static int ovc4cam_power_on(struct camera_common_data *s_data)
{
  // TODO gpio reset here or move to to probe
	int err = 0;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_dbg(dev, "%s: power on\n", __func__);
	if (pdata && pdata->power_on) {
		err = pdata->power_on(pw);
		if (err)
			dev_err(dev, "%s failed.\n", __func__);
		else
			pw->state = SWITCH_ON;
		return err;
	}

	/*exit reset mode: XCLR */
	if (pw->reset_gpio) {
		gpio_set_value(pw->reset_gpio, 0);
		usleep_range(30, 50);
		gpio_set_value(pw->reset_gpio, 1);
		usleep_range(30, 50);
	}

	pw->state = SWITCH_ON;
	return 0;

}

static int ovc4cam_power_off(struct camera_common_data *s_data)
{
	int err = 0;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_dbg(dev, "%s: power off\n", __func__);

	if (pdata && pdata->power_off) {
		err = pdata->power_off(pw);
		if (!err)
			goto power_off_done;
		else
			dev_err(dev, "%s failed.\n", __func__);
		return err;
	}
	/* enter reset mode: XCLR */
	usleep_range(1, 2);
	if (pw->reset_gpio)
		gpio_set_value(pw->reset_gpio, 0);

power_off_done:
	pw->state = SWITCH_OFF;

	return 0;
}

static int ovc4cam_power_get(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;
	struct camera_common_pdata *pdata = s_data->pdata;
	const char *mclk_name;
	struct clk *parent;
	int err = 0;

	pw->reset_gpio = pdata->reset_gpio;

	pw->state = SWITCH_OFF;
	return err;
}

static int ovc4cam_power_put(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;

	if (unlikely(!pw))
		return -EFAULT;

	return 0;
}

static struct camera_common_pdata *ovc4cam_parse_dt(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct device_node *np = dev->of_node;
	struct camera_common_pdata *board_priv_pdata;
	const struct of_device_id *match;
	struct camera_common_pdata *ret = NULL;
	int err;
	int gpio;

  dev_info(dev, "Parsing dt\n");
	if (!np)
		return NULL;

	match = of_match_device(ovc4cam_of_match, dev);
	if (!match) {
		dev_err(dev, "Failed to find matching dt id\n");
		return NULL;
	}
  printk(KERN_INFO "Matched dt device\n");

	board_priv_pdata = devm_kzalloc(dev,
					sizeof(*board_priv_pdata), GFP_KERNEL);
	if (!board_priv_pdata)
		return NULL;

	err = of_property_read_string(np, "mclk",
				      &board_priv_pdata->mclk_name);
	if (err)
		dev_err(dev, "mclk not in DT\n");

	gpio = of_get_named_gpio(np, "reset-gpios", 0);
	if (gpio < 0) {
		if (gpio == -EPROBE_DEFER)
			ret = ERR_PTR(-EPROBE_DEFER);
		dev_err(dev, "reset-gpios not found %d\n", err);
		goto error;
	}
	board_priv_pdata->reset_gpio = (unsigned int)gpio;

	return board_priv_pdata;

error:
	devm_kfree(dev, board_priv_pdata);
	return ret;
}

static int ovc4cam_set_mode(struct tegracam_device *tc_dev)
{
  // unimplemented
  return 0;
}

static int ovc4cam_start_streaming(struct tegracam_device *tc_dev)
{
  // unimplemented
  return 0;
}

static int ovc4cam_stop_streaming(struct tegracam_device *tc_dev)
{
  // unimplemented
  return 0;
}


static struct camera_common_sensor_ops ovc4cam_common_ops = {
	//.numfrmfmts = 0,
	.numfrmfmts = ARRAY_SIZE(ovc4cam_frmfmt),
	.frmfmt_table = ovc4cam_frmfmt,
	.power_on = ovc4cam_power_on,
	.power_off = ovc4cam_power_off,
	.write_reg = ovc4cam_write_reg,
	.read_reg = ovc4cam_read_reg,
	.parse_dt = ovc4cam_parse_dt,
	.power_get = ovc4cam_power_get,
	.power_put = ovc4cam_power_put,
	.set_mode = ovc4cam_set_mode,
	.start_streaming = ovc4cam_start_streaming,
	.stop_streaming = ovc4cam_stop_streaming,
};

static int ovc4cam_board_setup(struct ovc4cam *priv)
{
	struct camera_common_data *s_data = priv->s_data;
	struct device *dev = s_data->dev;
	int err = 0;

	dev_dbg(dev, "%s++\n", __func__);

	err = camera_common_mclk_enable(s_data);
	if (err) {
		dev_err(dev,
			"Error %d turning on mclk\n", err);
		return err;
	}

	err = ovc4cam_power_on(s_data);
	if (err) {
		dev_err(dev,
			"Error %d during power on sensor\n", err);
		return err;
	}

  return 0;
}

static int ovc4cam_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
  // unimplemented
	return 0;
}

static const struct v4l2_subdev_internal_ops ovc4cam_subdev_internal_ops = {
	.open = ovc4cam_open,
};

// TODO consider moving away from i2c driver
static int ovc4cam_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct tegracam_device *tc_dev;
	struct ovc4cam *priv;
	int err;

	dev_info(dev, "probing v4l2 sensor\n");

	if (!IS_ENABLED(CONFIG_OF) || !client->dev.of_node)
		return -EINVAL;

	priv = devm_kzalloc(dev,
			sizeof(struct ovc4cam), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	tc_dev = devm_kzalloc(dev,
			sizeof(struct tegracam_device), GFP_KERNEL);
	if (!tc_dev)
		return -ENOMEM;

	priv->i2c_client = tc_dev->client = client;
	tc_dev->dev = dev;
	strncpy(tc_dev->name, "ovc4cam", sizeof(tc_dev->name));
	tc_dev->dev_regmap_config = &sensor_regmap_config;
	tc_dev->sensor_ops = &ovc4cam_common_ops;
	tc_dev->v4l2sd_internal_ops = &ovc4cam_subdev_internal_ops;
	tc_dev->tcctrl_ops = &ovc4cam_ctrl_ops;

	err = tegracam_device_register(tc_dev);
	if (err) {
		dev_err(dev, "tegra camera driver registration failed\n");
		return err;
	}
	priv->tc_dev = tc_dev;
	priv->s_data = tc_dev->s_data;
	priv->subdev = &tc_dev->s_data->subdev;
	tegracam_set_privdata(tc_dev, (void *)priv);

	err = ovc4cam_board_setup(priv);
	if (err) {
		dev_err(dev, "board setup failed\n");
		return err;
	}

	err = tegracam_v4l2subdev_register(tc_dev, true);
	if (err) {
		dev_err(dev, "tegra camera subdev registration failed\n");
		return err;
	}

	dev_info(dev, "ovc4cam probed successfully\n");

	return 0;
}

static int
ovc4cam_remove(struct i2c_client *client)
{
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct ovc4cam *priv = (struct ovc4cam *)s_data->priv;

	tegracam_v4l2subdev_unregister(priv->tc_dev);
	tegracam_device_unregister(priv->tc_dev);

	return 0;
}

static const struct i2c_device_id ovc4cam_id[] = {
	{ "ovc4cam", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ovc4cam_id);

static struct i2c_driver ovc4cam_i2c_driver = {
	.driver = {
		.name = "ovc4cam",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ovc4cam_of_match),
	},
	.probe = ovc4cam_probe,
	.remove = ovc4cam_remove,
	.id_table = ovc4cam_id,
};

module_i2c_driver(ovc4cam_i2c_driver);

MODULE_DESCRIPTION("Media Controller driver for Sony IMX185");
MODULE_AUTHOR("NVIDIA Corporation");
MODULE_LICENSE("GPL v2");
