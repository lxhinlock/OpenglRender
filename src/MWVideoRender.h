#ifndef GLLOWLATENCY_H
#define GLLOWLATENCY_H


#include <stdint.h>

#ifdef GLFW_IMGUI  
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#else  
#include <GL/glew.h> 

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
#include <Windows.h>
#endif
#include "LibMWVideoRender.h"

#endif	

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
#include "cuda_runtime.h"
#include "cuda_gl_interop.h"
#include "helper_cuda.h"
#endif

#include "mwvideoframebuffer.h"
#include "mwcalacsize.h"

#define ALIGN_SIZE 4096

typedef struct _yuvlevels { double ymin, ymax, cmax, cmid; }yuvlevels_t;

typedef struct _rgblevels { double min, max; }rgblevels_t;

class CGLRender
{
public:
	CGLRender();
	~CGLRender();

public:
	
	mw_videorender_result_e deviceinfo(mw_device_info_t *pt_device_info);

	mw_videorender_result_e open(mw_init_setting_t *pt_rinit);

	mw_videorender_result_e open_with_glew(mw_init_setting_t *pt_init);

	void Close();

	mw_videorender_result_e render(void *p_data, mw_render_ctrl_setting_t *pt_ctrl, GLuint glu32_var, GLuint glu32_def=0);

	mw_videorender_result_e init_context(HWND t_hwnd);

	mw_videorender_result_e display(mw_display_mode_e e_mode);

	mw_videorender_result_e setinterval(mw_display_mode_e e_mode);

	mw_videorender_result_e screenshot(mw_screen_shot_t *pt_screenshot);

	mw_videorender_result_e create_framebuffer(uint32_t u32_count, mw_queue_mode_e e_map_mode);

	mw_videorender_result_e putframedata(void *p_data, uint32_t u32_size, uint32_t u32_width, uint32_t u32_height);

	mw_videorender_result_e render_frame(mw_render_ctrl_setting_t *pt_ctrl, GLuint glu32_var, GLuint glu32_def=0);

	void destory_framebuffer();


private:
	void CleanUp();

	mw_videorender_result_e GlVision(uint32_t *pu32_gl_version);

	mw_videorender_result_e GlslVision(uint32_t *pu32_glsl_version);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	mw_videorender_result_e nvGetMaxGflopsDeviceId(uint32_t *pu32_device);
#endif	
protected:

	HWND m_hwnd;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	Window m_win;
	Display *m_dpy;
	XVisualInfo *m_vi;
	GLXContext m_glc;
#else	
	HDC m_hdc;
	HGLRC m_hglrc;
#endif

private:
	GLuint glu32_tex;
	GLuint glu32_tex1;
	GLuint glu32_tex2;


	GLuint glu32_fbo;
	mw_colorformat_e e_render_mode;
	GLuint glu32_rbo;
	GLint  gli32_v;
	GLint  gli32_f;
	GLuint glu32_program;
	GLuint glu32_verLocation;
	GLuint glu32_texLocation0;
	GLuint glu32_texLocation1;
	GLuint glu32_texLocation2;

	uint32_t u32_tex_width;
	uint32_t u32_tex_height;

	uint32_t u32_ini_width;
	uint32_t u32_ini_height;

	GLuint glu32_buf[10];
	void **pptr;


	bool b_need_rev;
	uint32_t u32_gl_version;
	uint32_t u32_glsl_version;

	uint32_t gl_map_mode;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	cudaGraphicsResource *cuda_tex_result_resource;
#endif
	mw_color_space_e e_csp;
	mw_color_space_level_e e_csp_lvl_in;
	mw_color_space_level_e e_csp_lvl_out;
	float f32_hue;
	float f32_saturate;
	float f32_value;

	bool b_hdr;
	uint8_t  u8_val_ctrl;
	uint8_t  u8_threshold;
	GLint gli32_max_texture_size;
	CMWVideoFrameBuffer *pc_framebuffer;
	bool	b_opened;

	bool b_resize;
	bool b_alloc_buf;
	uint8_t u8_buf_num;
};


static char *vert_str =
"#version 110 \n"
"attribute vec2 vertexIn; \n"
"attribute vec2 textureIn0; \n"
"attribute vec2 textureIn1; \n"
"attribute vec2 textureIn2; \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"void main(void) \n"
"{ \n"
"  gl_Position = vec4(vertexIn, 0.0, 1); \n"
"  textureOut0 = textureIn0; \n"
"  textureOut1 = textureIn1; \n"
"  textureOut2 = textureIn2; \n"
"} \n";

static char *frag_str_BGR24 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b; \n"
"  r = texture2D(texture, textureOut0).b; \n"
"  g = texture2D(texture, textureOut0).g; \n"
"  b = texture2D(texture, textureOut0).r; \n"
"  gl_FragColor=vec4(r, g, b, 1.0); \n"
"} \n";

static char *frag_str_RGBA_RGB10 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, a; \n"
"  r = texture2D(texture, textureOut0).r; \n"
"  g = texture2D(texture, textureOut0).g; \n"
"  b = texture2D(texture, textureOut0).b; \n"
"  a = texture2D(texture, textureOut0).a; \n"
"  gl_FragColor=vec4(r, g, b, a); \n"
"} \n";

static char *frag_str_BGRA =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, a; \n"
"  r = texture2D(texture, textureOut0).b; \n"
"  g = texture2D(texture, textureOut0).g; \n"
"  b = texture2D(texture, textureOut0).r; \n"
"  a = texture2D(texture, textureOut0).a; \n"
"  gl_FragColor=vec4(r, g, b, a); \n"
"} \n";

static char *frag_str_YUY2 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform sampler2D texture1; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, my, mu, mv; \n"
"  vec3 rgb; \n"
"  my = texture2D(texture, textureOut0).r; \n"
"  mu = texture2D(texture1, textureOut1).g; \n"
"  mv = texture2D(texture1, textureOut1).a; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, 1.0); \n"
"} \n";

static char *frag_str_NV12_P010_NV16_P210 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform sampler2D texture1; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, my, mu, mv; \n"
"  vec3 rgb; \n"
"  my = texture2D(texture, textureOut0).r; \n"
"  mu = texture2D(texture1, textureOut1).r; \n"
"  mv = texture2D(texture1, textureOut1).a; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, 1.0); \n"
"} \n";

static char *frag_str_I420_I422 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform sampler2D texture1; \n"
"uniform sampler2D texture2; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, my, mu, mv; \n"
"  vec3 rgb; \n"
"  my = texture2D(texture, textureOut0).r; \n"
"  mu = texture2D(texture1, textureOut1).r; \n"
"  mv = texture2D(texture2, textureOut2).r; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, 1.0); \n"
"} \n";

static char *frag_str_UYVY =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform sampler2D texture1; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, my, mu, mv; \n"
"  vec3 rgb; \n"
"  my =texture2D(texture, textureOut0).a; \n"
"  mu = texture2D(texture1, textureOut1).r; \n"
"  mv = texture2D(texture1, textureOut1).b; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, 1.0); \n"
"} \n";


static char *frag_str_V308 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, my, mu, mv; \n"
"  vec3 rgb; \n"
"  mv = texture2D(texture, textureOut0).r; \n"
"  my = texture2D(texture, textureOut0).g; \n"
"  mu = texture2D(texture, textureOut0).b; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, 1.0); \n"
"} \n";

static char *frag_str_V408_Y410 =
"#version 110 \n"
"varying vec2 textureOut0; \n"
"varying vec2 textureOut1; \n"
"varying vec2 textureOut2; \n"
"uniform sampler2D texture; \n"
"uniform mat3 cspcoeff; \n"
"uniform vec3 cspconst; \n"
"void main(void) \n"
"{ \n"
"  float r, g, b, a, my, mu, mv; \n"
"  vec3 rgb; \n"
"  mv = texture2D(texture, textureOut0).b; \n"
"  my = texture2D(texture, textureOut0).g; \n"
"  mu = texture2D(texture, textureOut0).r; \n"
"  a = texture2D(texture, textureOut0).a; \n"
"  rgb = vec3(my, mu, mv); \n"
"  rgb = rgb * cspcoeff; \n"
"  rgb = rgb + cspconst; \n"
"  gl_FragColor=vec4(rgb, a); \n"
"} \n";


#ifndef NULL
#define NULL 0
#endif

static const GLfloat vertexVertices[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f,  1.0f,
	1.0f,  1.0f,
};

static const GLfloat textureVertices[] = {
	0.0f,  0.0f,
	1.0f,  0.0f,
	0.0f,  1.0f,
	1.0f,  1.0f,
};

static const GLfloat rev_textureVertices[] = {
	0.0f,  1.0f,
	1.0f,  1.0f,
	0.0f,  0.0f,
	1.0f,  0.0f,
};

static const GLfloat rev_textureVertices_rgb[] = {
	0.0f,  0.0f,
	1.0f,  0.0f,
	0.0f,  1.0f,
	1.0f,  1.0f,
};

static const GLfloat textureVertices2[] = {
	0.0f,  0.0f,
	1.0f,  0.0f,
	0.0f,  0.5f,
	1.0f,  0.5f,
};

static const GLfloat rev_textureVertices2[] = {
	0.0f,  0.5f,
	1.0f,  0.5f,
	0.0f,  0.0f,
	1.0f,  0.0f,
};

static const GLfloat textureVertices3[] = {
	0.0f,  0.5f,
	1.0f,  0.5f,
	0.0f,  0.75f,
	1.0f,  0.75f,
};

static const GLfloat rev_textureVertices3[] = {
	0.0f,  0.75f,
	1.0f,  0.75f,
	0.0f,  0.5f,
	1.0f,  0.5f,
};

static const GLfloat textureVertices4[] = {
	0.0f,  0.5f,
	1.0f,  0.5f,
	0.0f,  0.625f,
	1.0f,  0.625f,
};

static const GLfloat rev_textureVertices4[] = {
	0.0f,  0.625f,
	1.0f,  0.625f,
	0.0f,  0.5f,
	1.0f,  0.5f,
};

static const GLfloat textureVertices5[] = {
	0.0f,  0.625f,
	1.0f,  0.625f,
	0.0f,  0.75f,
	1.0f,  0.75f,
};

static const GLfloat rev_textureVertices5[] = {
	0.0f,  0.75f,
	1.0f,  0.75f,
	0.0f,  0.625f,
	1.0f,  0.625f,
};

static const GLfloat textureVertices6[] = {
	0.0f,  0.75f,
	1.0f,  0.75f,
	0.0f,  1.0f,
	1.0f,  1.0f,
};

static const GLfloat rev_textureVertices6[] = {
	0.0f,  1.0f,
	1.0f,  1.0f,
	0.0f,  0.75f,
	1.0f,  0.75f,
};

static const GLfloat textureVertices7[] = {
	0.0f,  0.5f,
	1.0f,  0.5f,
	0.0f,  1.0f,
	1.0f,  1.0f,
};

static const GLfloat rev_textureVertices7[] = {
	0.0f,  1.0f,
	1.0f,  1.0f,
	0.0f,  0.5f,
	1.0f,  0.5f,
};

#endif







