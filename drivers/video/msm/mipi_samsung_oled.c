/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/lcd.h>

#include "msm_fb.h"
#include "msm_fb_panel.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include "mipi_samsung_define.h"
#include "mipi_samsung_oled.h"

#ifdef CONFIG_SAMSUNG_CMC624
#include "samsung_cmc624.h"
#endif
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#include "mipi_samsung_esd_refresh.h"
#endif

#if defined(CONFIG_FB_MDP4_ENHANCE)
#include "mdp4_video_enhance.h"
#endif

struct mipi_samsung_driver_data msd;

#define WA_FOR_FACTORY_MODE

unsigned char bypass_lcd_id;
unsigned char lcd_type;
int is_lcd_connected = 1;
struct mutex dsi_tx_mutex;

#ifdef USE_READ_ID
static char manufacture_id1[2] = {0xDA, 0x00}; /* DTYPE_DCS_READ */
static char manufacture_id2[2] = {0xDB, 0x00}; /* DTYPE_DCS_READ */
static char manufacture_id3[2] = {0xDC, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc samsung_manufacture_id1_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id1), manufacture_id1};
static struct dsi_cmd_desc samsung_manufacture_id2_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id2), manufacture_id2};
static struct dsi_cmd_desc samsung_manufacture_id3_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id3), manufacture_id3};

extern int smart_dimming_init(struct msm_fb_data_type *mfd, struct mipi_panel_data *pmpd);
extern int smart_dimming_init_ea8061(struct msm_fb_data_type *mfd, struct mipi_panel_data *pmpd);

static uint32 mipi_samsung_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 id;

	mutex_lock(&dsi_tx_mutex);
	
	tp = &msd.samsung_tx_buf;
	rp = &msd.samsung_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &samsung_manufacture_id1_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 1);
	pr_info("%s: manufacture_id1=%x\n", __func__, *rp->data);
	msd.mpd->smart.panelid[0] = 0xFF&*((uint32 *)rp->data);
	id = msd.mpd->smart.panelid[0];
	id <<= 8;

	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	cmd = &samsung_manufacture_id2_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 1);
	pr_info("%s: manufacture_id2=%x\n", __func__, *rp->data);
	bypass_lcd_id = *rp->data;
	msd.mpd->smart.panelid[1] = 0xFF&*((uint32 *)rp->data);
	id |= msd.mpd->smart.panelid[1];
	id <<= 8;

	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	cmd = &samsung_manufacture_id3_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 1);
	pr_info("%s: manufacture_id3=%x\n", __func__, *rp->data);
	msd.mpd->smart.panelid[2] = 0xFF&*((uint32 *)rp->data);
	id |= msd.mpd->smart.panelid[2];

	mutex_unlock(&dsi_tx_mutex);
	
	pr_info("%s: manufacture_id=%x\n", __func__, id);

#ifdef FACTORY_TEST
	if (id == 0x00) {
		pr_info("Lcd is not connected\n");
		is_lcd_connected = 0;
	}
#endif
	return id;
}
#endif

static int __init lcd_read_id(char *param)
{
	if(param[0]=='4'&&param[1]=='2'&&param[2]=='1'&&param[3]=='6'&&param[4]=='A'&&param[5]=='2') {
		lcd_type=LDI_TYPE_LSI;
		printk("[LCD] LSI DDI\n");
	} else {
		lcd_type=LDI_TYPE_MAGNA;
		printk("[LCD] MAGNA DDI\n");
	}
	return 0;
}
__setup("LCDID=", lcd_read_id);

unsigned char bypass_LCD_Id(void){
	return bypass_lcd_id;
}

unsigned char LCD_ID3(void){
	return 0;
}
unsigned char LCD_Get_Value(void){
	return 0x20;
}
EXPORT_SYMBOL(LCD_Get_Value);

bool samsung_has_cmc624(void)
{
	return false;
}
EXPORT_SYMBOL(samsung_has_cmc624);

static int mipi_samsung_disp_send_cmd(struct msm_fb_data_type *mfd,
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock)
{
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;

	printk(KERN_INFO
		"[lcd] mipi_samsung_disp_send_cmd start : %d\n", (int)cmd);

	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_lock(&dsi_tx_mutex);
	else {
		if (lock)
			mutex_lock(&mfd->dma->ov_mutex);
	}

	switch (cmd) {
		case PANEL_READY_TO_READ_MTP:
			cmd_desc = msd.mpd->ready_to_read_mtp.cmd;
			cmd_size = msd.mpd->ready_to_read_mtp.size;
			break;
		case PANEL_READY_TO_ON:
			if(lcd_type == LDI_TYPE_MAGNA) {
				cmd_desc = msd.mpd->ready_to_on_magna.cmd;
				cmd_size = msd.mpd->ready_to_on_magna.size;
			} else {
				cmd_desc = msd.mpd->ready_to_on_lsi.cmd;
				cmd_size = msd.mpd->ready_to_on_lsi.size;
			}
			break;
		case PANEL_READY_TO_ON2:
			if(lcd_type == LDI_TYPE_MAGNA) {
				cmd_desc = msd.mpd->ready_to_on2_magna.cmd;
				cmd_size = msd.mpd->ready_to_on2_magna.size;
			} else {
				cmd_desc = msd.mpd->ready_to_on2_lsi.cmd;
				cmd_size = msd.mpd->ready_to_on2_lsi.size;
			}
			break;
		case PANEL_READY_TO_OFF:
			cmd_desc = msd.mpd->ready_to_off.cmd;
			cmd_size = msd.mpd->ready_to_off.size;
			break;
		case PANEL_ON:
			cmd_desc = msd.mpd->on.cmd;
			cmd_size = msd.mpd->on.size;
			break;
		case PANEL_OFF:
			cmd_desc = msd.mpd->off.cmd;
			cmd_size = msd.mpd->off.size;
			break;
		case PANEL_EARLY_OFF:
			cmd_desc = msd.mpd->early_off.cmd;
			cmd_size = msd.mpd->early_off.size;
			break;
		case PANEL_GAMMA_UPDATE:
			cmd_desc = msd.mpd->gamma_update.cmd;
			cmd_size = msd.mpd->gamma_update.size;
			break;
		case MTP_READ_ENABLE:
			cmd_desc = msd.mpd->mtp_read_enable.cmd;
			cmd_size = msd.mpd->mtp_read_enable.size;
			break;
		case PANEL_ACL_ON:
			cmd_desc = msd.mpd->acl_on.cmd;
			cmd_size = msd.mpd->acl_on.size;
			msd.mpd->ldi_acl_stat = true;
			break;
		case PANEL_ACL_OFF:
			cmd_desc = msd.mpd->acl_off.cmd;
			cmd_size = msd.mpd->acl_off.size;
			msd.mpd->ldi_acl_stat = false;
			break;
		case PANEL_ELVSS_UPDATE:
			msd.mpd->set_elvss(mfd->bl_level, lcd_type);
			cmd_desc = msd.mpd->elvss_update.cmd;
			cmd_size = msd.mpd->elvss_update.size;
			break;
		case PANEL_AID_CTRL:
			cmd_desc = msd.mpd->aid_ctrl.cmd;
			cmd_size = msd.mpd->aid_ctrl.size;
			break;
		case PANEL_BRIGHT_CTRL:
			msd.mpd->prepare_brightness_control_cmd_array(msd.mpd->smart_s6e8aa0x01.gen_table, mfd->bl_level, lcd_type);
			if(lcd_type == LDI_TYPE_MAGNA) {
				cmd_desc = msd.mpd->backlight_ctrl_magna.cmd;
				cmd_size = msd.mpd->backlight_ctrl_magna.size;
			} else {
				cmd_desc = msd.mpd->backlight_ctrl_lsi.cmd;
				cmd_size = msd.mpd->backlight_ctrl_lsi.size;
			}
			break;
		default:
			goto unknown_command;
	}

	if (!cmd_size)
		goto unknown_command;

	mipi_dsi_cmds_tx(&msd.samsung_tx_buf, cmd_desc, cmd_size);
	
	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_unlock(&dsi_tx_mutex);
	else {
		if (lock)
			mutex_unlock(&mfd->dma->ov_mutex);
	}

	printk(KERN_INFO "[lcd] mipi_samsung_disp_send_cmd end\n");

	return 0;

unknown_command:
	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_unlock(&dsi_tx_mutex);
	else {
		if (lock)
			mutex_unlock(&mfd->dma->ov_mutex);
	}

	return 0;
}

static void mipi_samsung_disp_backlight(struct msm_fb_data_type *mfd)
{
	printk("[lcd] mipi_samsung_disp_backlight : %d\n", mfd->bl_level);

	mipi_samsung_disp_send_cmd(mfd, PANEL_BRIGHT_CTRL, true);

	if(lcd_type == LDI_TYPE_MAGNA) {
		mipi_samsung_disp_send_cmd(mfd, PANEL_AID_CTRL, true);
	}
	
	if(msd.dstat.acl_on) {
		if (msd.mpd->set_acl(mfd->bl_level)) {
			mipi_samsung_disp_send_cmd(mfd, PANEL_ACL_OFF, true);
		} else {
			mipi_samsung_disp_send_cmd(mfd, PANEL_ACL_ON, true);
		}
	}

	mipi_samsung_disp_send_cmd(mfd, PANEL_ELVSS_UPDATE, true);

	return;
}

static int mipi_samsung_ldi_init(struct msm_fb_data_type *mfd)
{
	mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_ON, false);
//	mipi_samsung_disp_send_cmd(mfd, PANEL_BRIGHT_CTRL, false);
	mipi_samsung_disp_backlight(mfd);
	mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_ON2, false);
	mipi_samsung_disp_send_cmd(mfd, PANEL_ON, false);

	return 0;
}

static int mipi_samsung_disp_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	printk(KERN_INFO "[lcd] mipi_samsung_disp_on start\n");

	mfd = platform_get_drvdata(pdev);
	if (unlikely(!mfd))
		return -ENODEV;
	if (unlikely(mfd->key != MFD_KEY))
		return -EINVAL;

	mipi = &mfd->panel_info.mipi;

	if (!msd.dstat.is_smart_dim_loaded) {
		/*Load MTP Data*/
		struct SMART_DIM *psmart;

#ifdef USE_READ_ID
		msd.mpd->manufacture_id = mipi_samsung_manufacture_id(mfd);
		if(msd.mpd->smart.panelid[2] == 0xA2) {
			lcd_type = LDI_TYPE_LSI;
		} else {
			lcd_type = LDI_TYPE_MAGNA;
		}
#endif
		psmart = &(msd.mpd->smart_s6e8aa0x01);

		psmart->plux_table = msd.mpd->lux_table;
		psmart->lux_table_max = msd.mpd->lux_table_max_cnt;

		if(lcd_type == LDI_TYPE_MAGNA) {
			mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_READ_MTP, false);
			smart_dimming_init_ea8061(mfd, msd.mpd);
		} else {
			smart_dimming_init(mfd, msd.mpd);
		}
		
		msd.dstat.is_smart_dim_loaded = true;
		msd.dstat.gamma_mode = GAMMA_SMART;
	}

	msd.mpd->prepare_brightness_control_cmd_array(
		msd.mpd->smart_s6e8aa0x01.gen_table, mfd->bl_level, lcd_type);
	
	mipi_samsung_ldi_init(mfd);

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
	set_esd_enable();
#endif
	printk(KERN_INFO "[lcd] mipi_samsung_disp_on end : lcd type=%d\n", lcd_type);

	return 0;
}

static int mipi_samsung_disp_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	printk(KERN_INFO "[lcd] mipi_samsung_disp_off start\n");

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
	set_esd_disable();
#endif
	mfd = platform_get_drvdata(pdev);
	if (unlikely(!mfd))
		return -ENODEV;
	if (unlikely(mfd->key != MFD_KEY))
		return -EINVAL;

	mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_OFF, false);
	mipi_samsung_disp_send_cmd(mfd, PANEL_OFF, false);
	msd.mpd->ldi_acl_stat = false;
	printk(KERN_INFO "[lcd] mipi_samsung_disp_off end\n");

	return 0;
}

static void __devinit mipi_samsung_disp_shutdown(struct platform_device *pdev)
{
	static struct mipi_dsi_platform_data *mipi_dsi_pdata;

	if (pdev->id != 0)
		return;

	mipi_dsi_pdata = pdev->dev.platform_data;
	if (mipi_dsi_pdata == NULL) {
		pr_err("LCD Power off failure: No Platform Data\n");
		return;
	}
	/* Power off Seq:2: Sleepout
			mode->ResetLow -> delay 120ms->VCI,VDD off */
	if (mipi_dsi_pdata &&
		 mipi_dsi_pdata->
			dsi_power_save) {
					/* [junesok] lcd_rst_down() is not
					the member of mipi_dsi_platform_data */
					/*mipi_dsi_pdata->lcd_rst_down();*/

						mdelay(120);
						pr_info("LCD POWER OFF\n");

					}
}
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
void set_esd_refresh(boolean stat)
{
	msd.esd_refresh = stat;
}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void mipi_samsung_disp_early_suspend(struct early_suspend *h)
{
	struct msm_fb_data_type *mfd;
	pr_info("[lcd] %s\n", __func__);

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
	set_esd_disable();
#endif

	mfd = platform_get_drvdata(msd.msm_pdev);
	if (unlikely(!mfd)) {
		pr_info("%s NO PDEV.\n", __func__);
		return;
	}
	if (unlikely(mfd->key != MFD_KEY)) {
		pr_info("%s MFD_KEY is not matched.\n", __func__);
		return;
	}
//	mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_OFF, false);
//	mipi_samsung_disp_send_cmd(mfd, PANEL_OFF, false);
	msd.mpd->ldi_acl_stat = false;
//	mipi_samsung_disp_send_cmd(mfd, PANEL_EARLY_OFF, true);
	mfd->resume_state = MIPI_SUSPEND_STATE;
}

static void mipi_samsung_disp_late_resume(struct early_suspend *h)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(msd.msm_pdev);
	if (unlikely(!mfd)) {
		pr_info("%s NO PDEV.\n", __func__);
		return;
	}
	if (unlikely(mfd->key != MFD_KEY)) {
		pr_info("%s MFD_KEY is not matched.\n", __func__);
		return;
	}

	mfd->resume_state = MIPI_RESUME_STATE;
//	mipi_samsung_disp_send_cmd(mfd, PANEL_READY_TO_ON, false);
//	mipi_samsung_disp_send_cmd(mfd, PANEL_ON, false);

	pr_info("[lcd] %s\n", __func__);
}
#endif

#if defined(CONFIG_LCD_CLASS_DEVICE)
#ifdef WA_FOR_FACTORY_MODE
static ssize_t mipi_samsung_disp_get_power(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct msm_fb_data_type *mfd;
	int rc;

	mfd = platform_get_drvdata(msd.msm_pdev);
	if (unlikely(!mfd))
		return -ENODEV;
	if (unlikely(mfd->key != MFD_KEY))
		return -EINVAL;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", mfd->panel_power_on);
	pr_info("mipi_samsung_disp_get_power(%d)\n", mfd->panel_power_on);

	return rc;
}

static ssize_t mipi_samsung_disp_set_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd;
	unsigned int power;

	mfd = platform_get_drvdata(msd.msm_pdev);

	if (sscanf(buf, "%1u", &power) != 1)
		return -EINVAL;

	if (power == mfd->panel_power_on)
		return 0;

	if (power) {
		mfd->fbi->fbops->fb_blank(FB_BLANK_UNBLANK, mfd->fbi);
		mfd->fbi->fbops->fb_pan_display(&mfd->fbi->var, mfd->fbi);
		mipi_samsung_disp_send_cmd(mfd, PANEL_LATE_ON, true);
		mipi_samsung_disp_backlight(mfd);
	} else {
		mfd->fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, mfd->fbi);
	}

	pr_info("mipi_samsung_disp_set_power\n");

	return size;
}
#else
static int mipi_samsung_disp_get_power(struct lcd_device *dev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(msd.msm_pdev);
	if (unlikely(!mfd))
		return -ENODEV;
	if (unlikely(mfd->key != MFD_KEY))
		return -EINVAL;

	pr_info("mipi_samsung_disp_get_power(%d)\n", mfd->panel_power_on);

	return mfd->panel_power_on;
}

static int mipi_samsung_disp_set_power(struct lcd_device *dev, int power)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(msd.msm_pdev);

	if (power == mfd->panel_power_on)
		return 0;

	if (power) {
		mfd->fbi->fbops->fb_blank(FB_BLANK_UNBLANK, mfd->fbi);
		mfd->fbi->fbops->fb_pan_display(&mfd->fbi->var, mfd->fbi);
		mipi_samsung_disp_send_cmd(mfd, PANEL_LATE_ON, true);
		mipi_samsung_disp_backlight(mfd);
	} else {
		mfd->fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, mfd->fbi);
	}

	pr_info("mipi_samsung_disp_set_power\n");
	return 0;
}
#endif

static ssize_t mipi_samsung_disp_lcdtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[20];

	snprintf(temp, strnlen(msd.mpd->panel_name, 20) + 1,
			msd.mpd->panel_name);
	strlcat(buf, temp, 20);
	return strnlen(buf, 20);
}

static ssize_t mipi_samsung_disp_gamma_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", msd.dstat.gamma_mode);
	pr_info("gamma_mode: %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_disp_gamma_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	if (sysfs_streq(buf, "1") && !msd.dstat.gamma_mode) {
		/* 1.9 gamma */
		msd.dstat.gamma_mode = GAMMA_1_9;
	} else if (sysfs_streq(buf, "0") && msd.dstat.gamma_mode) {
		/* 2.2 gamma */
		msd.dstat.gamma_mode = GAMMA_2_2;
	} else {
		pr_info("%s: Invalid argument!!", __func__);
	}

	return size;
}

static ssize_t mipi_samsung_disp_acl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", msd.dstat.acl_on);
	pr_info("acl status: %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_disp_acl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(msd.msm_pdev);

	if (!mfd->panel_power_on) {
		pr_info("%s: panel is off state. updating state value.\n",
				__func__);
		if (sysfs_streq(buf, "1") && !msd.dstat.acl_on)
			msd.dstat.acl_on = true;
		else if (sysfs_streq(buf, "0") && msd.dstat.acl_on)
			msd.dstat.acl_on = false;
		else
			pr_info("%s: Invalid argument!!", __func__);
	} else {
		if (sysfs_streq(buf, "1") && !msd.dstat.acl_on) {
			if (msd.mpd->set_acl(mfd->bl_level))
				mipi_samsung_disp_send_cmd(
						mfd, PANEL_ACL_OFF, true);
			else {
				mipi_samsung_disp_send_cmd(
						mfd, PANEL_ACL_ON, true);
			}
			msd.dstat.acl_on = true;
		} else if (sysfs_streq(buf, "0") && msd.dstat.acl_on) {
			mipi_samsung_disp_send_cmd(mfd, PANEL_ACL_OFF, true);
			msd.dstat.acl_on = false;
		} else {
			pr_info("%s: Invalid argument!!", __func__);
		}
	}

	return size;
}

static ssize_t mipi_samsung_auto_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n",
			msd.dstat.auto_brightness);
	pr_info("auot_brightness: %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	if (sysfs_streq(buf, "1"))
		msd.dstat.auto_brightness = 1;
	else if (sysfs_streq(buf, "0"))
		msd.dstat.auto_brightness = 0;
	else
		pr_info("%s: Invalid argument!!", __func__);

	return size;
}

static struct lcd_ops mipi_samsung_disp_props = {
#ifdef WA_FOR_FACTORY_MODE
	.get_power = NULL,
	.set_power = NULL,
#else
	.get_power = mipi_samsung_disp_get_power,
	.set_power = mipi_samsung_disp_set_power,
#endif
};

#ifdef WA_FOR_FACTORY_MODE
static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR,
		mipi_samsung_disp_get_power,
		mipi_samsung_disp_set_power);
#endif
static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_samsung_disp_lcdtype_show, NULL);
static DEVICE_ATTR(gamma_mode, S_IRUGO | S_IWUSR | S_IWGRP,
		mipi_samsung_disp_gamma_mode_show,
		mipi_samsung_disp_gamma_mode_store);
static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP,
		mipi_samsung_disp_acl_show,
		mipi_samsung_disp_acl_store);
static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		mipi_samsung_auto_brightness_show,
		mipi_samsung_auto_brightness_store);

#endif

static int __devinit mipi_samsung_disp_probe(struct platform_device *pdev)
{
	int ret;
	struct platform_device *msm_fb_added_dev;
#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct lcd_device *lcd_device;
#endif
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif
	msd.dstat.acl_on = false;

	printk(KERN_INFO "[lcd] mipi_samsung_disp_probe start\n");

	if (pdev->id == 0) {
		msd.mipi_samsung_disp_pdata = pdev->dev.platform_data;

		printk(KERN_INFO
		"[lcd] pdev->id =%d,  pdev-name = %s\n", pdev->id, pdev->name);

#ifdef CONFIG_SAMSUNG_CMC624
		if (samsung_has_cmc624()) {
			printk(KERN_DEBUG "Is_There_cmc624 : CMC624 is there!!!!");
			samsung_cmc624_init();
		} else {
			printk(KERN_DEBUG "Is_There_cmc624 : CMC624 is not there!!!!");
		}
#endif

		printk(KERN_INFO "[lcd] mipi_samsung_disp_probe end since pdev-id is 0\n");

		return 0;
	}

	printk(KERN_INFO "[lcd] msm_fb_add_device : %s\n", pdev->name);

	msm_fb_added_dev = msm_fb_add_device(pdev);

	mutex_init(&dsi_tx_mutex);
	
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_LCD_CLASS_DEVICE)
	msd.msm_pdev = msm_fb_added_dev;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
	msd.early_suspend.suspend = mipi_samsung_disp_early_suspend;
	msd.early_suspend.resume = mipi_samsung_disp_late_resume;
	msd.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&msd.early_suspend);

#endif

#if defined(CONFIG_LCD_CLASS_DEVICE)
	printk(KERN_INFO "[lcd] lcd_device_register for panel start\n");

	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
			&mipi_samsung_disp_props);

	if (IS_ERR(lcd_device)) {
		ret = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return ret;
	}

#ifdef WA_FOR_FACTORY_MODE
	sysfs_remove_file(&lcd_device->dev.kobj,
			&dev_attr_lcd_power.attr);

	ret = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_lcd_power.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_lcd_power.attr.name);
	}
#endif

	ret = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_lcd_type.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_lcd_type.attr.name);
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_gamma_mode.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_gamma_mode.attr.name);
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_power_reduce.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_power_reduce.attr.name);
	}
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	printk(KERN_INFO "[lcd] backlight_device_register for panel start\n");

	bd = backlight_device_register("panel", &lcd_device->dev,
			NULL, NULL, NULL);
	if (IS_ERR(bd)) {
		ret = PTR_ERR(bd);
		pr_info("backlight : failed to register device\n");
		return ret;
	}

	ret = sysfs_create_file(&bd->dev.kobj,
			&dev_attr_auto_brightness.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_auto_brightness.attr.name);
	}
#endif
#endif

#if defined(CONFIG_FB_MDP4_ENHANCE)
	init_mdnie_class();
#endif

	printk(KERN_INFO "[lcd] mipi_samsung_disp_probe end\n");

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_samsung_disp_probe,
	.driver = {
		.name   = "mipi_samsung_oled",
	},
	.shutdown = mipi_samsung_disp_shutdown
};

static struct msm_fb_panel_data samsung_panel_data = {
	.on		= mipi_samsung_disp_on,
	.off		= mipi_samsung_disp_off,
	.set_backlight	= mipi_samsung_disp_backlight,
};

static int ch_used[3];

int mipi_samsung_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel,
		struct mipi_panel_data *mpd)
{
	struct platform_device *pdev = NULL;
	int ret = 0;

	printk(KERN_INFO "[lcd] mipi_samsung_device_register start\n");

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_samsung_oled",
			(panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	samsung_panel_data.panel_info = *pinfo;
	msd.mpd = mpd;
	if (!msd.mpd) {
		printk(KERN_ERR
			"%s: get mipi_panel_data failed!\n", __func__);
		goto err_device_put;
	}
	mpd->msd = &msd;
	ret = platform_device_add_data(pdev, &samsung_panel_data,
			sizeof(samsung_panel_data));
	if (ret) {
		printk(KERN_ERR
			"%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
			"%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	printk(KERN_INFO "[lcd] mipi_samsung_device_register end\n");

	return ret;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_samsung_disp_init(void)
{
	printk(KERN_INFO "[lcd] mipi_samsung_disp_init start\n");

	mipi_dsi_buf_alloc(&msd.samsung_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&msd.samsung_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
module_init(mipi_samsung_disp_init);
