/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MIPI_SAMSUNG_OLED_H
#define MIPI_SAMSUNG_OLED_H

#define USE_ACL
#define USE_ELVSS

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT)
#define USE_READ_ID
#endif

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
extern void set_esd_refresh(boolean stat);
#endif

int mipi_samsung_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel,
		struct mipi_panel_data *mpd);
		
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT)

void reset_gamma_level(void);
unsigned char bypass_LCD_Id(void);
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
	/* [junesok] This was originally defined in samsung_cmc624.c */
/*extern u8 mtp_read_data[];
*/
#endif

#if defined(CONFIG_SAMSUNG_CMC624)
extern int cmc_fast_reset;
#endif
#endif  /* MIPI_SAMSUNG_OLED_H */
