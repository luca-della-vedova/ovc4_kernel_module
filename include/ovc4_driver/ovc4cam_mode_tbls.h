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

static ovc4cam_reg ovc4cam_start_stream[] = {
	{0x0100, 0x01},
	{OVC4CAM_TABLE_WAIT_MS, 3},
	{OVC4CAM_TABLE_END, 0x00}
};

static ovc4cam_reg ovc4cam_stop_stream[] = {
	{0x0100, 0x00},
	{OVC4CAM_TABLE_END, 0x00}
};

static ovc4cam_reg ovc4cam_mode_common[] = {
	{OVC4CAM_TABLE_WAIT_MS, 10},
	/* software reset */
	{0x0103, 0x01},
	/* sensor config */
	{0x0114, 0x01}, /* D-Phy, 2-lane */
	{0x0128, 0x00},
	{0x012A, 0x18}, /* 24 MHz INCK */
	{0x012B, 0x00},
	/* access code - vendor addr. ranges */
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	/* cis tuning */
	{0x455E, 0x00},
	{0x471E, 0x4B},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x4540, 0x00},
	{0x47B4, 0x14},
	{0x4713, 0x30},
	{0x478B, 0x10},
	{0x478F, 0x10},
	{0x4793, 0x10},
	{0x4797, 0x0E},
	{0x479B, 0x0E},
	{OVC4CAM_TABLE_END, 0x00}
};

static ovc4cam_reg ovc4cam_mode_3264x2464_21fps[] = {
	/* capture settings */
	{0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x09}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0xbd}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
	{0x0160, 0x09}, /* FRM_LENGTH[15:8] */
	{0x0161, 0xC1}, /* FRM_LENGTH[7:0] */
	{0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
	{0x0163, 0x78}, /* LINE_LENGTH[7:0] */
	{0x0164, 0x00},
	{0x0165, 0x08},
	{0x0166, 0x0C},
	{0x0167, 0xC7},
	{0x0168, 0x00},
	{0x0169, 0x00},
	{0x016A, 0x09},
	{0x016B, 0x9F},
	{0x016C, 0x0C},
	{0x016D, 0xC0},
	{0x016E, 0x09},
	{0x016F, 0xA0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0264, 0x00},
	{0x0265, 0x08},
	{0x0266, 0x0C},
	{0x0267, 0xC7},
	{0x026C, 0x0C},
	{0x026D, 0xC0},
	/* clock dividers */
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x39},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x72},
	{OVC4CAM_TABLE_END, 0x00}
};

enum {
	OVC4CAM_MODE_3264x2464_21FPS,

	OVC4CAM_MODE_COMMON,
	OVC4CAM_START_STREAM,
	OVC4CAM_STOP_STREAM,
};

static ovc4cam_reg *mode_table[] = {
	[OVC4CAM_MODE_3264x2464_21FPS] = ovc4cam_mode_3264x2464_21fps,

	[OVC4CAM_MODE_COMMON]  = ovc4cam_mode_common,
	[OVC4CAM_START_STREAM]  = ovc4cam_start_stream,
	[OVC4CAM_STOP_STREAM]  = ovc4cam_stop_stream,
};

static const int ovc4cam_21fps[] = {
	21,
};

/*
 * WARNING: frmfmt ordering need to match mode definition in
 * device tree!
 */
static const struct camera_common_frmfmt ovc4cam_frmfmt[] = {
	{{3264, 2464},	ovc4cam_21fps, 1, 0, OVC4CAM_MODE_3264x2464_21FPS},
	/* Add modes with no device tree support after below */
  /*
	{{3264, 1848},	ovc4cam_28fps, 1, 0, OVC4CAM_MODE_3264x1848_28FPS},
	{{1920, 1080},	ovc4cam_30fps, 1, 0, OVC4CAM_MODE_1920x1080_30FPS},
	{{1280, 720},	ovc4cam_60fps, 1, 0, OVC4CAM_MODE_1280x720_60FPS},
	{{1280, 720},	ovc4cam_120fps, 1, 0, OVC4CAM_MODE_1280x720_120FPS},
  */
};

#endif /* __OVC4CAM_I2C_TABLES__ */
