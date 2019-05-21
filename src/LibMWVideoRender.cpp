// LibMWVideoRender.cpp : Defines the exported functions for the DLL application.
//
#if defined(_MSC_VER)
#include "stdafx.h"
#endif

#include "MWVideoRender.h"
#include "LibMWVideoRender.h"

#include "mwvideoframebuffer.h"
#include "mwcalacsize.h"

CMWVideoFrameBuffer m_frame_buffer;




HMWVIDEORENDER
LIBMWVIDEORENDER_API 
MWCreateVideoRender()
{
	CGLRender *t_videorender=new CGLRender();
	return (HMWVIDEORENDER)t_videorender;
}

void
LIBMWVIDEORENDER_API 
MWDestoryVideoRender(HMWVIDEORENDER t_hvideorender)
{
	CGLRender *t_videorender=(CGLRender*)t_hvideorender;
	if(t_videorender!=NULL){
		t_videorender->Close();
		delete t_videorender;
	}
}

mw_videorender_result_e
LIBMWVIDEORENDER_API
MWInitRender(
	HMWVIDEORENDER t_hvideorender,
	mw_init_setting_t *t_setting,
	HWND t_hwnd,
	bool t_b_glew
)
{
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	if (t_hwnd.win) {
#else
	if (t_hwnd){
#endif
		t_rslt_e = t_videorender->init_context(t_hwnd);
		if (t_rslt_e != MW_VIDEORENDER_NO_ERROR)
			return t_rslt_e;
	}

	if (t_b_glew)
		t_rslt_e = t_videorender->open_with_glew(t_setting);
	else
		t_rslt_e = t_videorender->open(t_setting);

	return t_rslt_e;
}

void
LIBMWVIDEORENDER_API 
MWDeInitRender(HMWVIDEORENDER t_hvideorender)
{
	CGLRender *t_videorender=(CGLRender*)t_hvideorender;
	if(t_videorender!=NULL)
		t_videorender->Close();	
}

mw_videorender_result_e
LIBMWVIDEORENDER_API
MWCreateFrameBuffer(
	HMWVIDEORENDER t_hvideorender,
	uint32_t t_u32_count,
	mw_queue_mode_e t_map_mode
)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (NULL == t_videorender)
		return MW_VIDEORENDER_RENDER_NULL;
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = 
		t_videorender->create_framebuffer(
			t_u32_count, 
			t_map_mode
		);
	return t_rslt_e;
}

mw_videorender_result_e
LIBMWVIDEORENDER_API
MWDestoryFrameBuffer(HMWVIDEORENDER t_hvideorender)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;

	t_videorender->destory_framebuffer();
}

mw_videorender_result_e
LIBMWVIDEORENDER_API
MWRenderFrame(
	HMWVIDEORENDER t_hvideorender,
	void *t_p_data,
	mw_render_ctrl_setting_t t_setting,
	uint32_t t_u32_glvar,
	uint32_t t_u32_gldefault,
	bool t_b_buffer
)
{
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;

	if (t_b_buffer)
		t_rslt_e = 
			t_videorender->render_frame(
				&t_setting, 
				(GLuint)t_u32_glvar, 
				(GLuint)t_u32_gldefault
			);
	else
		t_rslt_e = 
			t_videorender->render(
				t_p_data, 
				&t_setting, 
				(GLuint)t_u32_glvar, 
				(GLuint)t_u32_gldefault
			);
	
	return t_rslt_e;
}

mw_videorender_result_e
LIBMWVIDEORENDER_API 
MWDisplayRenderBuffer(
	HMWVIDEORENDER t_hvideorender, 
	mw_display_mode_e t_mode
)
{
	CGLRender *t_videorender=(CGLRender*)t_hvideorender;
	if(t_videorender==NULL)
		return MW_VIDEORENDER_RENDER_NULL;
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = t_videorender->display(t_mode);
	return t_rslt_e;
}

mw_videorender_result_e
LIBMWVIDEORENDER_API 
MWDisplaySetInterval(
	HMWVIDEORENDER t_hvideorender, 
	mw_display_mode_e t_display_mode
)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = t_videorender->setinterval(t_display_mode);
	return t_rslt_e;
}

mw_videorender_result_e
LIBMWVIDEORENDER_API 
MWPutFrameBufferQueue(
	HMWVIDEORENDER t_hvideorender, 
	void *t_p_frame_data, 
	uint32_t t_u32_size, 
	uint32_t t_u32_width,
	uint32_t t_u32_height
)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;


	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = 
		t_videorender->putframedata(
			t_p_frame_data, 
			t_u32_size, 
			t_u32_width, 
			t_u32_height
		);
		return t_rslt_e;
}


mw_videorender_result_e
LIBMWVIDEORENDER_API 
MWVideoScreenShot(
	HMWVIDEORENDER t_hvideorender, 
	mw_screen_shot_t *t_screenshot
)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;

	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = t_videorender->screenshot(t_screenshot);
	return t_rslt_e;
}



mw_videorender_result_e
LIBMWVIDEORENDER_API 
MWVideoDeviceInfo(
	HMWVIDEORENDER t_hvideorender, 
	mw_device_info_t *t_deviceinfo, 
	HWND t_hwnd, 
	bool t_b_glew
)
{
	CGLRender *t_videorender = (CGLRender*)t_hvideorender;
	if (t_videorender == NULL)
		return MW_VIDEORENDER_RENDER_NULL;

	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	if (t_hwnd.win) {
#else
	if (t_hwnd) {
#endif
		t_rslt_e = t_videorender->init_context(t_hwnd);
		if (t_rslt_e != MW_VIDEORENDER_NO_ERROR)
			return t_rslt_e;
	}

	if (t_b_glew)
		glewInit();


	t_rslt_e = t_videorender->deviceinfo(t_deviceinfo);
	return t_rslt_e;
}
