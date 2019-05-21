#ifndef MWVIDEOFRAMEBUFFER_H
#define MWVIDEOFRAMEBUFFER_H

#ifdef LIBMWVIDEORENDER_LINUX_DEF
  #include <pthread.h>
  #include <string.h>
  #include <stdlib.h>
#else
  #include "windows.h"
#endif

#include "LibMWVideoRender.h"
#include <queue>

using namespace std;

typedef enum _frame_state{
    MW_FRAME_BUFFER_STATE_FREE=0,
    MW_FRAME_BUFFER_STATE_FILLED,
    MW_FRAME_BUFFER_STATE_FILLING,
    MW_FRAME_BUFFER_STATE_RENDERING,
    MW_FRAME_BUFFER_STATE_COUNT
}mw_frame_state_e;

typedef struct _render_frame{
    void*                       m_p_data;
	uint32_t                    t_glvar;
	uint32_t                    m_width;
	uint32_t                    m_height;
    mw_frame_state_e            m_state;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_t             m_lock;
#else
    CRITICAL_SECTION            m_lock;
#endif
}mw_render_frame_t;

class CMWVideoFrameBuffer
{
public:
    CMWVideoFrameBuffer();
    ~CMWVideoFrameBuffer();

public:
    mw_videorender_result_e create_frame_buffer(uint32_t u32_count,
		                                        uint32_t u32_size, 
		                                        void **pptr, 
		                                        uint32_t *pu32_glptr,
		                                        mw_queue_mode_e e_map_mode,
		                                        bool b_alloc_buf);

    mw_videorender_result_e destory_frame_buffer(bool b_alloc_buf);

    mw_videorender_result_e put_frame_data(void *p_data,
                                           uint32_t u32_size,
		                                   uint32_t u32_width,
		                                   uint32_t u32_height);

    mw_videorender_result_e get_frame_buffer(mw_render_frame_t **pp_frame);
    void put_frame_buffer(mw_render_frame_t **pp_frame);

protected:
    mw_render_frame_t *m_arr_frames;
    uint32_t m_n_frame_count;
    queue<mw_render_frame_t *> m_queue_free;
    queue<mw_render_frame_t *> m_queue_full;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_t     m_lock_free;
	pthread_mutex_t     m_lock_full;
#else
    CRITICAL_SECTION    m_lock_free;
    CRITICAL_SECTION    m_lock_full;
#endif
	mw_queue_mode_e m_map_mode;
	uint32_t m_u32_size;

};

#endif // MWVIDEOFRAMEBUFFER_H
