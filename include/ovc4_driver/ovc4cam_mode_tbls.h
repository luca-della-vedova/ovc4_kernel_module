/*
 * ovc4cam_tables.h - sensor mode tables for ovc4cam HDR sensor.
 *
 * Copyright (c) 2015-2019, NVIDIA CORPORATION, All Rights Reserved.
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

#ifndef __OVC4CAM_I2C_TABLES__
#define __OVC4CAM_I2C_TABLES__

#define OVC4CAM_TABLE_WAIT_MS	0
#define OVC4CAM_TABLE_END	1

#define ovc4cam_reg struct reg_8

// Structures empty because i2c init is not done here
static ovc4cam_reg ovc4cam_start_stream[] = {
};

static ovc4cam_reg ovc4cam_stop_stream[] = {
};

static ovc4cam_reg ovc4cam_mode_common[] = {
};

static ovc4cam_reg ovc4cam_mode_1920x1080_30fps[] = {
};

static ovc4cam_reg ovc4cam_mode_3264x2464_21fps[] = {
};

enum {
	//OVC4CAM_MODE_3264x2464_21FPS,
  OVC4CAM_MODE_1920x1080_30FPS,

	OVC4CAM_MODE_COMMON,
	OVC4CAM_START_STREAM,
	OVC4CAM_STOP_STREAM,
};

static ovc4cam_reg *mode_table[] = {
	[OVC4CAM_MODE_1920x1080_30FPS] = ovc4cam_mode_1920x1080_30fps,
	//[OVC4CAM_MODE_3264x2464_21FPS] = ovc4cam_mode_3264x2464_21fps,

	[OVC4CAM_MODE_COMMON]  = ovc4cam_mode_common,
	[OVC4CAM_START_STREAM]  = ovc4cam_start_stream,
	[OVC4CAM_STOP_STREAM]  = ovc4cam_stop_stream,
};

static const int ovc4cam_30fps[] = {
	30,
};

static const int ovc4cam_60fps[] = {
	60,
};
/*
 * WARNING: frmfmt ordering need to match mode definition in
 * device tree!
 */
static const struct camera_common_frmfmt ovc4cam_frmfmt[] = {
	{{1920, 1080},	ovc4cam_30fps, 1, 0, OVC4CAM_MODE_1920x1080_30FPS},
	{{1920, 1080},	ovc4cam_60fps, 1, 0, OVC4CAM_MODE_1920x1080_30FPS},
	//{{3264, 2464},	ovc4cam_21fps, 1, 0, OVC4CAM_MODE_3264x2464_21FPS},
	/* Add modes with no device tree support after below */
  /*
	{{3264, 1848},	ovc4cam_28fps, 1, 0, OVC4CAM_MODE_3264x1848_28FPS},
	{{1920, 1080},	ovc4cam_30fps, 1, 0, OVC4CAM_MODE_1920x1080_30FPS},
	{{1280, 720},	ovc4cam_60fps, 1, 0, OVC4CAM_MODE_1280x720_60FPS},
	{{1280, 720},	ovc4cam_120fps, 1, 0, OVC4CAM_MODE_1280x720_120FPS},
  */
};

#endif /* __OVC4CAM_I2C_TABLES__ */
