/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Helper file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
#include <linux/smc.h>
#endif
#include <linux/exynos_iovmm.h>

#include "decon.h"
#include "dsim.h"
#include "dpp.h"
#include "displayport.h"
#include "../panel/panel_drv.h"
#include <video/mipi_display.h>

#ifdef CONFIG_DECON_EVENT_LOG
char acquire_fence_log[ACQUIRE_FENCE_LEN];
#endif

static int __dpu_match_dev(struct device *dev, void *data)
{
	struct dpp_device *dpp;
	struct dsim_device *dsim;
	struct displayport_device *displayport;
	struct panel_device *panel;
	struct decon_device *decon = (struct decon_device *)data;

	decon_dbg("%s: drvname(%s)\n", __func__, dev->driver->name);

	if (!strcmp(DPP_MODULE_NAME, dev->driver->name)) {
		dpp = (struct dpp_device *)dev_get_drvdata(dev);
		decon->dpp_sd[dpp->id] = &dpp->sd;
		decon_dbg("dpp%d sd name(%s)\n", dpp->id,
				decon->dpp_sd[dpp->id]->name);
	} else if (!strcmp(DSIM_MODULE_NAME, dev->driver->name)) {
		dsim = (struct dsim_device *)dev_get_drvdata(dev);
		decon->dsim_sd[dsim->id] = &dsim->sd;
		decon_dbg("dsim sd name(%s)\n", dsim->sd.name);
	} else if (!strcmp(DISPLAYPORT_MODULE_NAME, dev->driver->name)) {
		displayport = (struct displayport_device *)dev_get_drvdata(dev);
		decon->displayport_sd = &displayport->sd;
		decon_dbg("displayport sd name(%s)\n", displayport->sd.name);
	} else if (!strcmp(PANEL_DRV_NAME, dev->driver->name)) {
		panel = (struct panel_device *)dev_get_drvdata(dev);
		decon->panel_sd = &panel->sd;
		decon_dbg("panel sd name(%s)\n", panel->sd.name);
	} else {
		decon_err("failed to get driver name\n");
	}

	return 0;
}

int dpu_get_sd_by_drvname(struct decon_device *decon, char *drvname)
{
	struct device_driver *drv;
	struct device *dev;

	drv = driver_find(drvname, &platform_bus_type);
	if (IS_ERR_OR_NULL(drv)) {
		decon_err("failed to find driver\n");
		return -ENODEV;
	}

	dev = driver_find_device(drv, NULL, decon, __dpu_match_dev);

	return 0;
}

u32 dpu_translate_fmt_to_dpp(u32 format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_NV12:
		return DECON_PIXEL_FORMAT_NV21;
	case DECON_PIXEL_FORMAT_NV21:
		return DECON_PIXEL_FORMAT_NV12;
	case DECON_PIXEL_FORMAT_NV12M:
		return DECON_PIXEL_FORMAT_NV21M;
	case DECON_PIXEL_FORMAT_NV21M:
		return DECON_PIXEL_FORMAT_NV12M;
	case DECON_PIXEL_FORMAT_NV12N:
		return DECON_PIXEL_FORMAT_NV12N;
	case DECON_PIXEL_FORMAT_YUV420:
		return DECON_PIXEL_FORMAT_YVU420;
	case DECON_PIXEL_FORMAT_YVU420:
		return DECON_PIXEL_FORMAT_YUV420;
	case DECON_PIXEL_FORMAT_YUV420M:
		return DECON_PIXEL_FORMAT_YVU420M;
	case DECON_PIXEL_FORMAT_YVU420M:
		return DECON_PIXEL_FORMAT_YUV420M;
	case DECON_PIXEL_FORMAT_ARGB_8888:
		return DECON_PIXEL_FORMAT_BGRA_8888;
	case DECON_PIXEL_FORMAT_ABGR_8888:
		return DECON_PIXEL_FORMAT_RGBA_8888;
	case DECON_PIXEL_FORMAT_RGBA_8888:
		return DECON_PIXEL_FORMAT_ABGR_8888;
	case DECON_PIXEL_FORMAT_BGRA_8888:
		return DECON_PIXEL_FORMAT_ARGB_8888;
	case DECON_PIXEL_FORMAT_XRGB_8888:
		return DECON_PIXEL_FORMAT_BGRX_8888;
	case DECON_PIXEL_FORMAT_XBGR_8888:
		return DECON_PIXEL_FORMAT_RGBX_8888;
	case DECON_PIXEL_FORMAT_RGBX_8888:
		return DECON_PIXEL_FORMAT_XBGR_8888;
	case DECON_PIXEL_FORMAT_BGRX_8888:
		return DECON_PIXEL_FORMAT_XRGB_8888;
	default:
		return format;
	}
}

u32 dpu_get_bpp(enum decon_pixel_format fmt)
{
	switch (fmt) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_XRGB_8888:
	case DECON_PIXEL_FORMAT_XBGR_8888:
	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_BGRX_8888:
		return 32;

	case DECON_PIXEL_FORMAT_RGBA_5551:
	case DECON_PIXEL_FORMAT_RGB_565:
	case DECON_PIXEL_FORMAT_NV16:
	case DECON_PIXEL_FORMAT_NV61:
	case DECON_PIXEL_FORMAT_YVU422_3P:
		return 16;

	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV12M:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_YUV420:
	case DECON_PIXEL_FORMAT_YVU420:
	case DECON_PIXEL_FORMAT_YUV420M:
	case DECON_PIXEL_FORMAT_YVU420M:
	case DECON_PIXEL_FORMAT_NV12N:
	case DECON_PIXEL_FORMAT_NV12N_10B:
		return 12;

	default:
		break;
	}

	return 0;
}

int dpu_get_plane_cnt(enum decon_pixel_format format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_XRGB_8888:
	case DECON_PIXEL_FORMAT_XBGR_8888:
	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_BGRX_8888:
	case DECON_PIXEL_FORMAT_RGBA_5551:
	case DECON_PIXEL_FORMAT_RGB_565:
	case DECON_PIXEL_FORMAT_NV12N:
	case DECON_PIXEL_FORMAT_NV12N_10B:
		return 1;

	case DECON_PIXEL_FORMAT_NV16:
	case DECON_PIXEL_FORMAT_NV61:
	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV12M:
	case DECON_PIXEL_FORMAT_NV21M:
		return 2;

	case DECON_PIXEL_FORMAT_YVU422_3P:
	case DECON_PIXEL_FORMAT_YUV420:
	case DECON_PIXEL_FORMAT_YVU420:
	case DECON_PIXEL_FORMAT_YUV420M:
	case DECON_PIXEL_FORMAT_YVU420M:
		return 3;

	default:
		decon_err("invalid format(%d)\n", format);
		return 1;
	}
}

u32 dpu_get_alpha_len(int format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
		return 8;

	case DECON_PIXEL_FORMAT_RGBA_5551:
		return 1;

	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_RGB_565:
	case DECON_PIXEL_FORMAT_BGRX_8888:
		return 0;

	default:
		return 0;
	}
}

bool decon_intersect(struct decon_rect *r1, struct decon_rect *r2)
{
	return !(r1->left > r2->right || r1->right < r2->left ||
		r1->top > r2->bottom || r1->bottom < r2->top);
}

int decon_intersection(struct decon_rect *r1,
			struct decon_rect *r2, struct decon_rect *r3)
{
	r3->top = max(r1->top, r2->top);
	r3->bottom = min(r1->bottom, r2->bottom);
	r3->left = max(r1->left, r2->left);
	r3->right = min(r1->right, r2->right);
	return 0;
}

bool is_decon_rect_differ(struct decon_rect *r1, struct decon_rect *r2)
{
	return ((r1->left != r2->left) || (r1->top != r2->top) ||
		(r1->right != r2->right) || (r1->bottom != r2->bottom));
}

bool is_scaling(struct decon_win_config *config)
{
	return (config->dst.w != config->src.w) || (config->dst.h != config->src.h);
}

bool is_full(struct decon_rect *r, struct decon_lcd *lcd)
{
	return (r->left == 0) && (r->top == 0) &&
		(r->right == lcd->xres - 1) && (r->bottom == lcd->yres - 1);
}

bool is_rgb32(int format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_XRGB_8888:
	case DECON_PIXEL_FORMAT_XBGR_8888:
	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_BGRX_8888:
		return true;
	default:
		return false;
	}
}

bool is_decon_opaque_format(int format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_RGBA_5551:
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
		return false;

	default:
		return true;
	}
}

void dpu_unify_rect(struct decon_rect *r1, struct decon_rect *r2,
		struct decon_rect *dst)
{
	dst->top = min(r1->top, r2->top);
	dst->bottom = max(r1->bottom, r2->bottom);
	dst->left = min(r1->right, r2->right);
	dst->right = max(r1->right, r2->right);
}

void decon_to_psr_info(struct decon_device *decon, struct decon_mode_info *psr)
{
	psr->psr_mode = decon->dt.psr_mode;
	psr->trig_mode = decon->dt.trig_mode;
	psr->dsi_mode = decon->dt.dsi_mode;
	psr->out_type = decon->dt.out_type;
}

void decon_to_init_param(struct decon_device *decon, struct decon_param *p)
{
	struct decon_lcd *lcd_info = decon->lcd_info;
	struct v4l2_mbus_framefmt mbus_fmt;

	mbus_fmt.width = 0;
	mbus_fmt.height = 0;
	mbus_fmt.code = 0;
	mbus_fmt.field = 0;
	mbus_fmt.colorspace = 0;

	p->lcd_info = lcd_info;
	p->psr.psr_mode = decon->dt.psr_mode;
	p->psr.trig_mode = decon->dt.trig_mode;
	p->psr.dsi_mode = decon->dt.dsi_mode;
	p->psr.out_type = decon->dt.out_type;
	p->nr_windows = decon->dt.max_win;
	p->disp_ss_regs = decon->res.ss_regs;
	decon_dbg("%s: psr(%d) trig(%d) dsi(%d) out(%d) wins(%d) LCD[%d %d]\n",
			__func__, p->psr.psr_mode, p->psr.trig_mode,
			p->psr.dsi_mode, p->psr.out_type, p->nr_windows,
			decon->lcd_info->xres, decon->lcd_info->yres);
}

/* sync fence related functions */
void decon_create_timeline(struct decon_device *decon, char *name)
{
	decon->timeline = sw_sync_timeline_create(name);

	if (decon->dt.out_type == DECON_OUT_DSI)
		decon->timeline_max = 1;
	else if (decon->dt.out_type == DECON_OUT_WB)
		decon->timeline_max = 0;
	else if (decon->dt.out_type == DECON_OUT_DP)
		decon->timeline_max = 1;
	else
		decon->timeline_max = 1;
}

int decon_create_fence(struct decon_device *decon,
		struct sync_fence **fence, struct decon_reg_data *regs)
{
	struct sync_pt *pt;
	int fd = -EMFILE;

	decon->timeline_max++;
	pt = sw_sync_pt_create(decon->timeline, decon->timeline_max);
	if (!pt) {
		decon_err("%s: failed to create sync pt\n", __func__);
		goto err;
	}

	*fence = sync_fence_create("display", pt);
	if (!(*fence)) {
		decon_err("%s: failed to create fence\n", __func__);
		sync_pt_free(pt);
		goto err;
	}

	if (regs)
		regs->pt = pt;

	fd = get_unused_fd_flags(0);
	if (fd < 0) {
		decon_err("%s: failed to get unused fd\n", __func__);
		sync_fence_put(*fence);
		goto err;
	}

	return fd;

err:
	decon->timeline_max--;
	return fd;
}

void decon_install_fence(struct sync_fence *fence, int fd)
{
	sync_fence_install(fence, fd);
}

int decon_print_fence_err(struct decon_device *decon, struct seq_file *s)
{
	int i, idx;
	u64 time, before_time;
	ktime_t cur_time = ktime_get();
	struct disp_fence_err *fence_err;

	idx = (decon->fence_err_cnt - 1) % FENCE_ERR_LIST_CNT;

	if (s != NULL)
		seq_printf(s, "- Fence Error History (Total Cnt: %d) -\n",
			decon->fence_err_cnt);
	else
		decon_info("- Fence Error History (Total Cnt: %d) -\n",
			decon->fence_err_cnt);

	if (decon->fence_err_cnt <= 0)
		return 0;

	fence_err = &decon->first_fence_err;
	time = ktime_to_ms(fence_err->time);
	before_time = ktime_to_ms(ktime_sub(cur_time, fence_err->time));
	if (s != NULL)
		seq_printf(s, "1st : %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
			fence_err->name, fence_err->win_id, fence_err->status,
			time / 1000, time % 1000,
			before_time / 1000, before_time % 1000);
	else
		decon_info("1st: %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
			fence_err->name, fence_err->win_id, fence_err->status,
			time / 1000, time % 1000,
			before_time / 1000, before_time % 1000);


	for (i = idx ; i >= 0; i--) {
		fence_err = &decon->fence_err_list[i];
		if (fence_err == NULL) {
			decon_err("DECON:ERR:%s:fence_err_list is null\n", __func__);
			goto exit_dump;
		}

		time = ktime_to_ms(fence_err->time);
		before_time = ktime_to_ms(ktime_sub(cur_time, fence_err->time));
		if (s != NULL)
			seq_printf(s, "F: %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
				fence_err->name, fence_err->win_id, fence_err->status,
				time / 1000, time % 1000,
				before_time / 1000, before_time % 1000);

		else
			decon_info("F: %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
				fence_err->name, fence_err->win_id, fence_err->status,
				time / 1000, time % 1000,
				before_time / 1000, before_time % 1000);

	}

	for (i = FENCE_ERR_LIST_CNT - 1; i >= idx + 1; i--) {
		fence_err = &decon->fence_err_list[i];
		if (fence_err == NULL) {
			decon_err("DECON:ERR:%s:fence_err_list is null\n", __func__);
			goto exit_dump;
		}

		time = ktime_to_ms(fence_err->time);
		before_time = ktime_to_ms(ktime_sub(cur_time, fence_err->time));
		if (s != NULL)
			seq_printf(s, "F: %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
				fence_err->name, fence_err->win_id, fence_err->status,
				time / 1000, time % 1000,
				before_time / 1000, before_time % 1000);

		else
			decon_info("F: %s, W: %d, S: %d: %lld.%lld, Be: %lld.%lld\n",
				fence_err->name, fence_err->win_id, fence_err->status,
				time / 1000, time % 1000,
				before_time / 1000, before_time % 1000);
	}
exit_dump:
	return 0;
}


#define FENCE_NAME_MAX	32
#define FENCE_NAME_STR_MAX (FENCE_NAME_MAX - 1)

void decon_fence_err_log(struct decon_device *decon, int idx, struct sync_fence *fence)

{
	int err_idx;
	int namelen;

	namelen = strlen(fence->name);
	if (namelen > FENCE_NAME_STR_MAX)
		namelen = FENCE_NAME_STR_MAX;

	if (decon->fence_err_cnt == 0) {
		decon->first_fence_err.win_id = idx;
		decon->first_fence_err.time = ktime_get();
		decon->first_fence_err.status = atomic_read(&fence->status);
		memset(decon->first_fence_err.name, 0, FENCE_NAME_MAX);
		strncpy(decon->first_fence_err.name, fence->name, namelen);
	}

	err_idx = decon->fence_err_cnt % FENCE_ERR_LIST_CNT;
	decon->fence_err_list[err_idx].win_id = idx;
	decon->fence_err_list[err_idx].time = ktime_get();
	decon->fence_err_list[err_idx].status = atomic_read(&fence->status);
	memset(decon->fence_err_list[err_idx].name, 0, FENCE_NAME_MAX);
	strncpy(decon->fence_err_list[err_idx].name, fence->name, namelen);

	decon->fence_err_cnt += 1;

	decon_print_fence_err(decon, NULL);
}


int decon_wait_fence(struct sync_fence *fence)
{
	int err = 0;

#ifdef CONFIG_DECON_EVENT_LOG
	snprintf(acquire_fence_log, ACQUIRE_FENCE_LEN, "%p:%s:%d",
			fence, fence->name, atomic_read(&fence->status));
#endif
	err = sync_fence_wait(fence, 900);
#ifdef CONFIG_DECON_EVENT_LOG
	if (err < 0)
		decon_warn("%s: error waiting on acquire fence: %d\n", acquire_fence_log, err);
#endif

	return err;
}

void decon_signal_fence(struct decon_device *decon)
{
	sw_sync_timeline_inc(decon->timeline, 1);
}

void dpu_debug_printk(const char *function_name, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);
	vaf.fmt = format;
	vaf.va = &args;

	printk(KERN_INFO "[%s] %pV", function_name, &vaf);

	va_end(args);
}

void __iomem *dpu_get_sysreg_addr(void)
{
	void __iomem *regs;

	if (of_have_populated_dt()) {
		struct device_node *nd;

		nd = of_find_compatible_node(NULL, NULL,
				"samsung,exynos8-disp_ss");
		if (!nd) {
			decon_err("failed find compatible node(sysreg-disp)");
			return NULL;
		}

		regs = of_iomap(nd, 0);
		if (!regs) {
			decon_err("Failed to get sysreg-disp address.");
			return NULL;
		}
	} else {
		decon_err("failed have populated device tree");
		return NULL;
	}

	decon_dbg("%s: default sysreg value(0x%x)\n", __func__, readl(regs));

	return regs;
}

void __iomem *dpu_get_version_addr(void)
{
	void __iomem *regs;

	if (of_have_populated_dt()) {
		struct device_node *nd;

		nd = of_find_compatible_node(NULL, NULL,
				"samsung,exynos8-disp-ver");
		if (!nd) {
			decon_err("failed find compatible node(disp-ver)");
			return NULL;
		}

		regs = of_iomap(nd, 0);
		if (!regs) {
			decon_err("Failed to get disp-ver address.");
			return NULL;
		}
	} else {
		decon_err("failed have populated device tree");
		return NULL;
	}

	decon_info("%s: default ver value(0x%x)\n", __func__, readl(regs));

	return regs;
}




/*
 * DMA_CH0 : VGF0/VGF1
 * DMA_CH1 : G0-VG0
 * DMA_CH2 : G1-VG1
*/
u32 dpu_dma_type_to_channel(enum decon_idma_type type)
{
	u32 ch_id;

	switch (type) {
	case IDMA_G0_S:
	case IDMA_G0:
		ch_id = 5;
		break;
	case IDMA_G1:
		ch_id = 3;
		break;
	case IDMA_VG0:
		ch_id = 0;
		break;
	case IDMA_VG1:
		ch_id = 4;
		break;
	case IDMA_VGF0:
		ch_id = 1;
		break;
	case IDMA_VGF1:
		ch_id = 2;
		break;
	default:
		decon_dbg("channel(0x%x) is not valid\n", type);
		return -EINVAL;
	}

	return ch_id;
}

#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
static int decon_get_protect_id(int dma_id)
{
	int prot_id = 0;

	switch (dma_id) {
	case IDMA_G0:
		prot_id = PROT_G0;
		break;
	case IDMA_G1:
		prot_id = PROT_G1;
		break;
	case IDMA_VG0:
		prot_id = PROT_VG0;
		break;
	case IDMA_VG1:
		prot_id = PROT_VG1;
		break;
	case IDMA_VGF0:
		prot_id = PROT_VGR0;
		break;
	case IDMA_VGF1:
		prot_id = PROT_VGR1;
		break;
	case ODMA_WB:
		prot_id = PROT_WB1;
		break;
	default:
		decon_err("Unknown DMA_ID (%d)\n", dma_id);
		break;
	}

	return prot_id;
}

static int decon_control_protection(int dma_id, bool en)
{
	int ret = SUCCESS_EXYNOS_SMC;
	int prot_id;

	prot_id = decon_get_protect_id(dma_id);
	ret = exynos_smc(SMC_PROTECTION_SET, 0, prot_id,
		(en ? SMC_PROTECTION_ENABLE : SMC_PROTECTION_DISABLE));

	if (ret)
		decon_err("DMA%d (en=%d): exynos_smc call fail (err=%d)\n",
			dma_id, en, ret);
	else
		decon_dbg("DMA%d protection %s\n",
			dma_id, en ? "enabled" : "disabled");

	return ret;
}

void decon_set_protected_content(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	bool en;
	int dma_id, i, ret = 0;
	u32 change = 0;
	u32 cur_protect_bits = 0;

	/* IDMA protection configs (G0,G1,VG0,VG1,VGF0,VGF1) */
	for (i = 0; i < decon->dt.max_win; i++) {
		if (!regs)
			break;

		cur_protect_bits |=
			(regs->protection[i] << regs->dpp_config[i].idma_type);
	}

	/* ODMA protection config (WB: writeback) */
	if (decon->dt.out_type == DECON_OUT_WB)
		if (regs)
			cur_protect_bits |= (regs->protection[MAX_DECON_WIN] << ODMA_WB);

	if (decon->prev_protection_bitmask != cur_protect_bits) {

		/* apply protection configs for each DMA */
		for (dma_id = 0; dma_id < MAX_DPP_CNT; dma_id++) {
			en = cur_protect_bits & (1 << dma_id);

			change = (cur_protect_bits & (1 << dma_id)) ^
				(decon->prev_protection_bitmask & (1 << dma_id));

			if (change) {
				if (decon->version == 0) {
					/**
					 * if the shadowed-TZPC is supported,
					 * DPP_WAIT_IDLE is not necessary
					 */
					struct v4l2_subdev *sd = NULL;
					unsigned long wait_to = 20*1000; /* 20ms */
					sd = decon->dpp_sd[dma_id];
					v4l2_subdev_call(sd, core, ioctl,
						DPP_WAIT_IDLE, (void *)wait_to);
				}
				ret = decon_control_protection(dma_id, en);
			}
		}
	}

	/* save current portection configs */
	decon->prev_protection_bitmask = cur_protect_bits;
}
#endif

/* id : VGF0=0, VGF1=1 */
void dpu_dump_data_to_console(void *v_addr, int buf_size, int id)
{
	dpp_info("=== (IDMA_VGF%d) Frame Buffer Data(%d Bytes) ===\n",
		id == IDMA_VGF0 ? 0 : 1, BUF_DUMP_SIZE);

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			v_addr, buf_size, false);
}

void dpu_clear_all_irq(struct decon_device *decon)
{
	int i;
	u32 mgr_id = 0;
	struct v4l2_subdev *sd;

	/* dsim */
	if (decon->dt.out_type == DECON_OUT_DSI) {
		sd = decon->out_sd[0];
		v4l2_subdev_call(sd, core, ioctl, DSIM_IOC_CLEAR_IRQ, NULL);
	}

	/* displayport : add code if necessary */

	/* decon */
	decon_reg_clear_int_all(decon->id);

	/* dma & dpp */
	if (decon->id == 0) {
		sd = decon->dpp_sd[IDMA_G0];
		v4l2_subdev_call(sd, core, ioctl, DPP_CLEAR_IRQ, NULL);
	}
	for (i = IDMA_G1; i < MAX_DPP_SUBDEV; i++) {
		mgr_id = decon_reg_get_rsc_dma_info(decon->id, i);
		if (mgr_id == decon->id) {
			sd = decon->dpp_sd[i];
			v4l2_subdev_call(sd, core, ioctl, DPP_CLEAR_IRQ, NULL);
		}
	}
}
