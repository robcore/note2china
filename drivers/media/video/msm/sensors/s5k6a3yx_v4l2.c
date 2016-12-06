/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <mach/board.h>
#include "msm_sensor.h"
//#include "s5k6a3yx.h"

extern struct platform_driver s5k6a3yx_driver;
extern struct class *camera_class;

#define	s5k6a3yx_DEBUG
#ifdef s5k6a3yx_DEBUG
#define CAM_DEBUG(fmt, arg...)	\
do {\
	printk(KERN_DEBUG "[s5k6a3yx] %s:" fmt "\n", \
	__func__, ##arg); } \
	while (0)

#define cam_info(fmt, arg...)	\
do {\
	printk(KERN_INFO "[s5k6a3yx] %s:" fmt "\n", __func__, ##arg); } \
	while (0)

#define cam_err(fmt, arg...)	\
do {\
	printk(KERN_ERR "[s5k6a3yx] %s:" fmt "\n", __func__, ##arg); } \
	while (0)

#define cam_i2c_dbg(fmt, arg...)	\
do { \
	printk(KERN_ERR "[s5k6a3yx] %s:" fmt "\n", __func__, ##arg); } \
	while (0)
#else
#define CAM_DEBUG(fmt, arg...)
#define cam_info(fmt, arg...)
#define cam_err(fmt, arg...)
#define cam_i2c_dbg(fmt, arg...)
#endif

#define SENSOR_NAME "s5k6a3yx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k6a3yx"
#define s5k6a3yx_obj s5k6a3yx_##obj

#define VISION_MODE_TEST_PATTERN 	0

#define VISION_MODE_AE_REG_ADDR		0x600a
#define VISION_MODE_AE_BACKLIGHT	0x7a
#define VISION_MODE_AE_NORMAL		0x2a

#define VISION_MODE_SET_FPS_ADDR         0x6027
#define VISION_MODE_SET_FPS_5           0x1
#define VISION_MODE_SET_FPS_10          0X2  
#define VISION_MODE_FPS_5_VAL           0xD0
#define VISION_MODE_FPS_10_VAL          0x68

DEFINE_MUTEX(s5k6a3yx_mut);
static struct msm_sensor_ctrl_t s5k6a3yx_s_ctrl;
int32_t msm_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf);
int32_t msm_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf);

static struct msm_camera_i2c_reg_conf s5k6a3yx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_start_settings_vision[] = {
	{0x4100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_stop_settings_vision[] = {
	{0x4100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_mode0_settings[] = {
	{0x0344, 0x00}, /* x_addr_start MSB */
	{0x0345, 0x00}, /* x_addr_start LSB */
	{0x0346, 0x00}, /* y_addr_start MSB */
	{0x0347, 0x00}, /* y_addr_start LSB */
	{0x0348, 0x05}, /* x_addr_end MSB */
	{0x0349, 0x83}, /* x_addr_end LSB */
	{0x034A, 0x05}, /* y_addr_end MSB */
	{0x034B, 0x83}, /* y_addr_end LSB */
	{0x034C, 0x05}, /* x_output_size */
	{0x034D, 0x84}, /* x_output_size */
	{0x034E, 0x05}, /* y_output_size */
	{0x034F, 0x84}, /* y_output_size */
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_mode1_settings[] = {
	{0x0344, 0x00}, /* x_addr_start MSB */
	{0x0345, 0x00}, /* x_addr_start LSB */
	{0x0346, 0x00}, /* y_addr_start MSB */
	{0x0347, 0x00}, /* y_addr_start LSB */
	{0x0348, 0x05}, /* x_addr_end MSB */
	{0x0349, 0x83}, /* x_addr_end LSB */
	{0x034A, 0x05}, /* y_addr_end MSB */
	{0x034B, 0x83}, /* y_addr_end LSB */
	{0x034C, 0x05}, /* x_output_size */
	{0x034D, 0x80}, /* x_output_size */
	{0x034E, 0x05}, /* y_output_size */
	{0x034F, 0x84}, /* y_output_size */
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_recommend_settings[] = {
	{0x0100, 0x00}, /* Streaming off */
	{0x3061, 0x55},
	{0x3062, 0x54},
	{0x5703, 0x07},
	{0x5704, 0x07},
	{0x305E, 0x0D},
	{0x305F, 0x2E},
	{0x3052, 0x01},
	{0x300B, 0x28},
	{0x300C, 0x2E},
	{0x3004, 0x0A},
	{0x5700, 0x08},
	{0x3005, 0x3D},
	{0x3008, 0x1E},
	{0x3025, 0x40},
	{0x3023, 0x20},
	{0x3029, 0xFF},
	{0x302A, 0xFF},
	{0x3505, 0x41},
	{0x3506, 0x00},
	{0x3521, 0x01},
	{0x3522, 0x01},
	{0x3D20, 0x63},
	{0x3095, 0x15},
	{0x3110, 0x01},
	{0x3111, 0x62},
	{0x3112, 0x0E},
	{0x3113, 0xBC},
	{0x311D, 0x30},
	{0x311F, 0x40},
	{0x3009, 0x1E},
	{0x0138, 0x00},
	/* MIPI CLK 720Mbps */
	{0x0305, 0x06}, /* pre_pll_clk_div */
	{0x0306, 0x00}, /* pll_multiplier MSB */
	{0x0307, 0xB4}, /* pll_multiplier LSB */
	{0x0820, 0x02}, /* requested_link_bit_rate_mbps MSB MSB */
	{0x0821, 0xD0}, /* requested_link_bit_rate_mbps MSB LSB */
	{0x0822, 0x00}, /* requested_link_bit_rate_mbps LSB MSB */
	{0x0823, 0x00}, /* requested_link_bit_rate_mbps LSB LSB */
	{0x0101, 0x00}, /* image_orientation */
	{0x0111, 0x02}, /* CSI_signaling_mode */
	{0x0112, 0x0A}, /* CSI_data_format MSB */
	{0x0113, 0x0A}, /* CSI_data_format LSB */
	{0x0136, 0x18}, /* extclk_frequency_mhz MSB */
	{0x0137, 0x00}, /* extclk_frequency_mhz LSB */
	{0x0200, 0x01}, /* fine_integration_time MSB */
	{0x0201, 0xD3}, /* fine_integration_time LSB */
	{0x0202, 0x05}, /* coarse_integration_time MSB */
	{0x0203, 0xB9}, /* coarse_integration_time LSB */
	{0x0204, 0x00}, /* analogue_gain_code_global MSB */
	{0x0205, 0x32}, /* analogue_gain_code_global LSB */
	{0x0340, 0x05}, /* frame_length_lines MSB */
	{0x0341, 0xAA}, /* frame_length_lines LSB */
	{0x0342, 0x06}, /* line_length_pck MSB */
	{0x0343, 0x42}, /* line_length_pck LSB */
	{0x0381, 0x01}, /* x_even_inc */
	{0x0383, 0x01}, /* x_odd_inc */
	{0x0385, 0x01}, /* y_even_inc */
	{0x0387, 0x01}, /* y_odd_inc */
	{0x0408, 0x00}, /* digital_crop_x_offset MSB */
	{0x0409, 0x00}, /* digital_crop_x_offset LSB */
	{0x040A, 0x00}, /* digital_crop_y_offset MSB */
	{0x040B, 0x00}, /* digital_crop_y_offset LSB */
	{0x040C, 0x05}, /* digital_crop_image_width MSB */
	{0x040D, 0x84}, /* digital_crop_image_width LSB */
	{0x040E, 0x05}, /* digital_crop_image_height MSB */
	{0x040F, 0x84}, /* digital_crop_image_height LSB */
	{0x0900, 0x00}, /* binning_mode */
	{0x0105, 0x01}, /* frame mask */
	};
static struct v4l2_subdev_info s5k6a3yx_subdev_info[] = {
	{
	.code = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt = 1,
	.order = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k6a3yx_init_conf[] = {
	{&s5k6a3yx_recommend_settings[0],
	ARRAY_SIZE(s5k6a3yx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k6a3yx_confs[] = {
	{&s5k6a3yx_mode0_settings[0],
	ARRAY_SIZE(s5k6a3yx_mode0_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k6a3yx_mode1_settings[0],
	ARRAY_SIZE(s5k6a3yx_mode1_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k6a3yx_dimensions[] = {
	/* mode 0 */
	{
		.x_output = 0x0584, /* 1412 */
		.y_output = 0x0584, /* 1412 */
		.line_length_pclk = 0x0642, /* 1602 */
		.frame_length_lines = 0x05AA, /* 1450*/
		.vt_pixel_clk =  72000000, /*76800000*/
		.op_pixel_clk = 72000000, /*76800000*/
		.binning_factor = 1,
	},
	/* mode 1 */
	{
		.x_output = 0x0580, /* 1408 */
		.y_output = 0x0584, /* 1412 */
		.line_length_pclk = 0x0642, /* 1602 */
		.frame_length_lines = 0x05AA, /* 1450*/
		.vt_pixel_clk =  72000000, /*76800000*/
		.op_pixel_clk = 72000000, /*76800000*/
		.binning_factor = 1,
	},
};

static struct msm_sensor_output_reg_addr_t s5k6a3yx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k6a3yx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0000,
};

static struct msm_sensor_exp_gain_info_t s5k6a3yx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 8,
};

static const struct i2c_device_id s5k6a3yx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k6a3yx_s_ctrl},
	{ }
};

static enum msm_camera_vreg_name_t s5k6a3yx_veg_seq[] = {
	CAM_VANA,
	CAM_VDIG,
	CAM_VIO,
};

static struct i2c_driver s5k6a3yx_i2c_driver = {
	.id_table = s5k6a3yx_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k6a3yx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

static int s5k6a3yx_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CAM_DEBUG("E");
	CAM_DEBUG("s5k6a3yx_sensor_power_up(1) : i2c_scl: %d, i2c_sda: %d\n",
		 gpio_get_value(85), gpio_get_value(84));


	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0)
		pr_err("%s: request gpio failed\n", __func__);

	CAM_DEBUG("s5k6a3yx_sensor_power_up(2) : i2c_scl: %d, i2c_sda: %d\n",
		 gpio_get_value(85), gpio_get_value(84));

	/* Power on */
	data->sensor_platform_info->sensor_power_on();

	/* MCLK */
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0)
		pr_err("%s: clk enable failed\n", __func__);

	mdelay(5);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);
	mdelay(5);

	data->sensor_platform_info->sensor_power_on_sub();

	CAM_DEBUG("s5k6a3yx_sensor_power_up(3) : i2c_scl: %d, i2c_sda: %d\n",
		 gpio_get_value(85), gpio_get_value(84));

	mdelay(5);
	CAM_DEBUG("X");

	return rc;
}

static int s5k6a3yx_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CAM_DEBUG("E");

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	/* MCLK */
	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);

	/* VT_CAM_RESET */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 0);

	/* STBY */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->stby, 0);

	/* Power off */
	data->sensor_platform_info->sensor_power_off();

	msm_camera_request_gpio_table(data, 0);
	s_ctrl->vision_mode_flag=0;

	CAM_DEBUG("X");

	return rc;
}

/* Switch to low power vision mode */
static int s5k6a3yx_sensor_set_streaming_mode(
		struct msm_sensor_ctrl_t *s_ctrl, int32_t vision_mode_enable) {
	int rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

		CAM_DEBUG("stop streaming");
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	CAM_DEBUG("vision_mode_enable=%d: E", vision_mode_enable);
	if(vision_mode_enable) { /*switch from normal/dual to vision mode */
		CAM_DEBUG("set X_SHUTDOWN pin to low");
		data->sensor_platform_info->
			sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 0);
		usleep(1100);
		CAM_DEBUG("set VIS_STBY pin to high");
		data->sensor_platform_info->
			sensor_pmic_gpio_ctrl(data->sensor_platform_info->stby, 1);

		CAM_DEBUG("change stream config arrays");
		s_ctrl->msm_sensor_reg->start_stream_conf = s5k6a3yx_start_settings_vision;
		s_ctrl->msm_sensor_reg->start_stream_conf_size = ARRAY_SIZE(s5k6a3yx_start_settings_vision);
		s_ctrl->msm_sensor_reg->stop_stream_conf = s5k6a3yx_stop_settings_vision;
		s_ctrl->msm_sensor_reg->stop_stream_conf_size = ARRAY_SIZE(s5k6a3yx_stop_settings_vision);
		s_ctrl->vision_mode_flag = 1;
                /*set FPS based on flag set from user space driver*/
                if (vision_mode_enable == VISION_MODE_SET_FPS_5)
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
							VISION_MODE_SET_FPS_ADDR,
							VISION_MODE_FPS_5_VAL,
							MSM_CAMERA_I2C_BYTE_DATA);
		else if (vision_mode_enable == VISION_MODE_SET_FPS_10)	
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
							VISION_MODE_SET_FPS_ADDR,
							VISION_MODE_FPS_10_VAL,
							MSM_CAMERA_I2C_BYTE_DATA);


	} else { /*switch from vision to normal/dual mode */
		CAM_DEBUG("set VIS_STBY pin to low");
		data->sensor_platform_info->
			sensor_pmic_gpio_ctrl(data->sensor_platform_info->stby, 0);
		usleep(1100);
		CAM_DEBUG("set X_SHUTDOWN pin to high");
		data->sensor_platform_info->
			sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 1);

		CAM_DEBUG("change stream config arrays");
		s_ctrl->msm_sensor_reg->start_stream_conf = s5k6a3yx_start_settings;
		s_ctrl->msm_sensor_reg->start_stream_conf_size = ARRAY_SIZE(s5k6a3yx_start_settings);
		s_ctrl->msm_sensor_reg->stop_stream_conf = s5k6a3yx_stop_settings;
		s_ctrl->msm_sensor_reg->stop_stream_conf_size = ARRAY_SIZE(s5k6a3yx_stop_settings);
		s_ctrl->vision_mode_flag = 0;
	}
	CAM_DEBUG("rc=%d : X", rc);
	return rc;
}

/*
 * Change normal/backlight AEC for vision mode
 * aec_mode:
 * 	0 = normal
 * 	1 = backlight
 */
int s5k6a3yx_sensor_set_vision_ae_control(
				struct msm_sensor_ctrl_t *s_ctrl, int ae_mode) {

	if(s_ctrl->vision_mode_flag == 0) {
		cam_err("Error: sensor not in vision mode, cannot set AE.");
		return -1;
	}
	if(ae_mode == 0) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
							VISION_MODE_AE_REG_ADDR,
							VISION_MODE_AE_NORMAL,
							MSM_CAMERA_I2C_BYTE_DATA);
		CAM_DEBUG("normal mode AEC set");
	} else {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
							VISION_MODE_AE_REG_ADDR,
							VISION_MODE_AE_BACKLIGHT,
							MSM_CAMERA_I2C_BYTE_DATA);
		CAM_DEBUG("backlight mode AEC set");
	}
	return 0;
}

static ssize_t s5k6a3yx_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "s5k6a3yx");
}

static ssize_t s5k6a3yx_camera_fw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", "s5k6a3yx", "N");
}

static DEVICE_ATTR(front_camtype, S_IRUGO, s5k6a3yx_camera_type_show, NULL);
static DEVICE_ATTR(front_camfw, S_IRUGO, s5k6a3yx_camera_fw_show, NULL);

static int __init s5k6a3yx_sensor_init_module(void)
{
	struct device *cam_dev_front = NULL;

	cam_dev_front =
	device_create(camera_class, NULL, 0, NULL, "front");
	if (IS_ERR(cam_dev_front)) {
		cam_err("failed to create device cam_dev_front!\n");
		return 0;
	}

	if (device_create_file
	(cam_dev_front, &dev_attr_front_camtype) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_front_camtype.attr.name);
	}

	if (device_create_file
	(cam_dev_front, &dev_attr_front_camfw) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_front_camfw.attr.name);
	}

	return i2c_add_driver(&s5k6a3yx_i2c_driver);
}


static void s5k6a3yx_write_exp_params(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t fl_lines,
	uint32_t line)
{

uint8_t msb_fl_lines, lsb_fl_lines;
uint8_t msb_line, lsb_line;
uint8_t msb_gain, lsb_gain;

	CDBG("%s gain %d fl %d line %d\n",
		__func__,
		gain,
		fl_lines,
		line);
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);


	msb_fl_lines = (uint8_t)((fl_lines >> 8) & 0xFF);
	lsb_fl_lines = (uint8_t)(fl_lines & 0xFF);

	msb_line = (uint8_t)((line >> 8) & 0xFF);
	lsb_line = (uint8_t)(line & 0xFF);

	msb_gain = (uint8_t)((gain >> 8) & 0xFF);
	lsb_gain = (uint8_t)(gain & 0xFF);

	CDBG("%s : %d %d %d %d %d %d\n",
		   __func__, msb_fl_lines,  lsb_fl_lines, msb_line
		   , lsb_line, msb_gain, lsb_gain);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines,
		msb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines+1,
		lsb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, msb_line,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, lsb_line,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, msb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr+1, lsb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
}


static int32_t s5k6a3yx_write_exp_gain(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t line)
{
	int rc = 0;
	uint32_t fl_lines = 0;
	uint8_t offset;

	CDBG("%s E gain %d line %d\n", __func__, gain, line);

		fl_lines = s_ctrl->curr_frame_length_lines;
		fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
		offset = s_ctrl->sensor_exp_gain_info->vert_offset;

		if (line > (fl_lines - offset))
			fl_lines = line + offset;

			s5k6a3yx_write_exp_params(s_ctrl, gain, fl_lines, line);

	return rc;
}


static int32_t s5k6a3yx_write_snapshot_exp_gain(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t line)
{
	int rc = 0;
	uint32_t fl_lines = 0;
	uint8_t offset;

	CDBG("%s E gain %d line %d\n", __func__, gain, line);

	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;

	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s5k6a3yx_write_exp_params(s_ctrl, gain, fl_lines, line);

	return rc;
}



static struct v4l2_subdev_core_ops s5k6a3yx_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k6a3yx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k6a3yx_subdev_ops = {
	.core = &s5k6a3yx_subdev_core_ops,
	.video  = &s5k6a3yx_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k6a3yx_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = s5k6a3yx_write_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k6a3yx_write_snapshot_exp_gain, //msm_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k6a3yx_sensor_power_up,
	.sensor_power_down = s5k6a3yx_sensor_power_down,
	.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines1,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_set_streaming_mode = s5k6a3yx_sensor_set_streaming_mode,
	.sensor_set_vision_ae_control = s5k6a3yx_sensor_set_vision_ae_control,
};

static struct msm_sensor_reg_t s5k6a3yx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k6a3yx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k6a3yx_start_settings),
	.stop_stream_conf = s5k6a3yx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k6a3yx_stop_settings),
	.group_hold_on_conf = s5k6a3yx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k6a3yx_groupon_settings),
	.group_hold_off_conf = s5k6a3yx_groupoff_settings,
	.group_hold_off_conf_size = ARRAY_SIZE(s5k6a3yx_groupoff_settings),
	.init_settings = &s5k6a3yx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k6a3yx_init_conf),
	.mode_settings = &s5k6a3yx_confs[0],
	.output_settings = &s5k6a3yx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k6a3yx_confs),
};

static struct msm_sensor_ctrl_t s5k6a3yx_s_ctrl = {
	.msm_sensor_reg = &s5k6a3yx_regs,
	.sensor_i2c_client = &s5k6a3yx_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.vreg_seq = s5k6a3yx_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(s5k6a3yx_veg_seq),
	.sensor_output_reg_addr = &s5k6a3yx_reg_addr,
	.sensor_id_info = &s5k6a3yx_id_info,
	.sensor_exp_gain_info = &s5k6a3yx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.msm_sensor_mutex = &s5k6a3yx_mut,
	.sensor_i2c_driver = &s5k6a3yx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k6a3yx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k6a3yx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k6a3yx_subdev_ops,
	.func_tbl = &s5k6a3yx_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
	.vision_mode_flag = 0,
};

module_init(s5k6a3yx_sensor_init_module);
MODULE_DESCRIPTION("Samsung 2MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
