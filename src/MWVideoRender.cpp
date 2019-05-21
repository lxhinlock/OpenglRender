#if defined(_MSC_VER)
#include "stdafx.h"
#endif

#include "MWVideoRender.h"
#include <stdio.h>
#include <malloc.h>


#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
#pragma comment(lib,"opengl32.lib") 
#pragma comment(lib,"cudart_static.lib") 
#endif

void *mw_aligned_malloc(uint32_t i_size)
{
	uint8_t *p_align_buf = NULL;
	uint8_t *p_buf = (uint8_t*)malloc(i_size + (ALIGN_SIZE - 1) + sizeof(void **));
	if (p_buf) {
		p_align_buf = p_buf + (ALIGN_SIZE - 1) + sizeof(void **);
		p_align_buf -= (intptr_t)p_align_buf & (ALIGN_SIZE - 1);
		*((void **)(p_align_buf - sizeof(void **))) = p_buf;
	}
	return p_align_buf;
}

void mw_aligned_free(void *ptr)
{
	free(reinterpret_cast<void*>((static_cast<void**>(ptr))[-1]));
}

CGLRender::CGLRender() 
{
	glu32_tex = 0;  
	glu32_tex1 = 0;
	glu32_tex2 = 0;
	e_render_mode = MW_RENDER_AUTO;
	glu32_fbo = 0;
	glu32_rbo = 0;
	gli32_v = 0;
	gli32_f = 0;
	glu32_program = 0;
	glu32_verLocation = 0;
	glu32_texLocation0 = 0;
	glu32_texLocation1 = 0;
	glu32_texLocation2 = 0;
	u32_tex_width = 0;
	u32_tex_height = 0;
	u32_ini_width = 0;
	u32_ini_height = 0;
	for (int i = 0; i < 10; i++)
	{
		glu32_buf[i] = 0;
	}
	b_alloc_buf = false;
	b_need_rev = false; 
	u32_gl_version = 0;
	u32_glsl_version = 0;
	gl_map_mode = MW_MAP_MODE_AUTO;
	e_csp = MW_CSP_AUTO;
	e_csp_lvl_in = MW_CSP_LEVELS_AUTO;
	e_csp_lvl_out = MW_CSP_LEVELS_AUTO;
	f32_hue = 0.0f;
	f32_saturate = 1.0f;
	f32_value = 1.0f;
	b_hdr = false;
	b_opened = false;
	u8_val_ctrl = 0;
	u8_threshold = 0;
	u8_buf_num = 0;
	pc_framebuffer = NULL;
	gli32_max_texture_size = 0;
	b_resize = false;
}

CGLRender::~CGLRender() 
{
	CleanUp();
}

void CGLRender::CleanUp()
{
	if (glu32_fbo) {
		glDeleteFramebuffers(1, &glu32_fbo);
		glu32_fbo = 0;
	}

	if (glu32_rbo) {
		glDeleteRenderbuffers(1, &glu32_rbo);
		glu32_rbo = 0;
	}

	if (gli32_v) {
		glDeleteShader(gli32_v);
		gli32_v = 0;
	}

	if (gli32_f) {
		glDeleteShader(gli32_f);
		gli32_f = 0;
	}

	if (glu32_program) {
		glDeleteProgram(glu32_program);
		glu32_program = 0;
	}

	for (int i = 0; i < 10; i++) {
		if (glu32_buf[i]) {
			if (MW_INTEL_MAP_TEXT == gl_map_mode) {
				glUnmapTexture2DINTEL(glu32_buf[i], 0);
				glDeleteTextures(1, &glu32_buf[i]);
			}
			else {
				glDeleteBuffers(1, &glu32_buf[i]);
			}
			glu32_buf[i] = 0;
		}
	}

	if (glu32_tex) {
		glDeleteTextures(1,&glu32_tex);
		glu32_tex = 0;
	}

	if (glu32_tex1) {
		glDeleteTextures(1, &glu32_tex1);
		glu32_tex1 = 0;
	}

	if (glu32_tex2) {
		glDeleteTextures(1, &glu32_tex2);
		glu32_tex2 = 0;
	}

	if (pc_framebuffer) {
		pc_framebuffer->destory_frame_buffer(b_alloc_buf);
		delete pc_framebuffer;
		pc_framebuffer = NULL;
	}

	if (b_alloc_buf) {
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
		if (MW_NVIDIA_MAP_TEXT == gl_map_mode) {
			if (cuda_tex_result_resource) {
				cudaGraphicsUnmapResources(1, &cuda_tex_result_resource, 0);
				if (!b_resize)				
					cudaGraphicsUnregisterResource(cuda_tex_result_resource);				
			}
		}
#endif

		for (uint8_t i = 0; i < u8_buf_num; i++) {
			if (pptr[i]) {
				switch (gl_map_mode) {
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
				case MW_NVIDIA_MAP_TEXT:
					cudaFree(pptr[i]);
					break;
#endif
				case MW_INTEL_MAP_TEXT:
					break;
				case MW_AMD_MAP_BUF:
					mw_aligned_free(pptr[i]);
					break;
				case MW_GL_MAP_BUF:
					glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
					break;
				}
			}

		}
	}
} 	

void CGLRender::Close()
{
	b_opened = false;
	CleanUp();
}

mw_videorender_result_e CGLRender::GlVision(uint32_t *gl_version)
{
	int major = 0, minor = 0;
	const char *version_string = (const char *)glGetString(GL_VERSION);
	if (!version_string) 
		return MW_VIDEORENDER_GLVERSION_ERROR;

	if (sscanf(version_string, "%d.%d", &major, &minor) < 2)
		return MW_VIDEORENDER_GLVERSION_ERROR;

	*gl_version = ((major) * 100) + (minor) * 10;
	return MW_VIDEORENDER_NO_ERROR;
}


mw_videorender_result_e CGLRender::GlslVision(uint32_t *glsl_version)
{
	int major = 0, minor = 0;
	const char *version_string = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if (!version_string)
		return MW_VIDEORENDER_GLVERSION_ERROR;

	if (sscanf(version_string, "%d.%d", &major, &minor) < 2)
		return MW_VIDEORENDER_GLVERSION_ERROR;

	*glsl_version = ((major) * 100) + (minor);
	return MW_VIDEORENDER_NO_ERROR;
}

#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
mw_videorender_result_e CGLRender::nvGetMaxGflopsDeviceId(uint32_t *device)
{
	int current_device = 0, sm_per_multiproc = 0;
	int max_perf_device = 0;
	int device_count = 0, best_SM_arch = 0;
	int devices_prohibited = 0;

	uint64_t max_compute_perf = 0;
	cudaDeviceProp deviceProp;
	cudaError_t result;
	result = cudaGetDeviceCount(&device_count);

	if (result)
		return MW_VIDEORENDER_CUDA_ERROR;

	if (0 == device_count)
		return MW_VIDEORENDER_CUDA_NODEVICE;

	// Find the best major SM Architecture GPU device
	while (current_device < device_count) {
		cudaGetDeviceProperties(&deviceProp, current_device);

		// If this GPU is not running on Compute Mode prohibited,
		// then we can add it to the list
		if (deviceProp.computeMode != cudaComputeModeProhibited) {
			if (deviceProp.major > 0 && deviceProp.major < 9999) {
				best_SM_arch = MAX(best_SM_arch, deviceProp.major);
			}
		}
		else {
			devices_prohibited++;
		}

		current_device++;
	}

	if (devices_prohibited == device_count) {
		return MW_VIDEORENDER_CUDA_ERROR;
	}

	// Find the best CUDA capable GPU device
	current_device = 0;

	while (current_device < device_count) {
		cudaGetDeviceProperties(&deviceProp, current_device);

		// If this GPU is not running on Compute Mode prohibited,
		// then we can add it to the list
		if (deviceProp.computeMode != cudaComputeModeProhibited) {
			if (deviceProp.major == 9999 && deviceProp.minor == 9999) {
				sm_per_multiproc = 1;
			}
			else {
				sm_per_multiproc =
					_ConvertSMVer2Cores(deviceProp.major, deviceProp.minor);
			}

			uint64_t compute_perf = (uint64_t)deviceProp.multiProcessorCount *
				sm_per_multiproc * deviceProp.clockRate;

			if (compute_perf > max_compute_perf) {
				// If we find GPU with SM major > 2, search only these
				if (best_SM_arch > 2) {
					// If our device==dest_SM_arch, choose this, or else pass
					if (deviceProp.major == best_SM_arch) {
						max_compute_perf = compute_perf;
						max_perf_device = current_device;
					}
				}
				else {
					max_compute_perf = compute_perf;
					max_perf_device = current_device;
				}
			}
		}

		++current_device;
	}
	*device = max_perf_device;
	return MW_VIDEORENDER_NO_ERROR;
}
#endif

mw_videorender_result_e CGLRender::deviceinfo(mw_device_info_t *device_info)
{
	GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	GLenum ret;
	void *ptr;
	mw_videorender_result_e rslt;
	GLint stride;
	GLenum layout;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	uint32_t cuda_device;
	cudaError_t cuda_rslt;
	cudaArray *texture_ptr;
#endif
	rslt = GlVision(&u32_gl_version);
	if (MW_VIDEORENDER_NO_ERROR != rslt)
		return rslt;
	
	device_info->m_u32_glver = u32_gl_version;

	rslt = GlslVision(&u32_glsl_version);
	if (MW_VIDEORENDER_NO_ERROR != rslt)
		return rslt;
	
	device_info->m_u32_glslver = u32_glsl_version;
	gl_map_mode = 0;
	////////////////GL map
	ptr = 0;
	device_info->m_u32_gl_map = 0;
	if (GLEW_ARB_map_buffer_range) {		
		glGenBuffers(1, &glu32_buf[0]);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glu32_buf[0]);
		glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 4096*2160*4, 0, flags);
		ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 4096 * 2160 * 4, flags);
		if (ptr) {
			device_info->m_u32_gl_map = 0x7FFF;
			gl_map_mode |= MW_GL_MAP_BUF;
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		if (glu32_buf[0]) {
			glDeleteBuffers(1, &glu32_buf[0]);
			glu32_buf[0] = 0;
		}
	}
	////////////////amd map
	ptr = 0;
	device_info->m_u32_amd_map = 0;
	if (GLEW_AMD_pinned_memory) {
		glGenBuffers(1, &glu32_buf[0]);
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, glu32_buf[0]);
		ptr = malloc(4096*2160*4);
		glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 4096 * 2160 * 4, ptr, GL_STREAM_DRAW);
		ret = glGetError();
		if (GL_NO_ERROR == ret) {
			device_info->m_u32_amd_map = 0x7FFF;
			gl_map_mode |= MW_AMD_MAP_BUF;
		}
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0);
		if (glu32_buf[0]) {
			glDeleteBuffers(1, &glu32_buf[0]);
			glu32_buf[0] = 0;
		}
		if (ptr) {
			free(ptr);
			ptr = 0;
		}			
	}
	////////////////intel map
	ptr = 0;
	device_info->m_u32_intel_map = 0;
	if (GLEW_INTEL_map_texture) {
		glGenTextures(1, &glu32_buf[0]);
		glBindTexture(GL_TEXTURE_2D, glu32_buf[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MEMORY_LAYOUT_INTEL, GL_LAYOUT_LINEAR_CPU_CACHED_INTEL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4096, 2160, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		ptr = glMapTexture2DINTEL(glu32_buf[0], 0, GL_MAP_WRITE_BIT, &stride, &layout);
		if (ptr) {
			device_info->m_u32_intel_map = 0x7DFE;
			gl_map_mode |= MW_INTEL_MAP_TEXT;
			glUnmapTexture2DINTEL(glu32_buf[0], 0);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		if (glu32_buf[0]) {
			glDeleteTextures(1, &glu32_buf[0]);
			glu32_buf[0] = 0;
		}
	}
	////////////////nvidia map
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	ptr = 0;
	device_info->m_u32_nvidia_map = 0;

	rslt = nvGetMaxGflopsDeviceId(&cuda_device);
	if (MW_VIDEORENDER_NO_ERROR != rslt)	
		goto DONE;
	
	cuda_rslt = cudaSetDevice(cuda_device);
	if (cuda_rslt)	
		goto DONE;
		
	glGenTextures(1, &glu32_tex);
	glBindTexture(GL_TEXTURE_2D, glu32_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4096, 2160, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	cuda_rslt = cudaMallocHost(&ptr, 4 * 4096 * 2160);
	if (cuda_rslt) 
		goto DONE;
		
	cuda_rslt = 
		cudaGraphicsGLRegisterImage(
			&cuda_tex_result_resource,
			glu32_tex,
			GL_TEXTURE_2D,
			cudaGraphicsMapFlagsWriteDiscard
		);
	if (cuda_rslt)		
		goto DONE;
		
	cuda_rslt = 
		cudaGraphicsMapResources(
			1, 
			&cuda_tex_result_resource, 
			0
		);
	if (cuda_rslt)		
		goto DONE;
		

	cuda_rslt = 
		cudaGraphicsSubResourceGetMappedArray(
			&texture_ptr, 
			cuda_tex_result_resource, 
			0, 
			0
		);
	if (cuda_rslt)
		goto DONE;
		
	cuda_rslt = 
		cudaMemcpy2DToArray(
			texture_ptr, 
			0, 
			0, 
			ptr, 
			4096 * 4, 
			4096 * 4, 
			2160, 
			cudaMemcpyHostToDevice
		);
	if (cuda_rslt)		
		goto DONE;
		
	cuda_rslt = 
		cudaGraphicsUnmapResources(
			1, 
			&cuda_tex_result_resource, 
			0
		);
	if (cuda_rslt)
		goto DONE;
		
	device_info->m_u32_nvidia_map = 0xDDA;
	gl_map_mode |= MW_NVIDIA_MAP_TEXT;
	if (ptr) {
		cudaFreeHost(ptr);
		ptr = 0;
	}
	if (glu32_tex) {
		glDeleteTextures(1, &glu32_tex);
		glu32_tex = 0;
	}
	
#endif	
DONE:	
	return MW_VIDEORENDER_NO_ERROR;
}

mw_videorender_result_e CGLRender::open(mw_init_setting_t *rinit)
{	
	b_opened = false;
	u32_ini_width = rinit->m_u32_width;
	u32_ini_height = rinit->m_u32_height;
	b_need_rev = rinit->m_b_reverse;
	u8_buf_num = 0;
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);


#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	cudaError_t result;
#endif
	rinit->m_glptr = 0;
	rinit->m_pptr = 0;


	b_resize = rinit->m_b_resize;

	if (rinit->m_u32_buf_num) {
		if (rinit->m_u32_buf_num > 10)		
			return MW_VIDEORENDER_BUF_NUM_OUTOFRANGE;		
		else		
			u8_buf_num = rinit->m_u32_buf_num;
		
		pptr = new void *[u8_buf_num];
		if (NULL == pptr) {
			rinit->m_u32_buf_num = 0;
			u8_buf_num = 0;
			return MW_VIDEORENDER_BUF_CREATE_FAIL;
		}
	}

	if ((rinit->m_format > MW_RENDER_Y410) || 
		((rinit->m_format > MW_RENDER_BGRA) && 
		 (rinit->m_format < MW_RENDER_YUY2)))
		return MW_VIDEORENDER_COLORFORMAT_ERROR;	
	else	
		e_render_mode = rinit->m_format;
	

	if (0 == u32_gl_version) {
		mw_videorender_result_e rslt;
		rslt = GlVision(&u32_gl_version);
		if (MW_VIDEORENDER_NO_ERROR != rslt)		
			return rslt;		
	}

	if (u32_gl_version < 210)
		return MW_VIDEORENDER_GLVERSION_UNSUPPORT;
	

	if ((rinit->m_map_mode) && (u8_buf_num)) {
		gl_map_mode = rinit->m_map_mode & gl_map_mode;
		if (!gl_map_mode) {
			u8_buf_num = 0;
			rinit->m_u32_buf_num = 0;
			return MW_VIDEORENDER_MAP_MODE_ERROR;
		}
		else if ((MW_INTEL_MAP_TEXT != gl_map_mode) &&
			     (MW_GL_MAP_BUF != gl_map_mode) &&
			     (MW_AMD_MAP_BUF != gl_map_mode) &&
			     (MW_NVIDIA_MAP_TEXT != gl_map_mode)) {
			u8_buf_num = 0;
			rinit->m_u32_buf_num = 0;
			return MW_VIDEORENDER_MAP_MODE_ERROR;
		}
	}
	else {
		u8_buf_num = 0;
		rinit->m_u32_buf_num = 0;
	}


	if ((MW_INTEL_MAP_TEXT == gl_map_mode) && (u8_buf_num)) {
		if ((MW_RENDER_BGR24 == e_render_mode) || 
			(MW_RENDER_V308 == e_render_mode) ||
			b_resize) {
			u8_buf_num = 0;
			rinit->m_u32_buf_num = 0;
			return MW_VIDEORENDER_COLORFORMAT_ERROR;
		}
	}


	if ((MW_NVIDIA_MAP_TEXT == gl_map_mode) && (u8_buf_num)) {
		if ((MW_RENDER_P010 == e_render_mode) ||
			(MW_RENDER_P210 == e_render_mode) ||
			(MW_RENDER_V308 == e_render_mode) ||
			(MW_RENDER_BGR24 == e_render_mode) ||
			(MW_RENDER_RGB10 == e_render_mode) ||
			(MW_RENDER_Y410 == e_render_mode)) {
			u8_buf_num = 0;
			rinit->m_u32_buf_num = 0;
			return MW_VIDEORENDER_COLORFORMAT_ERROR;
		}
	}
	
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gli32_max_texture_size);

	if ((u32_ini_width > gli32_max_texture_size) || (u32_ini_height > gli32_max_texture_size))	
		return MW_VIDEORENDER_SIZE_ERROR;
	
	if ((u32_ini_width % 2) || (u32_ini_height % 2))	
		return MW_VIDEORENDER_SIZE_ERROR;
	
	u32_tex_width = u32_ini_width;
	u32_tex_height = u32_ini_height;

	GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	GLint vertCompiled = 0, fragCompiled = 0, linked = 0;
	void *ptr;
	uint32_t frame_size = 0;


	gli32_v = glCreateShader(GL_VERTEX_SHADER);
	if (!gli32_v) {
		CleanUp();
		return MW_VIDEORENDER_CREATE_SHADER_ERROR;
	}
	gli32_f = glCreateShader(GL_FRAGMENT_SHADER);
	if (!gli32_f) {
		CleanUp();
		return MW_VIDEORENDER_CREATE_SHADER_ERROR;
	}


	if (u8_buf_num) {
		if (MW_INTEL_MAP_TEXT == gl_map_mode) {
			GLint stride;
			GLenum layout;
			for (uint8_t i = 0; i < u8_buf_num; i++) {
				glGenTextures(1, &glu32_buf[i]);
				if (!glu32_buf[i]) {
					CleanUp();
					return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
				}
				glBindTexture(GL_TEXTURE_2D, glu32_buf[i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MEMORY_LAYOUT_INTEL, GL_LAYOUT_LINEAR_CPU_CACHED_INTEL);
				

				if (u32_gl_version >= 210) {
					switch (e_render_mode) {
					case MW_RENDER_RGBA:
					case MW_RENDER_V408:
					case MW_RENDER_BGRA:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGBA8, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 4;
						break;
					case MW_RENDER_YUY2:
					case MW_RENDER_UYVY:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8_ALPHA8,
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_LUMINANCE_ALPHA,
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 2;
						break;
					case MW_RENDER_NV12:
					case MW_RENDER_NV16:
					case MW_RENDER_I420:
					case MW_RENDER_I422:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 2;
						break;
					case MW_RENDER_RGB10:
					case MW_RENDER_Y410:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGB10_A2, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_INT_2_10_10_10_REV, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 4;
						break;
					case MW_RENDER_P010:
					case MW_RENDER_P210:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE16, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_SHORT, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 4;
						break;
					}
				}			  
				ptr = glMapTexture2DINTEL(glu32_buf[i], 0, GL_MAP_WRITE_BIT, &stride, &layout);
				if (!ptr) {
					CleanUp();
					rinit->m_u32_buf_num = i;
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				}
				pptr[i] = ptr;
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}	
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else	    
		else if (MW_NVIDIA_MAP_TEXT == gl_map_mode) {
			glGenTextures(1, &glu32_tex);
			if (!glu32_tex) {
				CleanUp();
				return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
			}
/////////////////////////////////////////////////////////
			if (b_resize) {
				glBindTexture(GL_TEXTURE_2D, glu32_tex);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glBindTexture(GL_TEXTURE_2D, 0);
				if (u32_gl_version >= 210) {
					switch (e_render_mode) {
					case MW_RENDER_RGBA:
					case MW_RENDER_V408:
					case MW_RENDER_BGRA:
						frame_size = u32_tex_width * u32_tex_height * 4;
						break;
					case MW_RENDER_YUY2:
					case MW_RENDER_UYVY:
					case MW_RENDER_NV12:
					case MW_RENDER_NV16:
					case MW_RENDER_I420:
					case MW_RENDER_I422:
						frame_size = u32_tex_width * u32_tex_height * 2;
						break;
					}
				}
			}
			else {
				glBindTexture(GL_TEXTURE_2D, glu32_tex);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (u32_gl_version >= 210) {
					switch (e_render_mode) {
					case MW_RENDER_RGBA:
					case MW_RENDER_V408:
					case MW_RENDER_BGRA:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGBA8, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 4;
						break;
					case MW_RENDER_YUY2:
					case MW_RENDER_UYVY:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8_ALPHA8,
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_LUMINANCE_ALPHA,
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 2;
						break;
					case MW_RENDER_NV12:
					case MW_RENDER_NV16:
					case MW_RENDER_I420:
					case MW_RENDER_I422:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						frame_size = u32_tex_width * u32_tex_height * 2;
						break;
					}
				}

				glBindTexture(GL_TEXTURE_2D, 0);

				result = 
					cudaGraphicsGLRegisterImage(
						&cuda_tex_result_resource,
						glu32_tex,
						GL_TEXTURE_2D,
						cudaGraphicsMapFlagsWriteDiscard
					);
				if (result)				
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				
			}
/////////////////////////////////////////////
			for (uint8_t i = 0; i < u8_buf_num; i++) {
				result = cudaMallocHost(&ptr, frame_size);
				if ((result) || (!ptr)) {
					CleanUp();
					rinit->m_u32_buf_num = i;
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				}
				pptr[i] = ptr;
			}

		}
#endif	  
		else if((MW_GL_MAP_BUF == gl_map_mode) || (MW_AMD_MAP_BUF == gl_map_mode)) {
			glGenTextures(1, &glu32_tex);
			if (!glu32_tex) {
				CleanUp();
				return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
			}
			glBindTexture(GL_TEXTURE_2D, glu32_tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
			if (u32_gl_version >= 210) {
				switch (e_render_mode) {
				case MW_RENDER_BGR24:
				case MW_RENDER_V308:
					frame_size = u32_tex_width * u32_tex_height * 3;
					break;
				case MW_RENDER_RGBA:
				case MW_RENDER_V408:
				case MW_RENDER_BGRA:
				case MW_RENDER_RGB10:
				case MW_RENDER_Y410:
				case MW_RENDER_P010:
				case MW_RENDER_P210:
					frame_size = u32_tex_width * u32_tex_height * 4;
					break;
				case MW_RENDER_YUY2:
				case MW_RENDER_UYVY:
				case MW_RENDER_NV12:
				case MW_RENDER_NV16:
				case MW_RENDER_I420:
				case MW_RENDER_I422:
					frame_size = u32_tex_width * u32_tex_height * 2;
					break;
				}
			}
			for (uint8_t i = 0; i < u8_buf_num; i++) {
				glGenBuffers(1, &glu32_buf[i]);
				if (MW_GL_MAP_BUF == gl_map_mode) {
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glu32_buf[i]);
					glBufferStorage(GL_PIXEL_UNPACK_BUFFER, frame_size, 0, flags);
					ptr = (uint8_t *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, frame_size, flags);
					if (!ptr) {
						CleanUp();
						rinit->m_u32_buf_num = i;
						return MW_VIDEORENDER_MAPBUFFER_ERROR;
					}
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				}
				else if (MW_AMD_MAP_BUF == gl_map_mode) {
					glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, glu32_buf[i]);
					ptr = mw_aligned_malloc(frame_size);
					glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, frame_size, ptr, GL_STREAM_DRAW);
					GLenum ret = glGetError();
					if (GL_NO_ERROR != ret) {
						CleanUp();
						rinit->m_u32_buf_num = i;
						return MW_VIDEORENDER_MAPBUFFER_ERROR;
					}
					glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0);

				}
				pptr[i] = ptr;
			}
		}
		rinit->m_glptr = glu32_buf;
		rinit->m_pptr = pptr;
		b_alloc_buf = true;
	}
	else {
		glGenTextures(1, &glu32_tex);
		if (!glu32_tex) {
			CleanUp();
			return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
		}
		glBindTexture(GL_TEXTURE_2D, glu32_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenTextures(1, &glu32_tex1);
	if (!glu32_tex1) {
		CleanUp();
		return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
	}
	glBindTexture(GL_TEXTURE_2D, glu32_tex1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &glu32_tex2);
	if (!glu32_tex2) {
		CleanUp();
		return MW_VIDEORENDER_GEN_TEXTURE_ERROR;
	}
	glBindTexture(GL_TEXTURE_2D, glu32_tex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	if (u32_gl_version >= 210) {
		glShaderSource(gli32_v, 1, (const GLchar **)&vert_str, NULL);
		switch (e_render_mode) {
		case MW_RENDER_BGR24:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_BGR24, NULL);
			break;
		case MW_RENDER_RGBA:
		case MW_RENDER_RGB10:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_RGBA_RGB10, NULL);
			break;
		case MW_RENDER_BGRA:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_BGRA, NULL);
			break;
		case MW_RENDER_YUY2:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_YUY2, NULL);
			break;
		case MW_RENDER_NV12:
		case MW_RENDER_P010:
		case MW_RENDER_NV16:
		case MW_RENDER_P210:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_NV12_P010_NV16_P210, NULL);
			break;
		case MW_RENDER_I420:
		case MW_RENDER_I422:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_I420_I422, NULL);
			break;
		case MW_RENDER_UYVY:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_UYVY, NULL);
			break;
		case MW_RENDER_V308:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_V308, NULL);
			break;
		case MW_RENDER_V408:
		case MW_RENDER_Y410:
			glShaderSource(gli32_f, 1, (const GLchar **)&frag_str_V408_Y410, NULL);
			break;
		}
	}

	glCompileShader(gli32_v);
	glGetShaderiv(gli32_v, GL_COMPILE_STATUS, &vertCompiled);
	if (!vertCompiled) {
		CleanUp();
		return MW_VIDEORENDER_COMPILE_SHADER_ERROR;
	}
	glCompileShader(gli32_f);
	glGetShaderiv(gli32_f, GL_COMPILE_STATUS, &fragCompiled);
	if (!fragCompiled) {
		CleanUp();
		return MW_VIDEORENDER_COMPILE_SHADER_ERROR;
	}

	glu32_program = glCreateProgram();
	glAttachShader(glu32_program, gli32_v);
	glAttachShader(glu32_program, gli32_f);
	glLinkProgram(glu32_program);
	glGetProgramiv(glu32_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		CleanUp();
		return MW_VIDEORENDER_LINK_ERROR;
	}

	glUseProgram(glu32_program);

	glu32_verLocation = glGetAttribLocation(glu32_program, "vertexIn");
	glu32_texLocation0 = glGetAttribLocation(glu32_program, "textureIn0");
	glu32_texLocation1 = glGetAttribLocation(glu32_program, "textureIn1");
	glu32_texLocation2 = glGetAttribLocation(glu32_program, "textureIn2");

	glVertexAttribPointer(glu32_verLocation, 2, GL_FLOAT, 0, 0, vertexVertices);
	glEnableVertexAttribArray(glu32_verLocation);

	if (u32_gl_version >= 210) {
		if (b_need_rev) {
			switch (e_render_mode) {
			case MW_RENDER_BGR24:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices_rgb);
				glEnableVertexAttribArray(glu32_texLocation0);
				break;
			case MW_RENDER_YUY2:
			case MW_RENDER_UYVY:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
				glEnableVertexAttribArray(glu32_texLocation0);
				glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
				glEnableVertexAttribArray(glu32_texLocation1);
				break;
			case MW_RENDER_V308:
			case MW_RENDER_V408:
			case MW_RENDER_RGBA:
			case MW_RENDER_RGB10:
			case MW_RENDER_Y410:
			case MW_RENDER_BGRA:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
				glEnableVertexAttribArray(glu32_texLocation0);
				break;
			case MW_RENDER_P010:
			case MW_RENDER_NV12:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				break;
			case MW_RENDER_NV16:
			case MW_RENDER_P210:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				break;
			case MW_RENDER_I420:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				break;
			case MW_RENDER_I422:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, rev_textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				break;
			}
		}
		else {
			switch (e_render_mode) {
			case MW_RENDER_BGR24:
			case MW_RENDER_RGBA:
			case MW_RENDER_RGB10:
			case MW_RENDER_Y410:
			case MW_RENDER_BGRA:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
				glEnableVertexAttribArray(glu32_texLocation0);
				break;
			case MW_RENDER_YUY2:
			case MW_RENDER_UYVY:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
				glEnableVertexAttribArray(glu32_texLocation0);
				glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
				glEnableVertexAttribArray(glu32_texLocation1);
				break;
			case MW_RENDER_V308:
			case MW_RENDER_V408:
				glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
				glEnableVertexAttribArray(glu32_texLocation0);
				break;
			case MW_RENDER_P010:
			case MW_RENDER_NV12:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				break;
			case MW_RENDER_NV16:
			case MW_RENDER_P210:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
				}
				break;
			case MW_RENDER_I420:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				break;
			case MW_RENDER_I422:
				if (u8_buf_num) {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices2);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				else {
					glVertexAttribPointer(glu32_texLocation0, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation0);
					glVertexAttribPointer(glu32_texLocation1, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation1);
					glVertexAttribPointer(glu32_texLocation2, 2, GL_FLOAT, 0, 0, textureVertices);
					glEnableVertexAttribArray(glu32_texLocation2);
				}
				break;
			}
		}
	}

	///////////////////yuv->rgb 
	float f32_csp_coeff[3][3] = {0.0};
	float f32_csp_const[3] = {0.0};
	float f32_lr = 0.0;
	float f32_lg = 0.0;
	float f32_lb = 0.0;

	if (rinit->m_csp >= MW_CSP_COUNT) {
		return MW_VIDEORENDER_COLOR_SPACE_MODE_ERROR;
	}
	else if (rinit->m_csp == MW_CSP_AUTO) {
		e_csp = MW_CSP_BT_709;
	}
	else {
		e_csp = rinit->m_csp;
	}


	if (e_render_mode >= MW_RENDER_YUY2) {
		switch (e_csp) {
		case MW_CSP_BT_601:
			f32_lr = 0.299, f32_lg = 0.587, f32_lb = 0.114;
			break;
		case MW_CSP_BT_709:
			f32_lr = 0.2126, f32_lg = 0.7152, f32_lb = 0.0722;
			break;
		case MW_CSP_BT_2020:
			f32_lr = 0.2627, f32_lg = 0.6780, f32_lb = 0.0593;
			break;
		default: return MW_VIDEORENDER_COLOR_SPACE_MODE_ERROR;
		}
	}

	f32_csp_coeff[0][0] = 1;
	f32_csp_coeff[0][1] = 0;
	f32_csp_coeff[0][2] = 2 * (1 - f32_lr);
	f32_csp_coeff[1][0] = 1;
	f32_csp_coeff[1][1] = -2 * (1 - f32_lb) * f32_lb / f32_lg;
	f32_csp_coeff[1][2] = -2 * (1 - f32_lr) * f32_lr / f32_lg;
	f32_csp_coeff[2][0] = 1;
	f32_csp_coeff[2][1] = 2 * (1 - f32_lb);
	f32_csp_coeff[2][2] = 0;


	yuvlevels_t t_yuvlev = {0.0};
	rgblevels_t t_rgblev = {0.0};
	rgblevels_t t_rgblev_tv={ 16.0 / 255.0, 235.0 / 255.0 };
	rgblevels_t t_rgblev_pc={ 0.0, 255.0 / 255.0 }; 
	yuvlevels_t t_lev_tv={ 16.0 / 255.0, 235.0 / 255.0, 240.0 / 255.0, 128.0 / 255.0 };
	yuvlevels_t t_lev_pc={ 0.0 , 255.0 / 255.0, 255.0 / 255.0, 128.0 / 255.0 };

	if (rinit->m_csp_lvl_in >= MW_CSP_LEVELS_COUNT) {
		return MW_VIDEORENDER_COLOR_SPACE_LEVEL_ERROR;
	}
	else if (rinit->m_csp_lvl_in == MW_CSP_LEVELS_AUTO) {
		e_csp_lvl_in = MW_CSP_LEVELS_TV;
	}
	else {
		e_csp_lvl_in = rinit->m_csp_lvl_in;
	}

	if (rinit->m_csp_lvl_out >= MW_CSP_LEVELS_COUNT) {
		return MW_VIDEORENDER_COLOR_SPACE_LEVEL_ERROR;
	}
	else if (rinit->m_csp_lvl_out == MW_CSP_LEVELS_AUTO) {
		e_csp_lvl_out = MW_CSP_LEVELS_PC;
	}
	else {
		e_csp_lvl_out = rinit->m_csp_lvl_out;
	}

	if (e_render_mode >= MW_RENDER_YUY2) {
		switch (e_csp_lvl_in) {
		case MW_CSP_LEVELS_TV:
			t_yuvlev = t_lev_tv;
			break;
		case MW_CSP_LEVELS_PC:
			t_yuvlev = t_lev_pc;
			break;
		default: return MW_VIDEORENDER_COLOR_SPACE_LEVEL_ERROR;
		}

		switch (e_csp_lvl_out) {
		case MW_CSP_LEVELS_TV:
			t_rgblev = t_rgblev_tv;
			break;
		case MW_CSP_LEVELS_PC: t_rgblev = t_rgblev_pc;
			break;
		default:
			return MW_VIDEORENDER_COLOR_SPACE_LEVEL_ERROR;
		}
	}


	double ymul = (t_rgblev.max - t_rgblev.min) / (t_yuvlev.ymax - t_yuvlev.ymin);
	double cmul = (t_rgblev.max - t_rgblev.min) / (t_yuvlev.cmax - t_yuvlev.cmid) / 2.0;

	for (int i = 0; i < 3; i++) {
		f32_csp_coeff[i][0] *= ymul;
		f32_csp_coeff[i][1] *= cmul;
		f32_csp_coeff[i][2] *= cmul;

		f32_csp_const[i] = t_rgblev.min - f32_csp_coeff[i][0] * t_yuvlev.ymin
			- (f32_csp_coeff[i][1] + f32_csp_coeff[i][2]) * t_yuvlev.cmid;

	}

	glUniformMatrix3fv(glGetUniformLocation(glu32_program, "cspcoeff"), 1, GL_FALSE, &f32_csp_coeff[0][0]);
	glUniform3f(glGetUniformLocation(glu32_program, "cspconst"), f32_csp_const[0], f32_csp_const[1], f32_csp_const[2]);

	glClearColor(0.0, 0.0, 0.0, 1.0);

	b_opened = true;
	return MW_VIDEORENDER_NO_ERROR;		 	    
}



mw_videorender_result_e CGLRender::open_with_glew(mw_init_setting_t *rinit)
{
	b_opened = false;
	GLenum t_ret=glewInit();
	if(!t_ret)
		return open(rinit);

	return MW_VIDEORENDER_CREATE_SHADER_ERROR;
}

mw_videorender_result_e CGLRender::render(void *data,
	mw_render_ctrl_setting_t *rctrl,
	GLuint glvar,
	GLuint gldef)
{
	uint32_t display_w = rctrl->m_u32_display_w;
	uint32_t display_h = rctrl->m_u32_display_h;


	if (b_resize) {
		if (((rctrl->m_u32_width > u32_ini_width) || (rctrl->m_u32_height > u32_ini_height)) && 
			(b_alloc_buf || pc_framebuffer)) {
			return MW_VIDEORENDER_SIZE_ERROR;
		}

		if ((rctrl->m_u32_width > gli32_max_texture_size) || 
			(rctrl->m_u32_height > gli32_max_texture_size)) {
			return MW_VIDEORENDER_SIZE_ERROR;
		}
		if ((rctrl->m_u32_width % 2) || (rctrl->m_u32_height % 2)) {
			return MW_VIDEORENDER_SIZE_ERROR;
		}
		u32_tex_width = rctrl->m_u32_width;
		u32_tex_height = rctrl->m_u32_height;
    }



#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else
	cudaError_t result;
#endif
	if (b_alloc_buf && glvar && (MW_INTEL_MAP_TEXT == gl_map_mode))	
		glSyncTextureINTEL(glvar);
	

	if (b_alloc_buf && glvar && (MW_GL_MAP_BUF == gl_map_mode))	
		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT); 
	

	if (u32_gl_version >= 210) {
		glViewport(0, 0, display_w, display_h);	
	}

	if (u32_gl_version >= 210) {
		if (b_alloc_buf) {
			if ((glvar) && (MW_NVIDIA_MAP_TEXT != gl_map_mode)) {
				glActiveTexture(GL_TEXTURE0);
				if (MW_INTEL_MAP_TEXT == gl_map_mode) {
					glBindTexture(GL_TEXTURE_2D, glvar);
				}
				else {
					glBindTexture(GL_TEXTURE_2D, glu32_tex);
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glvar);
					switch (e_render_mode) {
					case MW_RENDER_BGR24:
					case MW_RENDER_V308:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGB8, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGB, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_RGBA:
					case MW_RENDER_V408:
					case MW_RENDER_BGRA:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGBA8, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_RGB10:
					case MW_RENDER_Y410:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGB10_A2, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_INT_2_10_10_10_REV, 
							0
						);
						break;
					case MW_RENDER_YUY2:
					case MW_RENDER_UYVY:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8_ALPHA8,
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_LUMINANCE_ALPHA,
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_NV12:
					case MW_RENDER_NV16:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_P010:
					case MW_RENDER_P210:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE16, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_SHORT, 
							0
						);
						break;
					case MW_RENDER_I420:
					case MW_RENDER_I422:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					}
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				}
				
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
#else		  
			else if ((data) && (MW_NVIDIA_MAP_TEXT == gl_map_mode)) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, glu32_tex);
/////////////////////////////////////////////////////////////////////////////
				if (b_resize) {
					switch (e_render_mode) {
					case MW_RENDER_RGBA:
					case MW_RENDER_V408:
					case MW_RENDER_BGRA:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_RGBA8, 
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_RGBA, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_YUY2:
					case MW_RENDER_UYVY:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8_ALPHA8,
							u32_tex_width, 
							u32_tex_height, 
							0, 
							GL_LUMINANCE_ALPHA,
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_NV12:
					case MW_RENDER_NV16:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					case MW_RENDER_I420:
					case MW_RENDER_I422:
						glTexImage2D(
							GL_TEXTURE_2D, 
							0, 
							GL_LUMINANCE8, 
							u32_tex_width, 
							(u32_tex_height * 2), 
							0, 
							GL_LUMINANCE, 
							GL_UNSIGNED_BYTE, 
							0
						);
						break;
					}
					result = 
						cudaGraphicsGLRegisterImage(
							&cuda_tex_result_resource,
							glu32_tex,
							GL_TEXTURE_2D,
							cudaGraphicsMapFlagsWriteDiscard
						);
					if (result)					
						return MW_VIDEORENDER_MAPBUFFER_ERROR;
					
				}
				
///////////////////////////////////////////////////////////////////////////////
				result = cudaGraphicsMapResources(1, &cuda_tex_result_resource, 0);
				if (result)				
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				
				cudaArray *texture_ptr;
				result = cudaGraphicsSubResourceGetMappedArray(&texture_ptr, cuda_tex_result_resource, 0, 0);
				if (result)				
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				
				switch (e_render_mode) {
				case MW_RENDER_RGBA:
				case MW_RENDER_V408:
				case MW_RENDER_BGRA:
					result = 
						cudaMemcpy2DToArray(
							texture_ptr, 
							0, 
							0, 
							data, 
							u32_tex_width * 4, 
							u32_tex_width * 4, 
							u32_tex_height, 
							cudaMemcpyHostToDevice
						);
					break;
				case MW_RENDER_YUY2:
				case MW_RENDER_UYVY:
					result = 
						cudaMemcpy2DToArray(
							texture_ptr, 
							0, 
							0, 
							data, 
							u32_tex_width * 2, 
							u32_tex_width * 2, 
							u32_tex_height, 
							cudaMemcpyHostToDevice
						);
					break;
				case MW_RENDER_NV12:
				case MW_RENDER_NV16:
				case MW_RENDER_I420:
				case MW_RENDER_I422:
					result = 
						cudaMemcpy2DToArray(
							texture_ptr, 
							0, 
							0, 
							data, 
							u32_tex_width, 
							u32_tex_width, 
							u32_tex_height * 2, 
							cudaMemcpyHostToDevice
						);
					break;
				}
				if (result)				
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				
				result = cudaGraphicsUnmapResources(1, &cuda_tex_result_resource, 0);
				if (result)				
					return MW_VIDEORENDER_MAPBUFFER_ERROR;
				
				//////////////////////////////////////////////////////////////
				if (b_resize)
				{
					cudaGraphicsUnregisterResource(cuda_tex_result_resource);
				}
				///////////////////////////////////////////////////////////////
			}
#endif		  
		}
		else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, glu32_tex);
			if (data) {
				switch (e_render_mode) {
				case MW_RENDER_BGR24:
				case MW_RENDER_V308:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_RGB8, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_RGB, 
						GL_UNSIGNED_BYTE, 
						data
					);
					break;
				case MW_RENDER_RGBA:
				case MW_RENDER_V408:
				case MW_RENDER_BGRA:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_RGBA8, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_RGBA, 
						GL_UNSIGNED_BYTE, 
						data
					);
					break;
				case MW_RENDER_RGB10:
				case MW_RENDER_Y410:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_RGB10_A2, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_RGBA, 
						GL_UNSIGNED_INT_2_10_10_10_REV, 
						data
					);
					break;
				case MW_RENDER_YUY2:
				case MW_RENDER_UYVY:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_LUMINANCE8_ALPHA8,
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_LUMINANCE_ALPHA,
						GL_UNSIGNED_BYTE, 
						data
					);
					break;
				case MW_RENDER_NV12:
				case MW_RENDER_NV16:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_LUMINANCE8, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_LUMINANCE, 
						GL_UNSIGNED_BYTE, 
						data
					);
					break;
				case MW_RENDER_P010:
				case MW_RENDER_P210:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_LUMINANCE16, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_LUMINANCE, 
						GL_UNSIGNED_SHORT, 
						data
					);
					break;
				case MW_RENDER_I420:
				case MW_RENDER_I422:
					glTexImage2D(
						GL_TEXTURE_2D, 
						0, 
						GL_LUMINANCE8, 
						u32_tex_width, 
						u32_tex_height, 
						0, 
						GL_LUMINANCE, 
						GL_UNSIGNED_BYTE, 
						data
					);
					break;
				}
			}
			
		}
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, glu32_tex1);
		if (data) {
			switch (e_render_mode) {
			case MW_RENDER_YUY2:
			case MW_RENDER_UYVY:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RGBA8,
					(u32_tex_width / 2),
					u32_tex_height,
					0,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					data
				);
				break;
			case MW_RENDER_NV12:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8_ALPHA8,
					u32_tex_width / 2,
					u32_tex_height / 2,
					0,
					GL_LUMINANCE_ALPHA,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width*u32_tex_height)
				);
				break;
			case MW_RENDER_NV16:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8_ALPHA8,
					(u32_tex_width / 2),
					u32_tex_height,
					0,
					GL_LUMINANCE_ALPHA,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width*u32_tex_height)
				);
				break;
			case MW_RENDER_P010:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8_ALPHA8,
					(u32_tex_width / 2),
					(u32_tex_height / 2),
					0,
					GL_LUMINANCE_ALPHA,
					GL_UNSIGNED_SHORT,
					(uint8_t*)data + (u32_tex_width*u32_tex_height * 2)
				);
				break;
			case MW_RENDER_P210:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8_ALPHA8,
					(u32_tex_width / 2),
					(u32_tex_height),
					0,
					GL_LUMINANCE_ALPHA,
					GL_UNSIGNED_SHORT,
					(uint8_t*)data + (u32_tex_width*u32_tex_height * 2)
				);
				break;
			case MW_RENDER_I420:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8,
					(u32_tex_width / 2),
					(u32_tex_height / 2),
					0,
					GL_LUMINANCE,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width*u32_tex_height)
				);
				break;
			case MW_RENDER_I422:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8,
					(u32_tex_width / 2),
					u32_tex_height,
					0,
					GL_LUMINANCE,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width*u32_tex_height)
				);
				break;
			}
		}
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, glu32_tex2);
		if (data) {
			switch (e_render_mode) {
			case MW_RENDER_I420:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8,
					(u32_tex_width / 2),
					(u32_tex_height / 2),
					0,
					GL_LUMINANCE,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width * u32_tex_height * 5 / 4)
				);
				break;
			case MW_RENDER_I422:
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_LUMINANCE8,
					(u32_tex_width / 2),
					u32_tex_height,
					0,
					GL_LUMINANCE,
					GL_UNSIGNED_BYTE,
					(uint8_t*)data + (u32_tex_width * u32_tex_height * 3 / 2)
				);
				break;
			}
		}
		
	}

	glUniform1i(glGetUniformLocation(glu32_program, "texture"), 0);
	glUniform1i(glGetUniformLocation(glu32_program, "texture1"), 1);
	glUniform1i(glGetUniformLocation(glu32_program, "texture2"), 2);

	if (b_alloc_buf && !glvar && (MW_INTEL_MAP_TEXT == gl_map_mode)) {

	}
	else {
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	return MW_VIDEORENDER_NO_ERROR;	
}


mw_videorender_result_e CGLRender::screenshot(mw_screen_shot_t *t_screenshot)
{
	if ((0 == t_screenshot->m_u32_width) || (0 == t_screenshot->m_u32_height))	
		return MW_VIDEORENDER_SIZE_ERROR;
	

	if (NULL == t_screenshot->m_ptr)	
		return MW_VIDEORENDER_DATA_NULL;
	
	glReadPixels(
		t_screenshot->m_u32_x, 
		t_screenshot->m_u32_y, 
		t_screenshot->m_u32_width, 
		t_screenshot->m_u32_height,
		GL_RGBA,
		GL_UNSIGNED_BYTE, 
		t_screenshot->m_ptr
	);
	GLenum err = glGetError();
	if (GL_NO_ERROR != err)	
		return MW_VIDEORENDER_GLVAR_ERROR;
	
	return MW_VIDEORENDER_NO_ERROR;
}

mw_videorender_result_e CGLRender::create_framebuffer(uint32_t t_u32_count, mw_queue_mode_e t_map_mode){
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	if (!b_opened)
		return MW_VIDEORENDER_BUF_RENDER_NOT_OPENED;
	

	if (pc_framebuffer != NULL){
		pc_framebuffer->destory_frame_buffer(b_alloc_buf);
		pc_framebuffer = NULL;
	}

	pc_framebuffer = new CMWVideoFrameBuffer();
	if (pc_framebuffer == NULL)
		return MW_VIDEORENDER_BUF_CREATE_FAIL;
	
	uint32_t t_u32_stride = fmt_calcminstride(e_render_mode, u32_tex_width, 4);
	uint32_t t_u32_size = fmt_calcimagesize(e_render_mode, u32_tex_width, u32_tex_height, t_u32_stride);
	if (b_alloc_buf)
	{
		t_rslt_e = 
			pc_framebuffer->create_frame_buffer(
				u8_buf_num, 
				t_u32_size, 
				pptr, 
				glu32_buf,
				t_map_mode, 
				b_alloc_buf
			);
	}
	else
	{
		t_rslt_e = 
			pc_framebuffer->create_frame_buffer(
				t_u32_count, 
				t_u32_size, 
				pptr, 
				glu32_buf,
				t_map_mode, 
				b_alloc_buf
			);
	}

	if (t_rslt_e != MW_VIDEORENDER_NO_ERROR){
		delete pc_framebuffer;
		pc_framebuffer = NULL;
	}
	return t_rslt_e;
}

mw_videorender_result_e CGLRender::putframedata(void *p_data, uint32_t t_u32_size, uint32_t t_u32_width, uint32_t t_u32_height){
	if (pc_framebuffer == NULL)
		return MW_VIDEORENDER_BUF_NONE;
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	t_rslt_e = pc_framebuffer->put_frame_data(p_data, t_u32_size, t_u32_width, t_u32_height);
	return t_rslt_e;
}

mw_videorender_result_e CGLRender::render_frame(mw_render_ctrl_setting_t *rctrl, GLuint glvar, GLuint gldef)
{
	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	mw_render_frame_t *t_p_frame = NULL;
	t_rslt_e = pc_framebuffer->get_frame_buffer(&t_p_frame);
	if (t_rslt_e != MW_VIDEORENDER_NO_ERROR)
		return t_rslt_e;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_lock(&t_p_frame->m_lock);
#else
	EnterCriticalSection(&t_p_frame->m_lock);
#endif
	rctrl->m_u32_width = t_p_frame->m_width;
	rctrl->m_u32_height = t_p_frame->m_height;
	t_rslt_e = render(t_p_frame->m_p_data, rctrl, t_p_frame->t_glvar, gldef);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_unlock(&t_p_frame->m_lock);
#else
	LeaveCriticalSection(&t_p_frame->m_lock);
#endif
	pc_framebuffer->put_frame_buffer(&t_p_frame);

	return t_rslt_e;
}

void CGLRender::destory_framebuffer(){
	if (pc_framebuffer != NULL){
		pc_framebuffer->destory_frame_buffer(b_alloc_buf);
		delete pc_framebuffer;
		pc_framebuffer = NULL;
	}
}

mw_videorender_result_e CGLRender::display(mw_display_mode_e t_mode)
{
	switch (t_mode) {
	case MW_DSP_VSYNC_ON:
	case MW_DSP_VSYNC_OFF:
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		glXSwapBuffers(m_dpy, m_win);
#else
		SwapBuffers(m_hdc);
#endif
		break;
	case MW_DSP_LOW_LATENCY:
		glFinish();
		break;
	default:
		break;
	}
	return MW_VIDEORENDER_NO_ERROR;

}

mw_videorender_result_e CGLRender::setinterval(mw_display_mode_e t_mode)
{
#ifdef LIBMWVIDEORENDER_LINUX_DEF

#else
	typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
#endif

	switch (t_mode) {
	case MW_DSP_VSYNC_ON:
#ifdef LIBMWVIDEORENDER_LINUX_DEF

#else
		wglSwapIntervalEXT(1);
#endif
		break;
	case MW_DSP_VSYNC_OFF:
#ifdef LIBMWVIDEORENDER_LINUX_DEF

#else
		wglSwapIntervalEXT(0);
#endif
		break;
	case MW_DSP_LOW_LATENCY:
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
		glDrawBuffer(GL_FRONT);
		break;
	default:
		break;
	}
	return MW_VIDEORENDER_NO_ERROR;

}

mw_videorender_result_e CGLRender::init_context(HWND t_hwnd)
{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	m_hwnd = t_hwnd;
	m_win = m_hwnd.win;
	m_dpy = m_hwnd.dpy;
	m_vi = m_hwnd.vi;

	m_glc = glXCreateContext(m_dpy, m_vi, NULL, GL_TRUE);
	if (!m_glc)
		return MW_VIDEORENDER_SETCONTEXT_ERROR;
	glXMakeCurrent(m_dpy, m_win, m_glc);

#else
	m_hwnd=t_hwnd;
	m_hdc = GetDC(m_hwnd);
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
		1,                              // version number
		PFD_DRAW_TO_WINDOW |            // support window
		PFD_SUPPORT_OPENGL |            // support OpenGL
		PFD_DOUBLEBUFFER,                // double buffered
		PFD_TYPE_RGBA,                  // RGBA type
		24,                             // 24-bit color depth
		0, 0, 0, 0, 0, 0,               // color bits ignored
		0,                              // no alpha buffer
		0,                              // shift bit ignored
		0,                              // no accumulation buffer
		0, 0, 0, 0,                     // accum bits ignored
		16,                             // 16-bit z-buffer
		0,                              // no stencil buffer
		0,                              // no auxiliary buffer
		PFD_MAIN_PLANE,                 // main layer
		0,                              // reserved
		0, 0, 0                         // layer masks ignored
	};
	int m_nPixelFormat = ::ChoosePixelFormat(m_hdc, &pfd);
	bool t_bs=SetPixelFormat(m_hdc, m_nPixelFormat, &pfd);
	if(!t_bs)
		return MW_VIDEORENDER_SETPIXELFORMAT_ERROR;

	m_hglrc=wglCreateContext(m_hdc);
	bool t_b=wglMakeCurrent(m_hdc, m_hglrc);
	if(!t_b)
		return MW_VIDEORENDER_SETCONTEXT_ERROR;
#endif
	if(glewInit()){
		return MW_VIDEORENDER_SETCONTEXT_ERROR;
	}

	return MW_VIDEORENDER_NO_ERROR;
}




