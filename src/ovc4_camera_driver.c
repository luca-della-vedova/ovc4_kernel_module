/*
 * ovc4cam.c - ovc4cam sensor driver
 *
 * Copyright (c) 2020 Open Source Robotics Foundation
 *
 * Based on imx185.c from
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

// Add UIO features
#include <linux/uio_driver.h>

#include <media/camera_common.h>
#include "ovc4_driver/ovc4cam_mode_tbls.h"
#include "ovc4_driver/ovc4_camera_driver.h"
#include "ovc4_driver/uio_map.h"

static char *sensor_name = "null";
module_param(sensor_name, charp, 0);
MODULE_PARM_DESC(sensor_name, "Name of the sensor to connect");

static int last_sensor_id = 0; // Hacky, to share between probe and parse_dt function

static const struct of_device_id ovc4cam_of_match[] = {
  { .compatible = "nvidia,ovc4cam",},
  { },
};
MODULE_DEVICE_TABLE(of, ovc4cam_of_match);

static const char* ovc4cam_supported_sensors[] = {
  "picam_v2",
};

// Dummy, remove?
static const u32 ctrl_cid_list[] = {
  TEGRA_CAMERA_CID_GAIN,
  TEGRA_CAMERA_CID_EXPOSURE,
  TEGRA_CAMERA_CID_FRAME_RATE,
  TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

struct ovc4cam {
  struct i2c_client *i2c_client;
  struct v4l2_subdev  *subdev;
  u32       frame_length;
  struct camera_common_data *s_data;
  struct tegracam_device    *tc_dev;

  // UIO stuff for exposure and userspace communication
  struct uio_info *uioinfo;
  // spinlock_t lock;
  // unsigned long flags;
  // struct platform_device *pdev;
  // This structure contains the memory mapped data
  struct uio_map *uiomap;
};

static const struct regmap_config sensor_regmap_config = {
  .reg_bits = 16,
  .val_bits = 8,
  .cache_type = REGCACHE_RBTREE,
  .use_single_rw = true,
};

static int test_mode;
module_param(test_mode, int, 0644);

static int imx219_set_gain(struct tegracam_device *tc_dev, s64 val)
{
  // Set the gain to be accessed through mmap
  struct ovc4cam *priv = (struct ovc4cam *)tc_dev->priv;
  // TODO not return 0 since this is not a success?
  if (priv == NULL || priv->uiomap == NULL)
    return 0;
  priv->uiomap->gain = val;
  return 0;
}

static int imx219_set_exposure(struct tegracam_device *tc_dev, s64 val)
{
  // Set the exposure to be accessed through mmap
  struct ovc4cam *priv = (struct ovc4cam *)tc_dev->priv;
  // TODO not return 0 since this is not a success?
  if (priv == NULL || priv->uiomap == NULL)
    return 0;
  priv->uiomap->exposure = val;
  return 0;
}

static int imx219_set_frame_rate(struct tegracam_device *tc_dev, s64 val)
{
  // Unimplemented
  return 0;
}

static int imx219_set_group_hold(struct tegracam_device *tc_dev, bool val)
{
  // Unimplemented
  return 0;
}

static inline int ovc4cam_read_reg(struct camera_common_data *s_data, u16 addr, u8 *val)
{
  // Unimplemented
  return 0;
}

static int ovc4cam_write_reg(struct camera_common_data *s_data, u16 addr, u8 val)
{
  // Unimplemented
  return 0;
}

static struct tegracam_ctrl_ops ovc4cam_ctrl_ops = {
  .numctrls = ARRAY_SIZE(ctrl_cid_list),
  .ctrl_cid_list = ctrl_cid_list,
  .set_gain = imx219_set_gain,
  .set_exposure = imx219_set_exposure,
  .set_frame_rate = imx219_set_frame_rate,
  .set_group_hold = imx219_set_group_hold,
};

static int ovc4cam_power_on(struct camera_common_data *s_data)
{
  // TODO gpio reset here or move to to probe
  int err = 0;
  struct camera_common_power_rail *pw = s_data->power;
  struct camera_common_pdata *pdata = s_data->pdata;
  struct device *dev = s_data->dev;

  dev_info(dev, "Powering on sensor\n");
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
    dev_info(dev, "Found reset gpio\n");
    gpio_set_value(pw->reset_gpio, 0);
    usleep_range(30, 50);
    gpio_set_value(pw->reset_gpio, 1);
    usleep_range(30, 50);
  }
  else
    dev_info(dev, "gpio not found\n");

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
  dev_info(dev, "Powering off sensor\n");

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
  // Hacky, only for testing
  //if (pw->reset_gpio)
  //  gpio_set_value(pw->reset_gpio, 0);

power_off_done:
  pw->state = SWITCH_OFF;

  return 0;
}

static int ovc4cam_power_get(struct tegracam_device *tc_dev)
{
  struct camera_common_data *s_data = tc_dev->s_data;
  struct camera_common_power_rail *pw = s_data->power;
  struct camera_common_pdata *pdata = s_data->pdata;
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
  int sensor_id;

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

  if (!of_property_read_s32(np, "sensor_id", &sensor_id))
    dev_info(dev, "No sensor_id found in device tree\n");

  dev_info(dev, "sensor_id is %d\n", sensor_id);
  last_sensor_id = sensor_id;

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

static int ovc4cam_get_sensor_num(const char *sensor_name)
{
  int sensor_idx;
  for (sensor_idx = 0; sensor_idx < ARRAY_SIZE(ovc4cam_supported_sensors);
      ++sensor_idx)
  {
    if (strcmp(sensor_name, ovc4cam_supported_sensors[sensor_idx]) == 0)
      return sensor_idx;
  }
  return -1;
}

// TODO consider moving away from i2c driver
static int ovc4cam_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
  struct device *dev = &client->dev;
  struct tegracam_device *tc_dev;
  struct ovc4cam *priv;
  struct uio_info *uioinfo;
  struct uio_map *uiomap;
  int err;

  dev_info(dev, "probing v4l2 sensor\n");
  dev_info(dev, "sensor name is %s\n", sensor_name); 

  int sensor_id = ovc4cam_get_sensor_num(sensor_name);
  
  //dev_info(dev, "sensor id is %d\n", sensor_id);
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

  // UIO stuff
  uioinfo = devm_kzalloc(dev,
      sizeof(struct uio_info), GFP_KERNEL);
  if (!uioinfo)
    return -ENOMEM;

  uiomap = vzalloc(sizeof(struct uio_map));
  if (!uioinfo)
    return -ENOMEM;
  
  priv->i2c_client = tc_dev->client = client;
  tc_dev->dev = dev;
  strncpy(tc_dev->name, "ovc4cam", sizeof(tc_dev->name));
  tc_dev->dev_regmap_config = &sensor_regmap_config;
  tc_dev->sensor_ops = &ovc4cam_common_ops;
  tc_dev->v4l2sd_internal_ops = &ovc4cam_subdev_internal_ops;
  tc_dev->tcctrl_ops = &ovc4cam_ctrl_ops;

  dev_info(dev, "Registering device\n");
  err = tegracam_device_register(tc_dev);
  dev_info(dev, "Registered device\n");
  if (err) {
    dev_err(dev, "tegra camera driver registration failed\n");
    return err;
  }
  priv->tc_dev = tc_dev;
  priv->s_data = tc_dev->s_data;
  priv->subdev = &tc_dev->s_data->subdev;

  // UIO for memory mapped operations
  uioinfo->mem[0].addr = (phys_addr_t) uiomap;
  uioinfo->mem[0].size = sizeof(uiomap);
  uioinfo->mem[0].memtype = UIO_MEM_VIRTUAL;
  uioinfo->version = "0.1";
  uioinfo->name = "ovc4cam_control";

  if (uio_register_device(dev, uioinfo))
    dev_err(dev, "error registering uio device\n");

  priv->uioinfo = uioinfo;
  priv->uiomap = uiomap;

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
  uiomap->sensor_id = last_sensor_id;

  return 0;
}

static int
ovc4cam_remove(struct i2c_client *client)
{
  struct camera_common_data *s_data = to_camera_common_data(&client->dev);
  struct ovc4cam *priv = (struct ovc4cam *)s_data->priv;

  tegracam_v4l2subdev_unregister(priv->tc_dev);
  tegracam_device_unregister(priv->tc_dev);

  // Free valloc memory
  vfree((void *)priv->uioinfo->mem[0].addr);
  uio_unregister_device(priv->uioinfo);

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

MODULE_DESCRIPTION("Driver for OVC4 cameras");
MODULE_AUTHOR("Open Robotics");
MODULE_LICENSE("GPL v2");
