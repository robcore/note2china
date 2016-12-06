/*
 * linux/arch/arm/mach-exynos/midas-wacom.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>

#include <linux/err.h>
#include <linux/gpio.h>

#include <linux/wacom_i2c.h>
#include <linux/barcode_emul.h>
#include <linux/regulator/consumer.h>
#include <mach/rpm-regulator.h>
//#include <plat/gpio-cfg.h>

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
#include <linux/cpufreq.h>
#endif


static struct wacom_g5_callbacks *wacom_callbacks;

int wacom_vdd_on(bool on)
{
	int ret;
	static struct regulator *wacom_reg;

	if (!wacom_reg) {
		wacom_reg = regulator_get(NULL, "8921_l16");
		if (IS_ERR(wacom_reg)){
			pr_info("[E-PEN] wacom power error:%d\n", on); 			
			return PTR_ERR(wacom_reg);
		}
		ret = regulator_set_voltage(wacom_reg, 3000000, 3000000);
		if (ret) {
			pr_info("[E-PEN] wacom voltage set error:%d\n", ret);			
			return ret;
		}
	}

	pr_info("[E-PEN] wacom power onoff:%d\n", on); 
	if (on) {
		if (!regulator_is_enabled(wacom_reg)) {
			ret = regulator_enable(wacom_reg);
			if (ret)
				pr_info("[E-PEN] wacom enable l16 failed:%d\n", ret);
			else
				pr_info("[E-PEN] wacom enable:%d\n", __LINE__);
		}
	} else {
		if (regulator_is_enabled(wacom_reg)) {
			ret = regulator_disable(wacom_reg);
			if (ret)
				pr_info("[E-PEN] wacom disable l16 failed:%d\n", ret);
			else
				pr_info("[E-PEN] wacom disable:%d\n", __LINE__);
		}
	}
	//regulator_put(wacom_reg);

	return 0;
}

static int wacom_early_suspend_hw(void)
{
#ifdef CONFIG_SEC_FPGA
	ice_gpiox_set(GPIO_PEN_RESET_N,0);
#else
	gpio_set_value(GPIO_PEN_RESET_N, 0);
#endif

	wacom_vdd_on(0);

	return 0;
}

static int wacom_late_resume_hw(void)
{
	wacom_vdd_on(1);
	msleep(100);

#ifdef CONFIG_SEC_FPGA
	ice_gpiox_set(GPIO_PEN_RESET_N,1);
#else
	gpio_set_value(GPIO_PEN_RESET_N, 1);
#endif

	return 0;
}

static int wacom_suspend_hw(void)
{
	return wacom_early_suspend_hw();
}

static int wacom_resume_hw(void)
{
	return wacom_late_resume_hw();
}

static int wacom_reset_hw(void)
{
	wacom_early_suspend_hw();
	msleep(100);
	wacom_late_resume_hw();

	return 0;
}

static void wacom_register_callbacks(struct wacom_g5_callbacks *cb)
{
	wacom_callbacks = cb;
};

#ifdef WACOM_HAVE_FWE_PIN
static void wacom_compulsory_flash_mode(bool en)
{
#ifdef CONFIG_SEC_FPGA
	ice_gpiox_set(GPIO_PEN_FWE1,en);
#else
	gpio_set_value(GPIO_PEN_FWE1, en);
#endif
}
#endif


static struct wacom_g5_platform_data wacom_platform_data = {
	.x_invert = 1,
	.y_invert = 0,
	.xy_switch = 1,
	.min_x = 0,
	.max_x = WACOM_POSX_MAX,
	.min_y = 0,
	.max_y = WACOM_POSY_MAX,
	.min_pressure = 0,
	.max_pressure = WACOM_PRESSURE_MAX,
	.gpio_pendct = GPIO_PEN_PDCT,
	/*.init_platform_hw = midas_wacom_init,*/
	/*      .exit_platform_hw =,    */
	.suspend_platform_hw = wacom_suspend_hw,
	.resume_platform_hw = wacom_resume_hw,
	.early_suspend_platform_hw = wacom_early_suspend_hw,
	.late_resume_platform_hw = wacom_late_resume_hw,
	.reset_platform_hw = wacom_reset_hw,
	.register_cb = wacom_register_callbacks,
#ifdef WACOM_HAVE_FWE_PIN
	.compulsory_flash_mode = wacom_compulsory_flash_mode,
#endif
#ifdef WACOM_PEN_DETECT
	.gpio_pen_insert = GPIO_WACOM_SENSE,
#endif
};

/* I2C5 */
static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("wacom_g5sp_i2c", 0x56),
			.platform_data = &wacom_platform_data,
	},
};

void __init midas_wacom_init(void)
{
	int gpio;
	
	/*RESET*/
#ifdef CONFIG_SEC_FPGA
	ice_gpiox_set(GPIO_PEN_RESET_N,0);
#else
	gpio = GPIO_PEN_RESET_N;
	gpio_request(gpio, "PEN_RESET");
	gpio_direction_output(gpio, 0);
#endif


	/*FWE1*/
#ifdef CONFIG_SEC_FPGA
	ice_gpiox_set(GPIO_PEN_FWE1,0);
#else
	printk(KERN_INFO "[E-PEN] Use FWE\n");
	gpio = GPIO_PEN_FWE1;
	gpio_request(gpio, "PEN_FWE1");
	gpio_direction_output(gpio, 0);
#endif


	/*PDCT*/
	gpio = GPIO_PEN_PDCT;
	gpio_request(gpio, "PEN_PDCT");
	gpio_direction_input(gpio);

	irq_set_irq_type(gpio_to_irq(gpio), IRQ_TYPE_EDGE_BOTH);


	/*IRQ*/
	gpio = GPIO_PEN_IRQ;
	gpio_request(gpio, "PEN_IRQ");
	gpio_direction_input(gpio);

	i2c_devs5[0].irq = gpio_to_irq(gpio);
	irq_set_irq_type(i2c_devs5[0].irq, IRQ_TYPE_EDGE_RISING);


	wacom_vdd_on(0);

	i2c_register_board_info(0, i2c_devs5, ARRAY_SIZE(i2c_devs5));


	printk(KERN_INFO "[E-PEN] : wacom IC initialized.\n");
}
