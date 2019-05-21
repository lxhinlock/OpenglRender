#ifndef LIBMWVIDEORENDER_H
#define LIBMWVIDEORENDER_H

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#define LIBMWVIDEORENDER_API 
#else
#ifdef LIBMWVIDEORENDER_EXPORTS
#define LIBMWVIDEORENDER_API __declspec(dllexport)
#else
#define LIBMWVIDEORENDER_API __declspec(dllimport)
#endif
#endif

#include <stdint.h>

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
#ifdef _DEBUG
//#include "GL/GL.h"
#endif // _DEBUG
#endif

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

typedef struct HWND
{
	Window win;
	Display *dpy;
	XVisualInfo *vi;
}HWND;

#endif

#ifdef __cplusplus

typedef enum _display_mode{
	MW_DSP_AUTO = 0,
	MW_DSP_VSYNC_ON,
	MW_DSP_VSYNC_OFF,
	MW_DSP_LOW_LATENCY,
	MW_DSP_COUNT,
}mw_display_mode_e;

typedef enum _color_space {
	MW_CSP_AUTO = 0,
	MW_CSP_BT_601,
	MW_CSP_BT_709,
	MW_CSP_BT_2020,
	MW_CSP_COUNT,
}mw_color_space_e;

typedef enum _color_space_level {
	MW_CSP_LEVELS_AUTO = 0,
	MW_CSP_LEVELS_TV,
	MW_CSP_LEVELS_PC,
	MW_CSP_LEVELS_COUNT,
}mw_color_space_level_e;

typedef enum _map_mode {
	MW_MAP_MODE_AUTO = 0,
	MW_INTEL_MAP_TEXT = 1,
	MW_AMD_MAP_BUF = 2,
	MW_GL_MAP_BUF = 4,
	MW_NVIDIA_MAP_TEXT = 8,
	MW_MAP_MODE_COUNT,
}mw_map_mode_e;


typedef enum _queue_mode {
	MW_QUEUE_AUTO = 0,
	MW_QUEUE_IMMEDIATE,
	MW_QUEUE_MAILBOX,
	MW_QUEUE_RELAXED_FIFO,
	MW_QUEUE_FIFO,
	MW_QUEUE_COUNT
}mw_queue_mode_e;


typedef enum _videorender_result{
	MW_VIDEORENDER_NO_ERROR=0,
	MW_VIDEORENDER_BUF_NUM_OUTOFRANGE,
	MW_VIDEORENDER_BUF_CREATE_FAIL,
	MW_VIDEORENDER_BUF_RENDER_NOT_OPENED,
	MW_VIDEORENDER_BUF_NONE,
	MW_VIDEORENDER_COLORFORMAT_ERROR,
	MW_VIDEORENDER_MAP_MODE_ERROR,

	MW_VIDEORENDER_GEN_TEXTURE_ERROR=10,
	MW_VIDEORENDER_MAPBUFFER_ERROR,
	MW_VIDEORENDER_CREATE_SHADER_ERROR,
	MW_VIDEORENDER_COMPILE_SHADER_ERROR,
	MW_VIDEORENDER_LINK_ERROR,
	MW_VIDEORENDER_GEN_FBO_ERROR,
	MW_VIDEORENDER_GEN_RBO_ERROR,
	MW_VIDEORENDER_SIZE_ERROR,

	MW_VIDEORENDER_SETPIXELFORMAT_ERROR=20,
	MW_VIDEORENDER_SETCONTEXT_ERROR,
	MW_VIDEORENDER_RENDER_NULL,
	MW_VIDEORENDER_DATA_NULL,
	MW_VIDEORENDER_COLOR_CTRL_ERROR,

	MW_VIDEORENDER_GLVAR_ERROR=30,
	MW_VIDEORENDER_RENDER_DATA_NULL,
	MW_VIDEORENDER_GLVERSION_ERROR,
	MW_VIDEORENDER_GLVERSION_UNSUPPORT,
	MW_VIDEORENDER_COLOR_SPACE_MODE_ERROR,
	MW_VIDEORENDER_COLOR_SPACE_LEVEL_ERROR,

	MW_VIDEORENDER_CUDA_ERROR = 40,
	MW_VIDEORENDER_CUDA_NODEVICE,
	MW_VIDEORENDER_BUF_ERR
}mw_videorender_result_e;

typedef enum _colorformat {
	MW_RENDER_AUTO  = 0,
	MW_RENDER_BGR24 = 1,
	MW_RENDER_RGBA  = 2,
	MW_RENDER_RGB10 = 3,
	MW_RENDER_BGRA  = 4,
	MW_RENDER_YUY2  = 10,
	MW_RENDER_NV12  = 11,
	MW_RENDER_P010  = 12,
	MW_RENDER_I420  = 13,
	MW_RENDER_I422  = 14,
	MW_RENDER_UYVY  = 15,
	MW_RENDER_V308  = 16,
	MW_RENDER_V408  = 17,
	MW_RENDER_NV16  = 18,
	MW_RENDER_P210  = 19,
	MW_RENDER_Y410  = 20,
	MW_RENDER_COUNT,
}mw_colorformat_e;

typedef struct _screen_shot {
	uint32_t m_u32_x;
	uint32_t m_u32_y;
	uint32_t m_u32_width;
	uint32_t m_u32_height;
	void *m_ptr;
}mw_screen_shot_t;


typedef struct _device_info {
	uint32_t m_u32_glver;
	uint32_t m_u32_glslver;
	uint32_t m_u32_intel_map;
	uint32_t m_u32_amd_map;
	uint32_t m_u32_gl_map;
	uint32_t m_u32_nvidia_map;
}mw_device_info_t;

typedef struct _init_setting{
	bool m_b_reverse;
	mw_colorformat_e m_format;
	uint32_t m_u32_width;
	uint32_t m_u32_height;
	uint32_t m_u32_buf_num;
	mw_color_space_e m_csp;
	mw_map_mode_e m_map_mode;
	mw_color_space_level_e m_csp_lvl_in;
	mw_color_space_level_e m_csp_lvl_out;
	void **m_pptr;
	uint32_t *m_glptr;
	bool m_b_resize;
}mw_init_setting_t;

typedef struct _render_ctrl_setting
{
	uint32_t m_u32_display_w;
	uint32_t m_u32_display_h;
	uint32_t m_u32_width;
	uint32_t m_u32_height;
	bool  m_b_hdr;
	uint8_t  m_u8_val_ctrl;
	uint8_t  m_u8_threshold;
	float m_f32_hue;
	float m_f32_saturate;
	float m_f32_value;
}mw_render_ctrl_setting_t;

extern "C"
{
#endif

	/////////////////////////
	// MWVIDEORENDER


#ifdef LIBMWVIDEORENDER_LINUX_DEF
	typedef void*   HMWVIDEORENDER;  

#else
	DECLARE_HANDLE(HMWVIDEORENDER); 
#endif

	HMWVIDEORENDER
	LIBMWVIDEORENDER_API
	MWCreateVideoRender();

	void
	LIBMWVIDEORENDER_API
	MWDestoryVideoRender(
		HMWVIDEORENDER t_hvideorender
	);

	void
	LIBMWVIDEORENDER_API
	MWDeInitRender(
		HMWVIDEORENDER t_hvideorender
	);

	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWCreateFrameBuffer(
		HMWVIDEORENDER t_hvideorender,
		uint32_t t_u32_count,
		mw_queue_mode_e t_map_mode=MW_QUEUE_FIFO
	);

	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWDestoryFrameBuffer(
		HMWVIDEORENDER t_hvideorender
	);


	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWInitRender(
		HMWVIDEORENDER t_hvideorender,
		mw_init_setting_t *t_setting,
		HWND t_hwnd,
		bool t_b_glew
	);


	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWRenderFrame(
		HMWVIDEORENDER t_hvideorender,
		void *t_p_data,
		mw_render_ctrl_setting_t t_setting,
		uint32_t t_u32_glvar,
		uint32_t t_u32_gldefault=0,
		bool t_b_buffer=false
	);

	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWDisplayRenderBuffer(
		HMWVIDEORENDER t_hvideorender,
		mw_display_mode_e t_mode
	);

	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWDisplaySetInterval(
		HMWVIDEORENDER t_hvideorender,
		mw_display_mode_e t_display_mode
	);


	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWPutFrameBufferQueue(
		HMWVIDEORENDER t_hvideorender,
		void *t_p_frame_data,
		uint32_t t_u32_size,
		uint32_t t_u32_width=1920,
		uint32_t t_u32_height=1080
	);

	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWVideoScreenShot(
		HMWVIDEORENDER t_hvideorender,
		mw_screen_shot_t *t_screenshot
	);



	mw_videorender_result_e
	LIBMWVIDEORENDER_API
	MWVideoDeviceInfo(
		HMWVIDEORENDER t_hvideorender,
		mw_device_info_t *t_deviceinfo,
		HWND t_hwnd,
		bool t_b_glew
	);



#ifdef __cplusplus
}
#endif

#endif
