/*
 * linux/arch/arm/mach-exynos/midas-tsp.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/keyreset.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <asm/mach-types.h>
#include <linux/regulator/consumer.h>
#include "board-8064.h"
#include <linux/delay.h>
#include <mach/apq8064-gpio.h>


#include <linux/err.h>

#include <linux/platform_data/mms152_ts.h>
//#include <plat/gpio-cfg.h>

#define NEW_GPIO_TOUCH_IRQ	67
#define GPIO_TOUCH_IRQ		57

/* touch is on i2c3 */
#define GPIO_TOUCH_SDA		8
#define GPIO_TOUCH_SCL		9



/* MELFAS TSP(T0) */
#if 0
static bool enabled;
int TSP_VDD_18V(int on)
{
	struct regulator *regulator;

	if (enabled == on)
		return 0;

	regulator = regulator_get(NULL, "touch_1.8v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on) {
		regulator_enable(regulator);
		/*printk(KERN_INFO "[TSP] melfas power on\n"); */
	} else {
		/*
		 * TODO: If there is a case the regulator must be disabled
		 * (e,g firmware update?), consider regulator_force_disable.
		 */
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	enabled = on;
	regulator_put(regulator);

	return 0;
}
#endif

int melfas_power(bool enable)
{
	int ret = 0;
	/* 3.3V */
	static struct regulator *reg_l17;

	/* 1.8V */
	static struct regulator *reg_l22;

pr_err( "[TSP] %s : %d \n", __func__,__LINE__);

	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8921_l17");
		if (IS_ERR(reg_l17)) {
			printk(KERN_ERR "%s: could not get 8921_l17, rc = %ld\n",
					__func__, PTR_ERR(reg_l17));
			return 1;
		}
		ret = regulator_set_voltage(reg_l17, 3300000, 3300000);
		if (ret) {
			printk(KERN_ERR "%s: unable to set ldo17 voltage to 3.3V\n",
					__func__);
			return 1;
		}
	}

	if (!reg_l22) {
		reg_l22 = regulator_get(NULL, "8921_l22");
		if (IS_ERR(reg_l22)) {
			printk(KERN_ERR "%s: could not get 8921_l22, rc = %ld\n",
					__func__, PTR_ERR(reg_l22));
			return 1;
		}
	}
	ret = regulator_set_voltage(reg_l22, 1800000, 1800000);
	if (ret) {
		printk(KERN_ERR"%s: unable to set ldo22 voltage to 1.8V\n",
				__func__);
		return 1;
	}

	if (enable) {
		if (!regulator_is_enabled(reg_l17)) {
			ret = regulator_enable(reg_l17);
			if (ret)
				printk(KERN_ERR "%s: enable l17 failed, rc=%d\n",
						__func__, ret);
			else
				printk(KERN_INFO "%s: enable l17\n", __func__);
		}

		if (!regulator_is_enabled(reg_l22)) {
			ret = regulator_enable(reg_l22);
			if (ret)
				printk(KERN_ERR "%s: enable l22 failed, rc=%d\n",
						__func__, ret);
			else
				printk(KERN_INFO "%s: enable l22\n", __func__);
		}

//		msleep(touch_sleep_time);

	} else {
		if (regulator_is_enabled(reg_l17)) {
			ret = regulator_disable(reg_l17);
			if (ret)
				printk(KERN_ERR "%s: disable l17 failed, rc=%d\n",
						__func__, ret);
			else
				printk(KERN_INFO "%s: disable l17\n", __func__);
		}

		if (regulator_is_enabled(reg_l22)) {
			ret = regulator_disable(reg_l22);
			if (ret)
				printk(KERN_ERR "%s: disable l22 failed, rc=%d\n",
						__func__, ret);
			else
				printk(KERN_INFO "%s: disable l22\n", __func__);
		}
	}
pr_err( "[TSP] %s : %d \n", __func__,__LINE__);
	return 0;
}

int is_melfas_vdd_on(void)
{
	int ret;
	/* 3.3V */
	static struct regulator *regulator;

	if (!regulator) {
		regulator = regulator_get(NULL, "reg_l17");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */

	if (to_gpios) {
		if (gpio_request(GPIO_TOUCH_SCL, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TOUCH_SDA, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(NEW_GPIO_TOUCH_IRQ, 0);
		//s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		//s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TOUCH_SCL, 0);
		//s3c_gpio_cfgpin(GPIO_TOUCH_SCL, S3C_GPIO_OUTPUT);
		//s3c_gpio_setpull(GPIO_TOUCH_SCL, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TOUCH_SDA, 0);
		//s3c_gpio_cfgpin(GPIO_TOUCH_SDA, S3C_GPIO_OUTPUT);
		//s3c_gpio_setpull(GPIO_TOUCH_SDA, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(NEW_GPIO_TOUCH_IRQ, 1);
		gpio_direction_input(NEW_GPIO_TOUCH_IRQ);
		//s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		//s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TOUCH_SCL, 1);
		gpio_direction_input(GPIO_TOUCH_SCL);
		//s3c_gpio_cfgpin(GPIO_TOUCH_SCL, S3C_GPIO_SFN(3));
		//s3c_gpio_setpull(GPIO_TOUCH_SCL, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TOUCH_SDA, 1);
		gpio_direction_input(GPIO_TOUCH_SDA);
		//s3c_gpio_cfgpin(GPIO_TOUCH_SDA, S3C_GPIO_SFN(3));
		//s3c_gpio_setpull(GPIO_TOUCH_SDA, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TOUCH_SCL);
		gpio_free(GPIO_TOUCH_SDA);
	}
	return 0;
}

#if 0
void melfas_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void melfas_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

#endif

int get_lcd_type;
void __init midas_tsp_set_lcdtype(int lcd_type)
{
	get_lcd_type = lcd_type;
}

int melfas_get_lcdtype(void)
{
	return get_lcd_type;
}
struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool, int);
};


void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en, 1);	// tsp mode
}


static void melfas_register_callback(void *cb)
{
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 720,
	.max_y = 1280,
#if 0
	.invert_x = 720,
	.invert_y = 1280,
#else
	.invert_x = 0,
	.invert_y = 0,
#endif
	.gpio_int = NEW_GPIO_TOUCH_IRQ,
	.gpio_scl = GPIO_TOUCH_SCL,
	.gpio_sda = GPIO_TOUCH_SDA,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.is_vdd_on = is_melfas_vdd_on,
	.config_fw_version = "N7100_Me_0910",
/*	.set_touch_i2c		= melfas_set_touch_i2c, */
/*	.set_touch_i2c_to_gpio	= melfas_set_touch_i2c_to_gpio, */
	.lcd_type = melfas_get_lcdtype,
	.register_cb = melfas_register_callback,
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init midas_tsp_init(void)
{
	int gpio;
	int ret;
	pr_err("[TSP] midas_tsp_init() is called\n");

	/* TSP_INT: XEINT_4 */
	gpio = NEW_GPIO_TOUCH_IRQ;
	ret = gpio_request(gpio, "tsp_int");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");

	i2c_devs3[0].irq = MSM_GPIO_TO_INT( NEW_GPIO_TOUCH_IRQ );

	gpio_tlmm_config(GPIO_CFG(NEW_GPIO_TOUCH_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	i2c_register_board_info(APQ_8064_GSBI3_QUP_I2C_BUS_ID, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}
