/*
 * @File
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/version.h>

#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/jiffies.h>
#include <linux/dma-fence.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <linux/completion.h>
#include <linux/dma-buf.h>
#if defined(CONFIG_OF)
#include <linux/of_device.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_vblank.h>
#include <linux/platform_device.h>
#else
#include <drm/drmP.h>
#endif

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_modes.h>
#include <drm/drm_plane_helper.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0))
#include <drm/drm_atomic_helper.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0))
#include <drm/drm_probe_helper.h>
#endif

#include "pvr_dma_resv.h"

#include "img_drm_fourcc_internal.h"
#include <pvrversion.h>

#include <drm/drm_fourcc.h>

#include "drm_nulldisp_drv.h"
#if defined(LMA)
#if !defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
#include "tc_drv.h"
#endif
#include "drm_pdp_gem.h"
#include "pdp_drm.h"
#else
#include "drm_nulldisp_gem.h"
#endif
#include "nulldisp_drm.h"
#include "drm_netlink_gem.h"
#include "drm_nulldisp_netlink.h"

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
#include <drm/drm_gem_atomic_helper.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0) */

#include "kernel_compatibility.h"

MODULE_IMPORT_NS(DMA_BUF);

#define DRIVER_NAME "nulldisp"
#define DRIVER_DESC "Imagination Technologies Null DRM Display Driver"
#define DRIVER_DATE "20150612"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#define	NULLDISP_DRIVER_PRIME 0
#else
#define	NULLDISP_DRIVER_PRIME DRIVER_PRIME
#endif

#define NULLDISP_FB_WIDTH_MIN 0
#define NULLDISP_FB_WIDTH_MAX 8192
#define NULLDISP_FB_HEIGHT_MIN 0
#define NULLDISP_FB_HEIGHT_MAX 8192

#if defined(ANDROID)
#define NULLDISP_DEFAULT_WIDTH PVR_ANDROID_VNC_WIDTH
#define NULLDISP_DEFAULT_HEIGHT PVR_ANDROID_VNC_HEIGHT
#else
#define NULLDISP_DEFAULT_WIDTH 640
#define NULLDISP_DEFAULT_HEIGHT 480
#endif
#define NULLDISP_DEFAULT_REFRESH_RATE 60

#define NULLDISP_MAX_PLANES 3

#define NULLDISP_NETLINK_TIMEOUT 5
#define NULLDISP_NETLINK_TIMEOUT_MAX 300
#define NULLDISP_NETLINK_TIMEOUT_MIN 1

enum nulldisp_crtc_flip_status {
	NULLDISP_CRTC_FLIP_STATUS_NONE = 0,
	NULLDISP_CRTC_FLIP_STATUS_DONE,
};

struct nulldisp_flip_data {
	struct dma_fence_cb base;
	struct drm_crtc *crtc;
	struct dma_fence *wait_fence;
};

struct nulldisp_crtc {
	struct drm_crtc base;
	struct delayed_work vb_work;
	struct drm_framebuffer *fb;
	struct completion flip_done;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
	struct completion copy_done;
#endif

	/* Reuse the drm_device event_lock to protect these */
	atomic_t flip_status;
	struct drm_pending_vblank_event *flip_event;
	bool flip_async;
};

/* Structure representing fake display hardware */
struct netlink_pipe {
	/* In jiffies */
	atomic_long_t refresh_interval;
};

struct nulldisp_display_device {
	struct drm_device *dev;

	struct workqueue_struct *workqueue;
	struct nulldisp_crtc *nulldisp_crtc;
	struct nlpvrdpy *nlpvrdpy;
#if defined(LMA)
	struct pdp_gem_private *pdp_gem_priv;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0))
	struct drm_connector *connector;
#endif

	struct netlink_pipe display_pipe;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
	atomic_t open_count;
#endif
};

struct nulldisp_module_params {
	unsigned int hdisplay;
	unsigned int vdisplay;
	unsigned int vrefresh;
	unsigned int updateto;
};

#define to_nulldisp_crtc(crtc) \
	container_of(crtc, struct nulldisp_crtc, base)

#if defined(LMA)
#define	obj_to_resv(obj) pdp_gem_get_resv(obj)
#else
#define	obj_to_resv(obj) nulldisp_gem_get_resv(obj)
#endif

/*
 * The order of this array helps determine the order in which EGL configs are
 * returned to an application using eglGetConfigs. As such, RGB 8888 formats
 * should appear first, followed by RGB 565 configs. YUV configs should appear
 * last.
 */
static const uint32_t nulldisp_modeset_formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_ABGR2101010,
#ifdef DRM_FORMAT_AXBXGXRX106106106106
	DRM_FORMAT_AXBXGXRX106106106106,
#endif
#ifdef DRM_FORMAT_ABGR16161616F
	DRM_FORMAT_ABGR16161616F,
#endif
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YVU420,
};

/*
 * Note that nulldisp, being a no-hardware display controller driver,
 * "supports" a number different decompression hardware
 * versions (V0, V1, V2 ...). Real, hardware display controllers are
 * likely to support only a single version.
 */
static const uint64_t nulldisp_primary_plane_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V0,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V0_FIX,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V1,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V2,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V3,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V7,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V8,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V10,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V12,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_8x8_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY37_8x8_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_8x8_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_8x8_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_8x8_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_8x8_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY37_8x8_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_8x8_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_8x8_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V0,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V0_FIX,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V1,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V2,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V3,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V7,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V8,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V10,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V12,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY37_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_16x4_V13,
	DRM_FORMAT_MOD_PVR_FBCDC_16x4_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_16x4_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY37_16x4_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_16x4_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_16x4_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V1,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V3,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V8,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V10,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V12,
	DRM_FORMAT_MOD_PVR_FBCDC_32x2_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY25_32x2_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY37_32x2_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY50_32x2_V14,
	DRM_FORMAT_MOD_PVR_FBCDC_LOSSY75_32x2_V14,
	DRM_FORMAT_MOD_INVALID
};

static struct nulldisp_module_params module_params = {
	.hdisplay = NULLDISP_DEFAULT_WIDTH,
	.vdisplay = NULLDISP_DEFAULT_HEIGHT,
	.vrefresh = NULLDISP_DEFAULT_REFRESH_RATE,
	.updateto = NULLDISP_NETLINK_TIMEOUT,
};

static int updateto_param_set(const char *val, const struct kernel_param *kp);

static const struct kernel_param_ops updateto_ops = {
	.set = updateto_param_set,
	.get = param_get_uint,
};

module_param_named(width, module_params.hdisplay, uint, 0444);
module_param_named(height, module_params.vdisplay, uint, 0444);
module_param_named(refreshrate, module_params.vrefresh, uint, 0444);
module_param_cb(updateto, &updateto_ops, &module_params.updateto, 0644);

MODULE_PARM_DESC(width, "Preferred display width in pixels");
MODULE_PARM_DESC(height, "Preferred display height in pixels");
MODULE_PARM_DESC(refreshrate, "Preferred display refresh rate");
MODULE_PARM_DESC(updateto, "Preferred remote update timeout (in seconds)");

/*
 * Please use this function to obtain the module parameters instead of
 * accessing the global "module_params" structure directly.
 */
static inline const struct nulldisp_module_params *
nulldisp_get_module_params(void)
{
	return &module_params;
}

static int updateto_param_set(const char *val, const struct kernel_param *kp)
{
	unsigned int updateto;
	int err;

	err = kstrtouint(val, 10, &updateto);
	if (err)
		return err;

	if (updateto < NULLDISP_NETLINK_TIMEOUT_MIN ||
	    updateto > NULLDISP_NETLINK_TIMEOUT_MAX)
		return -EINVAL;

	return param_set_uint(val, kp);
}

static unsigned long nulldisp_netlink_timeout(void)
{
	const struct nulldisp_module_params *module_params =
		nulldisp_get_module_params();
	unsigned int updateto;

	kernel_param_lock(THIS_MODULE);

	updateto = module_params->updateto;

	kernel_param_unlock(THIS_MODULE);

	return msecs_to_jiffies(updateto * 1000);
}

/******************************************************************************
 * Linux compatibility functions
 ******************************************************************************/
static inline void
nulldisp_drm_fb_set_format(struct drm_framebuffer *fb,
			   u32 pixel_format)
{
	fb->format = drm_format_info(pixel_format);
}

static inline void nulldisp_drm_fb_set_modifier(struct drm_framebuffer *fb,
						uint64_t value)
{
	fb->modifier = value;
}

/******************************************************************************
 * Plane functions
 ******************************************************************************/
static bool nulldisp_primary_format_mod_supported(struct drm_plane *plane,
						  uint32_t format,
						  uint64_t modifier)
{
	/*
	 * All 'nulldisp_modeset_formats' are supported for every modifier
	 * in the 'nulldisp_primary_plane_modifiers' array.
	 */
	return true;
}

static int nulldisp_plane_helper_atomic_check(struct drm_plane *plane,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))
					      struct drm_plane_state *state)
{
#else
					      struct drm_atomic_state *astate)
{
	struct drm_plane_state *state =
			drm_atomic_get_new_plane_state(astate, plane);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) */
	struct drm_crtc_state *crtc_new_state;

	if (!state->crtc)
		return 0;

	crtc_new_state = drm_atomic_get_new_crtc_state(state->state,
						       state->crtc);

	return drm_atomic_helper_check_plane_state(state, crtc_new_state,
						   DRM_PLANE_NO_SCALING,
						   DRM_PLANE_NO_SCALING,
						   false, true);
}

static void
nulldisp_plane_helper_atomic_update(struct drm_plane *plane,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))
				  struct drm_plane_state *old_state)
#else
				  struct drm_atomic_state *astate)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) */
{
	struct drm_plane_state *state = plane->state;

	if (state->crtc) {
		struct nulldisp_crtc *nulldisp_crtc =
					to_nulldisp_crtc(state->crtc);

		nulldisp_crtc->fb = state->fb;
	}
}

static const struct drm_plane_helper_funcs nulldisp_plane_helper_funcs = {
	.prepare_fb =  drm_gem_plane_helper_prepare_fb,
	.atomic_check = nulldisp_plane_helper_atomic_check,
	.atomic_update = nulldisp_plane_helper_atomic_update,
};

static const struct drm_plane_funcs nulldisp_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_helper_destroy,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.format_mod_supported = nulldisp_primary_format_mod_supported,
};

/******************************************************************************
 * CRTC functions
 ******************************************************************************/

static bool
nulldisp_crtc_helper_mode_fixup(struct drm_crtc *crtc,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	/*
	 * Fix up mode so that it's compatible with the hardware. The results
	 * should be stored in adjusted_mode (i.e. mode should be untouched).
	 */
	return true;
}

static void nulldisp_crtc_helper_disable(struct drm_crtc *crtc)
{
	struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);

	flush_delayed_work(&nulldisp_crtc->vb_work);

	drm_crtc_vblank_off(crtc);
	flush_delayed_work(&nulldisp_crtc->vb_work);

	/*
	 * Vblank has been disabled, so the vblank handler shouldn't be
	 * able to reschedule itself.
	 */
	BUG_ON(cancel_delayed_work(&nulldisp_crtc->vb_work));

	BUG_ON(atomic_read(&nulldisp_crtc->flip_status) !=
	       NULLDISP_CRTC_FLIP_STATUS_NONE);

}

static void nulldisp_crtc_flip_complete(struct drm_crtc *crtc)
{
	struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);

	/* The flipping process has been completed so reset the flip state */
	atomic_set(&nulldisp_crtc->flip_status, NULLDISP_CRTC_FLIP_STATUS_NONE);
	nulldisp_crtc->flip_async = false;

	if (nulldisp_crtc->flip_event) {
		drm_crtc_send_vblank_event(crtc, nulldisp_crtc->flip_event);
		nulldisp_crtc->flip_event = NULL;
	}

	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static void nulldisp_crtc_helper_mode_set_nofb(struct drm_crtc *crtc)
{
	struct nulldisp_display_device *nulldisp_dev = crtc->dev->dev_private;
	atomic_long_t *refresh_interval =
		&nulldisp_dev->display_pipe.refresh_interval;
	int new_refresh_rate;
	long new_refresh_interval;

	/* This callback is to program HW, modeset mutex must be held */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	drm_modeset_lock_assert_held(&crtc->mutex);
#endif

	new_refresh_rate = drm_mode_vrefresh(&crtc->state->adjusted_mode);
	if (!new_refresh_rate) {
		new_refresh_rate = NULLDISP_DEFAULT_REFRESH_RATE;
		DRM_WARN(
			"vertical refresh rate is zero, defaulting to %d\n",
			new_refresh_rate);
	}

	new_refresh_interval =
		usecs_to_jiffies(USEC_PER_SEC / new_refresh_rate);
	atomic_long_set(refresh_interval, new_refresh_interval);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void nulldisp_crtc_helper_atomic_flush(struct drm_crtc *crtc,
					      struct drm_crtc_state *old_state)
{
#else
static void nulldisp_crtc_helper_atomic_flush(struct drm_crtc *crtc,
					      struct drm_atomic_state *state)
{
	struct drm_crtc_state *old_state = drm_atomic_get_new_crtc_state(state, crtc);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */
	struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);

	if (!crtc->state->active || !old_state->active)
		return;

	if (nulldisp_crtc->fb) {
		struct nulldisp_display_device *nulldisp_dev =
							crtc->dev->dev_private;

		reinit_completion(&nulldisp_crtc->flip_done);

		if (!nlpvrdpy_send_flip(nulldisp_dev->nlpvrdpy,
				       nulldisp_crtc->fb,
				       &nulldisp_crtc->fb->obj[0])) {
			unsigned long res;

			res = wait_for_completion_timeout(
					&nulldisp_crtc->flip_done,
					nulldisp_netlink_timeout());

			if (!res)
				DRM_ERROR(
				    "timed out waiting for remote update\n");
		}

		nulldisp_crtc->fb = NULL;
	}

	if (crtc->state->event) {
		unsigned long flags;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		nulldisp_crtc->flip_async = crtc->state->async_flip;
#else
		nulldisp_crtc->flip_async = !!(crtc->state->pageflip_flags
					       & DRM_MODE_PAGE_FLIP_ASYNC);
#endif
		if (nulldisp_crtc->flip_async)
			WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		nulldisp_crtc->flip_event = crtc->state->event;
		crtc->state->event = NULL;

		atomic_set(&nulldisp_crtc->flip_status,
			   NULLDISP_CRTC_FLIP_STATUS_DONE);
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

		if (nulldisp_crtc->flip_async)
			nulldisp_crtc_flip_complete(crtc);
	}
}

static void nulldisp_crtc_set_enabled(struct drm_crtc *crtc, bool enable)
{
	if (enable)
		drm_crtc_vblank_on(crtc);
	else
		nulldisp_crtc_helper_disable(crtc);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void
nulldisp_crtc_helper_atomic_enable(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_crtc_state)
#else
static void
nulldisp_crtc_helper_atomic_enable(struct drm_crtc *crtc,
				   struct drm_atomic_state *state)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */
{
	nulldisp_crtc_set_enabled(crtc, true);

	if (crtc->state->event) {
		struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);
		unsigned long flags;

		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		nulldisp_crtc->flip_event = crtc->state->event;
		crtc->state->event = NULL;

		atomic_set(&nulldisp_crtc->flip_status,
			   NULLDISP_CRTC_FLIP_STATUS_DONE);
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void
nulldisp_crtc_helper_atomic_disable(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_crtc_state)
#else
static void
nulldisp_crtc_helper_atomic_disable(struct drm_crtc *crtc,
				    struct drm_atomic_state *state)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */
{
	struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);

	nulldisp_crtc_set_enabled(crtc, false);

	nulldisp_crtc->fb = NULL;

	if (crtc->state->event) {
		unsigned long flags;

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	}
}

static void nulldisp_crtc_destroy(struct drm_crtc *crtc)
{
	struct nulldisp_crtc *nulldisp_crtc = to_nulldisp_crtc(crtc);

	DRM_DEBUG_DRIVER("[CRTC:%d]\n", crtc->base.id);

	drm_crtc_cleanup(crtc);

	BUG_ON(atomic_read(&nulldisp_crtc->flip_status) !=
	       NULLDISP_CRTC_FLIP_STATUS_NONE);

	kfree(nulldisp_crtc);
}


static bool nulldisp_queue_vblank_work(struct nulldisp_crtc *nulldisp_crtc)
{
	struct drm_crtc *crtc = &nulldisp_crtc->base;
	struct nulldisp_display_device *nulldisp_dev = crtc->dev->dev_private;
	long refresh_interval =
		atomic_long_read(&nulldisp_dev->display_pipe.refresh_interval);

	/* Returns false if work already queued, else true */
	return queue_delayed_work(nulldisp_dev->workqueue,
				  &nulldisp_crtc->vb_work,
				  refresh_interval);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
static int nulldisp_enable_vblank(struct drm_crtc *crtc)
#else
static int nulldisp_enable_vblank(struct drm_device *dev, unsigned int pipe)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	struct drm_device *dev = crtc->dev;
	unsigned int pipe      = drm_crtc_index(crtc);
#endif

	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

	switch (pipe) {
	case 0:
		break;
	default:
		DRM_ERROR("invalid crtc %u\n", pipe);
		return -EINVAL;
	}

	if (!nulldisp_queue_vblank_work(nulldisp_dev->nulldisp_crtc)) {
		DRM_ERROR("work already queued\n");
		return -1;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
static void nulldisp_disable_vblank(struct drm_crtc *crtc)
#else
static void nulldisp_disable_vblank(struct drm_device *dev, unsigned int pipe)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	struct drm_device *dev = crtc->dev;
	unsigned int pipe      = drm_crtc_index(crtc);
#endif

	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

	switch (pipe) {
	case 0:
		break;
	default:
		DRM_ERROR("invalid crtc %u\n", pipe);
		return;
	}

	/*
	 * Vblank events may be disabled from within the vblank handler,
	 * so don't wait for the work to complete.
	 */
	(void) cancel_delayed_work(&nulldisp_dev->nulldisp_crtc->vb_work);
}

static const struct drm_crtc_helper_funcs nulldisp_crtc_helper_funcs = {
	.mode_fixup = nulldisp_crtc_helper_mode_fixup,
	.mode_set_nofb = nulldisp_crtc_helper_mode_set_nofb,
	.atomic_flush = nulldisp_crtc_helper_atomic_flush,
	.atomic_enable = nulldisp_crtc_helper_atomic_enable,
	.atomic_disable = nulldisp_crtc_helper_atomic_disable,
};

static const struct drm_crtc_funcs nulldisp_crtc_funcs = {
	.destroy = nulldisp_crtc_destroy,
	.reset = drm_atomic_helper_crtc_reset,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	.enable_vblank  = nulldisp_enable_vblank,
	.disable_vblank = nulldisp_disable_vblank,
#endif
};

static void nulldisp_handle_vblank(struct work_struct *w)
{
	struct delayed_work *dw =
		container_of(w, struct delayed_work, work);
	struct nulldisp_crtc *nulldisp_crtc =
		container_of(dw, struct nulldisp_crtc, vb_work);
	struct drm_crtc *crtc = &nulldisp_crtc->base;
	struct drm_device *dev = crtc->dev;
	enum nulldisp_crtc_flip_status status;

	/*
	 * Reschedule the handler, if necessary. This is done before
	 * calling drm_crtc_vblank_put, so that the work can be cancelled
	 * if vblank events are disabled.
	 */
	if (drm_handle_vblank(dev, 0))
		(void) nulldisp_queue_vblank_work(nulldisp_crtc);

	status = atomic_read(&nulldisp_crtc->flip_status);
	if (status == NULLDISP_CRTC_FLIP_STATUS_DONE) {
		if (!nulldisp_crtc->flip_async)
			nulldisp_crtc_flip_complete(crtc);
	}

}

static struct nulldisp_crtc *
nulldisp_crtc_create(struct nulldisp_display_device *nulldisp_dev)
{
	struct nulldisp_crtc *nulldisp_crtc;
	struct drm_crtc *crtc;
	struct drm_plane *primary;

	nulldisp_crtc = kzalloc(sizeof(*nulldisp_crtc), GFP_KERNEL);
	if (!nulldisp_crtc)
		goto err_return;

	primary = kzalloc(sizeof(*primary), GFP_KERNEL);
	if (!primary)
		goto err_free_crtc;

	crtc = &nulldisp_crtc->base;

	atomic_set(&nulldisp_crtc->flip_status, NULLDISP_CRTC_FLIP_STATUS_NONE);
	init_completion(&nulldisp_crtc->flip_done);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
	init_completion(&nulldisp_crtc->copy_done);
#endif

	if (drm_universal_plane_init(nulldisp_dev->dev, primary, 0,
				     &nulldisp_plane_funcs,
				     nulldisp_modeset_formats,
				     ARRAY_SIZE(nulldisp_modeset_formats),
				     nulldisp_primary_plane_modifiers,
				     DRM_PLANE_TYPE_PRIMARY, NULL)) {
		goto err_free_primary;
	}

	drm_plane_helper_add(primary, &nulldisp_plane_helper_funcs);

	if (drm_crtc_init_with_planes(nulldisp_dev->dev, crtc, primary,
				      NULL, &nulldisp_crtc_funcs, NULL)) {
		goto err_cleanup_plane;
	}

	drm_crtc_helper_add(crtc, &nulldisp_crtc_helper_funcs);

	INIT_DELAYED_WORK(&nulldisp_crtc->vb_work, nulldisp_handle_vblank);

	DRM_DEBUG_DRIVER("[CRTC:%d]\n", crtc->base.id);

	return nulldisp_crtc;

err_cleanup_plane:
	drm_plane_cleanup(primary);
err_free_primary:
	kfree(primary);
err_free_crtc:
	kfree(nulldisp_crtc);
err_return:
	return NULL;
}


/******************************************************************************
 * Connector functions
 ******************************************************************************/

static int
nulldisp_validate_module_parameters(void)
{
	const struct nulldisp_module_params *module_params =
		nulldisp_get_module_params();

	if (!module_params->hdisplay ||
	    !module_params->vdisplay ||
	    !module_params->vrefresh ||
	    (module_params->hdisplay > NULLDISP_FB_WIDTH_MAX) ||
	    (module_params->vdisplay > NULLDISP_FB_HEIGHT_MAX))
		return -EINVAL;

	return 0;
}

static bool
nulldisp_set_preferred_mode(struct drm_connector *connector,
			    uint32_t hdisplay,
			    uint32_t vdisplay,
			    uint32_t vrefresh)
{
	struct drm_display_mode *mode;

	/*
	 * Mark the first mode, matching the hdisplay, vdisplay and
	 * vrefresh, preferred.
	 */
	list_for_each_entry(mode, &connector->probed_modes, head)
		if (mode->hdisplay == hdisplay &&
		    mode->vdisplay == vdisplay &&
		    drm_mode_vrefresh(mode) == vrefresh) {
			mode->type |= DRM_MODE_TYPE_PREFERRED;
			return true;
		}

	return false;
}

static bool
nulldisp_connector_add_preferred_mode(struct drm_connector *connector,
				      uint32_t hdisplay,
				      uint32_t vdisplay,
				      uint32_t vrefresh)
{
	struct drm_display_mode *preferred_mode;

	preferred_mode = drm_cvt_mode(connector->dev,
				      hdisplay, vdisplay, vrefresh,
				      false, false, false);
	if (!preferred_mode) {
		DRM_DEBUG_DRIVER("[CONNECTOR:%s]:create mode %dx%d@%d failed\n",
				 connector->name,
				 hdisplay,
				 vdisplay,
				 vrefresh);

		return false;
	}

	preferred_mode->type = DRM_MODE_TYPE_PREFERRED | DRM_MODE_TYPE_DRIVER;

	drm_mode_probed_add(connector, preferred_mode);

	return true;
}

/*
 * Gather modes. Here we can get the EDID data from the monitor and
 * turn it into drm_display_mode structures.
 */
static int
nulldisp_connector_helper_get_modes(struct drm_connector *connector)
{
	int modes_count;
	struct drm_device *dev = connector->dev;
	const struct nulldisp_module_params *module_params =
		nulldisp_get_module_params();
	uint32_t hdisplay = module_params->hdisplay;
	uint32_t vdisplay = module_params->vdisplay;
	uint32_t vrefresh = module_params->vrefresh;

	/* Add common modes */
	modes_count = drm_add_modes_noedid(connector,
					   dev->mode_config.max_width,
					   dev->mode_config.max_height);

	/*
	 * Check if any of the connector modes match the preferred mode
	 * criteria specified by the module parameters. If the mode is
	 * found - flag it as preferred. Otherwise create the preferred
	 * mode based on the module parameters criteria, and flag it as
	 * preferred.
	 */
	if (!nulldisp_set_preferred_mode(connector,
					 hdisplay,
					 vdisplay,
					 vrefresh))
		if (nulldisp_connector_add_preferred_mode(connector,
							  hdisplay,
							  vdisplay,
							  vrefresh))
			modes_count++;

	/* Sort the connector modes by relevance */
	drm_mode_sort(&connector->probed_modes);

	return modes_count;
}

static enum drm_mode_status
nulldisp_connector_helper_mode_valid(struct drm_connector *connector,
				     struct drm_display_mode *mode)
{
	/*
	 * This function is called on each gathered mode (e.g. via EDID)
	 * and gives the driver a chance to reject it if the hardware
	 * cannot support it.
	 */
	return MODE_OK;
}



static void nulldisp_connector_destroy(struct drm_connector *connector)
{
	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n",
			 connector->base.id,
			 connector->name);

	drm_connector_update_edid_property(connector, NULL);
	drm_connector_cleanup(connector);

	kfree(connector);
}

static void nulldisp_connector_force(struct drm_connector *connector)
{
}

static const struct drm_connector_helper_funcs
nulldisp_connector_helper_funcs = {
	.get_modes = nulldisp_connector_helper_get_modes,
	.mode_valid = nulldisp_connector_helper_mode_valid,
	/*
	 * For atomic, don't set atomic_best_encoder or best_encoder. This will
	 * cause the DRM core to fallback to drm_atomic_helper_best_encoder().
	 * This is fine as we only have a single connector and encoder.
	 */
};

static const struct drm_connector_funcs nulldisp_connector_funcs = {
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = nulldisp_connector_destroy,
	.force = nulldisp_connector_force,
};

static struct drm_connector *
nulldisp_connector_create(struct nulldisp_display_device *nulldisp_dev,
			  int type)
{
	struct drm_connector *connector;

	connector = kzalloc(sizeof(*connector), GFP_KERNEL);
	if (!connector)
		return NULL;

	drm_connector_init(nulldisp_dev->dev,
			   connector,
			   &nulldisp_connector_funcs,
			   type);
	drm_connector_helper_add(connector, &nulldisp_connector_helper_funcs);

	connector->dpms = DRM_MODE_DPMS_OFF;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;
	connector->display_info.subpixel_order = SubPixelUnknown;

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n",
			 connector->base.id,
			 connector->name);

	return connector;
}


/******************************************************************************
 * Encoder functions
 ******************************************************************************/

static void nulldisp_encoder_helper_dpms(struct drm_encoder *encoder,
					 int mode)
{
	/*
	 * Set the display power state or active encoder based on the mode. If
	 * the mode passed in is unsupported, the provider must use the next
	 * lowest power level.
	 */
}

static bool
nulldisp_encoder_helper_mode_fixup(struct drm_encoder *encoder,
				   const struct drm_display_mode *mode,
				   struct drm_display_mode *adjusted_mode)
{
	/*
	 * Fix up mode so that it's compatible with the hardware. The results
	 * should be stored in adjusted_mode (i.e. mode should be untouched).
	 */
	return true;
}

static void nulldisp_encoder_helper_prepare(struct drm_encoder *encoder)
{
	/*
	 * Prepare the encoder for a mode change e.g. set the active encoder
	 * accordingly/turn the encoder off
	 */
}

static void nulldisp_encoder_helper_commit(struct drm_encoder *encoder)
{
	/* Turn the encoder back on/set the active encoder */
}

static void
nulldisp_encoder_helper_mode_set(struct drm_encoder *encoder,
				 struct drm_display_mode *mode,
				 struct drm_display_mode *adjusted_mode)
{
	/* Setup the encoder for the new mode */
}

static void nulldisp_encoder_destroy(struct drm_encoder *encoder)
{
	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	drm_encoder_cleanup(encoder);
	kfree(encoder);
}

static const struct drm_encoder_helper_funcs nulldisp_encoder_helper_funcs = {
	.dpms = nulldisp_encoder_helper_dpms,
	.mode_fixup = nulldisp_encoder_helper_mode_fixup,
	.prepare = nulldisp_encoder_helper_prepare,
	.commit = nulldisp_encoder_helper_commit,
	.mode_set = nulldisp_encoder_helper_mode_set,
	.detect = NULL,
	.disable = NULL,
};

static const struct drm_encoder_funcs nulldisp_encoder_funcs = {
	.reset = NULL,
	.destroy = nulldisp_encoder_destroy,
};

static struct drm_encoder *
nulldisp_encoder_create(struct nulldisp_display_device *nulldisp_dev,
			int type)
{
	struct drm_encoder *encoder;
	int err;

	encoder = kzalloc(sizeof(*encoder), GFP_KERNEL);
	if (!encoder)
		return ERR_PTR(-ENOMEM);

	err = drm_encoder_init(nulldisp_dev->dev,
			       encoder,
			       &nulldisp_encoder_funcs,
			       type,
			       NULL);
	if (err) {
		DRM_ERROR("Failed to initialise encoder\n");
		return ERR_PTR(err);
	}
	drm_encoder_helper_add(encoder, &nulldisp_encoder_helper_funcs);

	/*
	 * This is a bit field that's used to determine which
	 * CRTCs can drive this encoder.
	 */
	encoder->possible_crtcs = 0x1;

	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	return encoder;
}


/******************************************************************************
 * Framebuffer functions
 ******************************************************************************/

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
static int
nulldisp_framebuffer_dirty(struct drm_framebuffer *framebuffer,
			   struct drm_file *file_priv,
			   unsigned int flags,
			   unsigned int color,
			   struct drm_clip_rect *clips,
			   unsigned int num_clips)
{
	struct nulldisp_display_device *nulldisp_dev =
		framebuffer->dev->dev_private;
	struct nulldisp_crtc *nulldisp_crtc = nulldisp_dev->nulldisp_crtc;

	reinit_completion(&nulldisp_crtc->copy_done);

	if (!nlpvrdpy_send_copy(nulldisp_dev->nlpvrdpy,
				framebuffer,
				&framebuffer->obj[0])) {
		unsigned long res;

		res = wait_for_completion_timeout(&nulldisp_crtc->copy_done,
						  nulldisp_netlink_timeout());

		if (!res)
			DRM_ERROR("timed out waiting for remote update\n");
	}

	return 0;
}

static const struct drm_framebuffer_funcs nulldisp_framebuffer_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
	.dirty = nulldisp_framebuffer_dirty,
};

static struct drm_framebuffer *
nulldisp_fb_create(struct drm_device *dev, struct drm_file *file,
		   const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return drm_gem_fb_create_with_funcs(dev, file, mode_cmd,
					    &nulldisp_framebuffer_funcs);
}
#else /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)) */
#define nulldisp_fb_create drm_gem_fb_create_with_dirty
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)) */

static const struct drm_mode_config_funcs nulldisp_mode_config_funcs = {
	.fb_create = nulldisp_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static int nulldisp_nl_flipped_cb(void *data)
{
	struct nulldisp_crtc *nulldisp_crtc = data;

	complete(&nulldisp_crtc->flip_done);
	flush_delayed_work(&nulldisp_crtc->vb_work);

	return 0;
}

static int nulldisp_nl_copied_cb(void *data)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
	struct nulldisp_crtc *nulldisp_crtc = data;

	complete(&nulldisp_crtc->copy_done);
#endif
	return 0;
}

static void nulldisp_nl_disconnect_cb(void *data)
{
	struct nulldisp_crtc *nulldisp_crtc = data;

	complete(&nulldisp_crtc->flip_done);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
	complete(&nulldisp_crtc->copy_done);
#endif
}

#if defined(LMA)
struct pdp_gem_private *pdp_gem_get_private(struct drm_device *dev)
{
	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

	return nulldisp_dev->pdp_gem_priv;
}
#endif

static int nulldisp_early_load(struct drm_device *dev, unsigned int instance)
{
	struct nulldisp_display_device *nulldisp_dev;
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	int err;

	platform_set_drvdata(to_platform_device(dev->dev), dev);

	nulldisp_dev = kzalloc(sizeof(*nulldisp_dev), GFP_KERNEL);
	if (!nulldisp_dev)
		return -ENOMEM;

	dev->dev_private = nulldisp_dev;
	nulldisp_dev->dev = dev;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
	atomic_set(&nulldisp_dev->open_count, 0);
#endif

	drm_mode_config_init(dev);

	dev->mode_config.funcs = (void *)&nulldisp_mode_config_funcs;
	dev->mode_config.min_width = NULLDISP_FB_WIDTH_MIN;
	dev->mode_config.max_width = NULLDISP_FB_WIDTH_MAX;
	dev->mode_config.min_height = NULLDISP_FB_HEIGHT_MIN;
	dev->mode_config.max_height = NULLDISP_FB_HEIGHT_MAX;
	dev->mode_config.async_page_flip = true;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
	dev->mode_config.allow_fb_modifiers = true;
#endif

	nulldisp_dev->nulldisp_crtc = nulldisp_crtc_create(nulldisp_dev);
	if (!nulldisp_dev->nulldisp_crtc) {
		DRM_ERROR("failed to create a CRTC.\n");

		err = -ENOMEM;
		goto err_config_cleanup;
	}

	connector = nulldisp_connector_create(nulldisp_dev,
					      DRM_MODE_CONNECTOR_Unknown);
	if (!connector) {
		DRM_ERROR("failed to create a connector.\n");

		err = -ENOMEM;
		goto err_config_cleanup;
	}
	encoder = nulldisp_encoder_create(nulldisp_dev,
					  DRM_MODE_ENCODER_NONE);
	if (IS_ERR(encoder)) {
		DRM_ERROR("failed to create an encoder.\n");

		err = PTR_ERR(encoder);
		goto err_config_cleanup;
	}

	err = drm_connector_attach_encoder(connector, encoder);
	if (err) {
		DRM_ERROR("failed to attach [ENCODER:%d:%s] to [CONNECTOR:%d:%s] (err=%d)\n",
			  encoder->base.id,
			  encoder->name,
			  connector->base.id,
			  connector->name,
			  err);
		goto err_config_cleanup;
	}

#if defined(LMA)
	nulldisp_dev->pdp_gem_priv = pdp_gem_init(dev, instance);
	if (!nulldisp_dev->pdp_gem_priv) {
		err = -ENOMEM;
		goto err_config_cleanup;
	}
#endif
	nulldisp_dev->workqueue =
		create_singlethread_workqueue(DRIVER_NAME);
	if (!nulldisp_dev->workqueue) {
		DRM_ERROR("failed to create work queue\n");
		goto err_gem_cleanup;
	}

	err = drm_vblank_init(nulldisp_dev->dev, 1);
	if (err) {
		DRM_ERROR("failed to complete vblank init (err=%d)\n", err);
		goto err_workqueue_cleanup;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
	dev->irq_enabled = true;
#endif

	nulldisp_dev->nlpvrdpy = nlpvrdpy_create(dev,
						 nulldisp_nl_disconnect_cb,
						 nulldisp_dev->nulldisp_crtc,
						 nulldisp_nl_flipped_cb,
						 nulldisp_dev->nulldisp_crtc,
						 nulldisp_nl_copied_cb,
						 nulldisp_dev->nulldisp_crtc);
	if (!nulldisp_dev->nlpvrdpy) {
		DRM_ERROR("Netlink initialisation failed (err=%d)\n", err);
		goto err_vblank_cleanup;
	}

	return 0;

err_vblank_cleanup:
err_workqueue_cleanup:
	destroy_workqueue(nulldisp_dev->workqueue);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
	dev->irq_enabled = false;
#endif
err_gem_cleanup:
#if defined(LMA)
	pdp_gem_cleanup(nulldisp_dev->pdp_gem_priv);
#endif
err_config_cleanup:
	drm_mode_config_cleanup(dev);
	kfree(nulldisp_dev);
	return err;
}

static int nulldisp_late_load(struct drm_device *dev)
{
	drm_mode_config_reset(dev);

	return 0;
}

static void nulldisp_early_unload(struct drm_device *dev)
{
}

static void nulldisp_late_unload(struct drm_device *dev)
{
	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

	nlpvrdpy_send_disconnect(nulldisp_dev->nlpvrdpy);
	nlpvrdpy_destroy(nulldisp_dev->nlpvrdpy);

	destroy_workqueue(nulldisp_dev->workqueue);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
	dev->irq_enabled = false;
#endif

#if defined(LMA)
	pdp_gem_cleanup(nulldisp_dev->pdp_gem_priv);
#endif
	drm_mode_config_cleanup(dev);

	kfree(nulldisp_dev);
}

static void nulldisp_lastclose(struct drm_device *dev)
{
	drm_atomic_helper_shutdown(dev);
}

#if defined(LMA)
static const struct vm_operations_struct nulldisp_gem_vm_ops = {
	.fault	= pdp_gem_object_vm_fault,
	.open	= drm_gem_vm_open,
	.close	= drm_gem_vm_close,
};

static int pdp_gem_dumb_create(struct drm_file *file,
			       struct drm_device *dev,
			       struct drm_mode_create_dumb *args)
{
	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

	return pdp_gem_dumb_create_priv(file,
					dev,
					nulldisp_dev->pdp_gem_priv,
					args);
}

static int nulldisp_gem_object_create_ioctl(struct drm_device *dev,
					    void *data,
					    struct drm_file *file)
{
	struct drm_nulldisp_gem_create *args = data;
	struct nulldisp_display_device *nulldisp_dev = dev->dev_private;
	struct drm_pdp_gem_create pdp_args;
	int err;

	if (args->flags) {
		DRM_ERROR("invalid flags: %#08x\n", args->flags);
		return -EINVAL;
	}

	if (args->handle) {
		DRM_ERROR("invalid handle (this should always be 0)\n");
		return -EINVAL;
	}

	/*
	 * Remapping of nulldisp create args to pdp create args.
	 *
	 * Note: even though the nulldisp and pdp args are identical
	 * in this case, they may potentially change in future.
	 */
	pdp_args.size = args->size;
	pdp_args.flags = args->flags;
	pdp_args.handle = args->handle;

	err = pdp_gem_object_create_ioctl_priv(dev,
					       nulldisp_dev->pdp_gem_priv,
					       &pdp_args,
					       file);
	if (!err)
		args->handle = pdp_args.handle;

	return err;
}

static int nulldisp_gem_object_mmap_ioctl(struct drm_device *dev,
					  void *data,
					  struct drm_file *file)
{
	struct drm_nulldisp_gem_mmap *args = data;
	struct drm_pdp_gem_mmap pdp_args;
	int err;

	pdp_args.handle = args->handle;
	pdp_args.pad = args->pad;
	pdp_args.offset = args->offset;

	err = pdp_gem_object_mmap_ioctl(dev, &pdp_args, file);

	if (!err)
		args->offset = pdp_args.offset;

	return err;
}

static int nulldisp_gem_object_cpu_prep_ioctl(struct drm_device *dev,
					      void *data,
					      struct drm_file *file)
{
	struct drm_nulldisp_gem_cpu_prep *args =
		(struct drm_nulldisp_gem_cpu_prep *)data;
	struct drm_pdp_gem_cpu_prep pdp_args;

	pdp_args.handle = args->handle;
	pdp_args.flags = args->flags;

	return pdp_gem_object_cpu_prep_ioctl(dev, &pdp_args, file);
}

static int nulldisp_gem_object_cpu_fini_ioctl(struct drm_device *dev,
				       void *data,
				       struct drm_file *file)
{
	struct drm_nulldisp_gem_cpu_fini *args =
		(struct drm_nulldisp_gem_cpu_fini *)data;
	struct drm_pdp_gem_cpu_fini pdp_args;

	pdp_args.handle = args->handle;
	pdp_args.pad = args->pad;

	return pdp_gem_object_cpu_fini_ioctl(dev, &pdp_args, file);
}

void pdp_gem_object_free(struct drm_gem_object *obj)
{
	struct nulldisp_display_device *nulldisp_dev = obj->dev->dev_private;

	pdp_gem_object_free_priv(nulldisp_dev->pdp_gem_priv, obj);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
const struct drm_gem_object_funcs nulldisp_gem_funcs = {
	.free = pdp_gem_object_free,
	.export = pdp_gem_prime_export,
	.vm_ops = &nulldisp_gem_vm_ops,
};
#endif
#endif /* defined(LMA) */

static const struct drm_ioctl_desc nulldisp_ioctls[] = {
	DRM_IOCTL_DEF_DRV(NULLDISP_GEM_CREATE,
			  nulldisp_gem_object_create_ioctl,
			  DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NULLDISP_GEM_MMAP,
			  nulldisp_gem_object_mmap_ioctl,
			  DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NULLDISP_GEM_CPU_PREP,
			  nulldisp_gem_object_cpu_prep_ioctl,
			  DRM_AUTH),
	DRM_IOCTL_DEF_DRV(NULLDISP_GEM_CPU_FINI,
			  nulldisp_gem_object_cpu_fini_ioctl,
			  DRM_AUTH),
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
static int nulldisp_drm_open(struct inode *inode, struct file *filp)
{
	int ret = drm_open(inode, filp);

	if (!ret) {
		struct drm_file *file_priv = filp->private_data;
		struct drm_device *dev = file_priv->minor->dev;
		struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

		atomic_fetch_inc(&nulldisp_dev->open_count);
	}
	return ret;
}

static int nulldisp_drm_release(struct inode *inode, struct file *filp)
{
	int ret = drm_release(inode, filp);

	if (!ret) {
		struct drm_file *file_priv = filp->private_data;
		struct drm_device *dev = file_priv->minor->dev;
		struct nulldisp_display_device *nulldisp_dev = dev->dev_private;

		if (atomic_dec_and_test(&nulldisp_dev->open_count))
			nulldisp_lastclose(dev);
	}
	return ret;
}
#endif

static int nulldisp_gem_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = netlink_gem_mmap(file, vma);
#if !defined(LMA)
	if (!err) {
		struct drm_file *file_priv = file->private_data;
		struct drm_device *dev = file_priv->minor->dev;
		struct drm_gem_object *obj;

		mutex_lock(&dev->struct_mutex);
		obj = vma->vm_private_data;

		if (obj->import_attach)
			err = dma_buf_mmap(obj->dma_buf, vma, 0);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		else
			err = nulldisp_gem_object_get_pages(obj);
#endif

		mutex_unlock(&dev->struct_mutex);
	}
#endif
	return err;
}

static const struct file_operations nulldisp_driver_fops = {
	.owner		= THIS_MODULE,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
	.open		= nulldisp_drm_open,
	.release	= nulldisp_drm_release,
#else
	.open		= drm_open,
	.release	= drm_release,
#endif
	.unlocked_ioctl	= drm_ioctl,
	.mmap		= nulldisp_gem_mmap,
	.poll		= drm_poll,
	.read		= drm_read,
	.llseek		= noop_llseek,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= drm_compat_ioctl,
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0))
	.fop_flags	= FOP_UNSIGNED_OFFSET,
#endif
};

static struct drm_driver nulldisp_drm_driver = {
	.load				= NULL,
	.unload				= NULL,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
	.lastclose			= nulldisp_lastclose,
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
	.enable_vblank			= nulldisp_enable_vblank,
	.disable_vblank			= nulldisp_disable_vblank,
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	.prime_handle_to_fd		= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle		= drm_gem_prime_fd_to_handle,
#endif

#if defined(LMA)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	.gem_free_object		= pdp_gem_object_free,
	.gem_prime_export		= pdp_gem_prime_export,
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0) */
	.gem_prime_import		= pdp_gem_prime_import,
	.gem_prime_import_sg_table	= pdp_gem_prime_import_sg_table,

	.dumb_create			= pdp_gem_dumb_create,
	.dumb_map_offset		= pdp_gem_dumb_map_offset,
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	.gem_prime_pin			= nulldisp_gem_prime_pin,
	.gem_prime_unpin		= nulldisp_gem_prime_unpin,
	.gem_prime_get_sg_table = nulldisp_gem_prime_get_sg_table,
	.gem_prime_vmap			= nulldisp_gem_prime_vmap,
	.gem_prime_vunmap		= nulldisp_gem_prime_vunmap,
	.gem_free_object		= nulldisp_gem_object_free,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	.gem_prime_export		= nulldisp_gem_prime_export,
#else
	.gem_prime_export		= drm_gem_prime_export,
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) */
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0) */
	.gem_prime_import_sg_table	= nulldisp_gem_prime_import_sg_table,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	.gem_prime_mmap			= nulldisp_gem_prime_mmap,
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	.gem_prime_mmap			= drm_gem_prime_mmap,
#endif
	.gem_prime_import		= drm_gem_prime_import,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	.gem_prime_res_obj		= nulldisp_gem_prime_res_obj,
#endif
	.dumb_create			= nulldisp_gem_dumb_create,
#endif /* defined(LMA) */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	.gem_vm_ops			= &nulldisp_gem_vm_ops,
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0) */
	.name				= DRIVER_NAME,
	.desc				= DRIVER_DESC,
	.date				= DRIVER_DATE,
	.major				= PVRVERSION_MAJ,
	.minor				= PVRVERSION_MIN,
	.patchlevel			= PVRVERSION_BUILD,

	.driver_features		= DRIVER_GEM |
					  DRIVER_MODESET |
					  DRIVER_ATOMIC |
					  NULLDISP_DRIVER_PRIME,
	.ioctls				= nulldisp_ioctls,
	.num_ioctls			= ARRAY_SIZE(nulldisp_ioctls),
	.fops				= &nulldisp_driver_fops,
};

static int nulldisp_probe(struct platform_device *pdev)
{
	unsigned int instance = 0;
	struct drm_device *ddev;
	int ret;

	ddev = drm_dev_alloc(&nulldisp_drm_driver, &pdev->dev);
	if (IS_ERR(ddev))
		return PTR_ERR(ddev);

	/*
	 * The load callback, called from drm_dev_register, is deprecated,
	 * because of potential race conditions.
	 */
	BUG_ON(nulldisp_drm_driver.load != NULL);

#if defined(CONFIG_OF)
	/* The device is not spawned from a device tree: arch_setup_dma_ops
	 * was not called and the device has dummy DMA ops. Call of_dma_configure()
	 * to setup proper dma ops.
	 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
	of_dma_configure(ddev->dev, NULL);
#else
	of_dma_configure(ddev->dev, NULL, true);
#endif
#endif

	ret = nulldisp_early_load(ddev, instance);
	if (ret)
		goto err_drm_dev_put;

	ret = drm_dev_register(ddev, 0);
	if (ret)
		goto err_drm_dev_late_unload;

	ret = nulldisp_late_load(ddev);
	if (ret)
		goto err_drm_dev_unregister;

	drm_mode_config_reset(ddev);

	return 0;

err_drm_dev_unregister:
	drm_dev_unregister(ddev);
err_drm_dev_late_unload:
	nulldisp_late_unload(ddev);
err_drm_dev_put:
	drm_dev_put(ddev);
	return	ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
static int nulldisp_remove(struct platform_device *pdev)
#else
static void nulldisp_remove(struct platform_device *pdev)
#endif
{
	struct drm_device *ddev = platform_get_drvdata(pdev);

	/*
	 * The unload callback, called from drm_dev_unregister, is
	 * deprecated.
	 */
	BUG_ON(nulldisp_drm_driver.unload != NULL);

	nulldisp_early_unload(ddev);

	drm_dev_unregister(ddev);

	nulldisp_late_unload(ddev);

	drm_dev_put(ddev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
	return 0;
#endif
}

static void nulldisp_shutdown(struct platform_device *pdev)
{
	drm_atomic_helper_shutdown(platform_get_drvdata(pdev));
}

static struct platform_device_id nulldisp_platform_device_id_table[] = {
#if defined(LMA) && !defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
	{ .name = APOLLO_DEVICE_NAME_PDP, .driver_data = 0 },
	{ .name = ODN_DEVICE_NAME_PDP, .driver_data = 0 },
#else
	{ .name = "nulldisp", .driver_data = 0 },
#endif
	{ },
};

static struct platform_driver nulldisp_platform_driver = {
	.probe		= nulldisp_probe,
	.remove		= nulldisp_remove,
	.shutdown	= nulldisp_shutdown,
	.driver		= {
		.owner  = THIS_MODULE,
		.name	= DRIVER_NAME,
	},
	.id_table	= nulldisp_platform_device_id_table,
};


#if !defined(LMA) || defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
static struct platform_device_info nulldisp_device_info = {
	.name		= "nulldisp",
	.id		= -1,
#if defined(LMA)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-count-overflow"
#endif /* defined(__clang__) */
	/* No restriction, the GPU can access all on-card memory */
	.dma_mask	= DMA_BIT_MASK(64),
#if defined(__clang__)
#pragma clang diagnostic pop
#endif /* defined(__clang__) */
#elif defined(NULLDISP_PHYS_BUS_WIDTH)
	.dma_mask	= DMA_BIT_MASK(NULLDISP_PHYS_BUS_WIDTH),
#elif defined(NO_HARDWARE)
	/*
	 * Not all cores have 40 bit physical support, but this
	 * will work unless > 32 bit address is returned on those cores.
	 * In the future this will be fixed properly.
	 */
	.dma_mask	= DMA_BIT_MASK(40),
#else
	.dma_mask	= DMA_BIT_MASK(32),
#endif
};

static struct platform_device *nulldisp_dev;
#endif

static int __init nulldisp_init(void)
{
	int err;

	err = nulldisp_validate_module_parameters();
	if (err) {
		DRM_ERROR("invalid module parameters (err=%d)\n", err);
		return err;
	}

	err = nlpvrdpy_register();
	if (err) {
		DRM_ERROR("failed to register with netlink (err=%d)\n", err);
		return err;
	}

#if !defined(LMA) || defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
	nulldisp_dev = platform_device_register_full(&nulldisp_device_info);
	if (IS_ERR(nulldisp_dev)) {
		err = PTR_ERR(nulldisp_dev);
		nulldisp_dev = NULL;
		goto err_unregister_family;
	}
#endif
	err = platform_driver_register(&nulldisp_platform_driver);
	if (err)
		goto err_unregister_family;

	return 0;

err_unregister_family:
		(void) nlpvrdpy_unregister();
		return err;
}

static void __exit nulldisp_exit(void)
{
	int err;

	err = nlpvrdpy_unregister();
	BUG_ON(err);

#if !defined(LMA) || defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
	if (nulldisp_dev)
		platform_device_unregister(nulldisp_dev);
#endif
	platform_driver_unregister(&nulldisp_platform_driver);
}

module_init(nulldisp_init);
module_exit(nulldisp_exit);

#if defined(LMA) && !defined(SUPPORT_EXTERNAL_PHYSHEAP_INTERFACE)
/*
 * For Test Chip, this module relies on a memory heap created in another
 * module. There is no explicit dependency on the other module, as the heap
 * information is passed in device data. This lack of a dependency would
 * allow the other module to be unloaded whilst the heap is still in use,
 * resulting in the PDP platform device being removed whilst there are
 * resources still associated with it. Avoid the problem by creating a
 * dependency on the other module, by referencing a symbol in it.
 */
static int (*nulldisp_parent_mod_ref)(struct device *) __used = tc_enable;
#endif

MODULE_AUTHOR("Imagination Technologies Ltd. <gpl-support@imgtec.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual MIT/GPL");
