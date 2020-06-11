/*
 * imx219_tables.h - sensor mode tables for imx219 HDR sensor.
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

#ifndef __IMX219_I2C_TABLES__
#define __IMX219_I2C_TABLES__

#define IMX219_TABLE_WAIT_MS	0
#define IMX219_TABLE_END	1

#define imx219_reg struct reg_8

static imx219_reg imx219_start_stream[] = {
	{0x0100, 0x01},
	{IMX219_TABLE_WAIT_MS, 3},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_stop_stream[] = {
	{0x0100, 0x00},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_common[] = {
	{IMX219_TABLE_WAIT_MS, 10},
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
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_3264x2464_21fps[] = {
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
	{IMX219_TABLE_END, 0x00}
};

enum {
	IMX219_MODE_3264x2464_21FPS,

	IMX219_MODE_COMMON,
	IMX219_START_STREAM,
	IMX219_STOP_STREAM,
};

static imx219_reg *mode_table[] = {
	[IMX219_MODE_3264x2464_21FPS] = imx219_mode_3264x2464_21fps,

	[IMX219_MODE_COMMON]  = imx219_mode_common,
	[IMX219_START_STREAM]  = imx219_start_stream,
	[IMX219_STOP_STREAM]  = imx219_stop_stream,
};

static const int imx219_21fps[] = {
	21,
};

/*
 * WARNING: frmfmt ordering need to match mode definition in
 * device tree!
 */
static const struct camera_common_frmfmt imx219_frmfmt[] = {
	{{3264, 2464},	imx219_21fps, 1, 0, IMX219_MODE_3264x2464_21FPS},
	/* Add modes with no device tree support after below */
  /*
	{{3264, 1848},	imx219_28fps, 1, 0, IMX219_MODE_3264x1848_28FPS},
	{{1920, 1080},	imx219_30fps, 1, 0, IMX219_MODE_1920x1080_30FPS},
	{{1280, 720},	imx219_60fps, 1, 0, IMX219_MODE_1280x720_60FPS},
	{{1280, 720},	imx219_120fps, 1, 0, IMX219_MODE_1280x720_120FPS},
  */
};

#endif /* __IMX219_I2C_TABLES__ */
