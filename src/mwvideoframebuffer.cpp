#include "mwvideoframebuffer.h"


CMWVideoFrameBuffer::CMWVideoFrameBuffer()
{
    m_arr_frames=NULL;
    m_n_frame_count=0;

#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_init(&m_lock_free, NULL);
	pthread_mutex_init(&m_lock_full, NULL);
#else
    InitializeCriticalSection(&m_lock_free);
    InitializeCriticalSection(&m_lock_full);
#endif
}

CMWVideoFrameBuffer::~CMWVideoFrameBuffer()
{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_destroy(&m_lock_free);
	pthread_mutex_destroy(&m_lock_full);
#else
    DeleteCriticalSection(&m_lock_free);
    DeleteCriticalSection(&m_lock_full);
#endif
}

mw_videorender_result_e 
CMWVideoFrameBuffer::create_frame_buffer(
	uint32_t u32_count, 
	uint32_t u32_size, 
	void **pptr, 
	uint32_t *pu32_glptr, 
	mw_queue_mode_e e_map_mode,
	bool b_alloc_buf
)
{
    mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;

	m_map_mode = e_map_mode;
	m_u32_size = u32_size;

	if ((MW_QUEUE_IMMEDIATE != m_map_mode) 
		&& (MW_QUEUE_MAILBOX != m_map_mode)
		&& (MW_QUEUE_RELAXED_FIFO != m_map_mode)
		&& (MW_QUEUE_FIFO != m_map_mode)
		)
		return MW_VIDEORENDER_MAP_MODE_ERROR;

    m_arr_frames = new mw_render_frame_t[u32_count];
    if (m_arr_frames == NULL)
        return MW_VIDEORENDER_BUF_CREATE_FAIL;
    
    for (uint32_t i=0; i < u32_count; i++) {
        m_arr_frames[i].m_p_data = NULL;
		if (b_alloc_buf) {
			m_arr_frames[i].m_p_data = pptr[i];
			m_arr_frames[i].t_glvar = pu32_glptr[i];
		}
		else {
			m_arr_frames[i].m_p_data = malloc(u32_size);
		}
		
        if (m_arr_frames[i].m_p_data == NULL) {
            for(uint32_t j=0;j<i;j++) {
                free(m_arr_frames[j].m_p_data);
                m_arr_frames[j].m_p_data=NULL;
            }
            t_rslt_e=MW_VIDEORENDER_BUF_CREATE_FAIL;
            return t_rslt_e;
        }
    }

    m_n_frame_count=u32_count;
    for(uint32_t i=0; i < u32_count; i++) {
        m_arr_frames[i].m_state=MW_FRAME_BUFFER_STATE_FREE;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_init(&m_arr_frames[i].m_lock, NULL);
#else
        InitializeCriticalSection(&m_arr_frames[i].m_lock);
#endif
        m_queue_free.push(&m_arr_frames[i]);
    }

    return t_rslt_e;
}

mw_videorender_result_e 
CMWVideoFrameBuffer::destory_frame_buffer(bool b_alloc_buf)
{
    mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
    if(m_arr_frames==NULL)
        return MW_VIDEORENDER_BUF_NONE;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_lock(&m_lock_free);
#else
	EnterCriticalSection(&m_lock_free);
#endif
	while (!m_queue_free.empty()) {
		m_queue_free.pop();
	}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_unlock(&m_lock_free);
	pthread_mutex_lock(&m_lock_full);
#else
	LeaveCriticalSection(&m_lock_free);
	EnterCriticalSection(&m_lock_full);
#endif
	while (!m_queue_full.empty()) {
		m_queue_full.pop();
	}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_unlock(&m_lock_full);
#else
	LeaveCriticalSection(&m_lock_full);
#endif
	m_u32_size = 0;

    for(uint32_t i=0; i < m_n_frame_count; i++) {
        if (m_arr_frames[i].m_p_data != NULL) {
			if (!b_alloc_buf)
				delete m_arr_frames[i].m_p_data;
			
            m_arr_frames[i].m_p_data = NULL;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_destroy(&m_arr_frames[i].m_lock);
#else
            DeleteCriticalSection(&m_arr_frames[i].m_lock);
#endif
        }
    }

    delete m_arr_frames;
    m_arr_frames = NULL;

    return t_rslt_e;
}

mw_videorender_result_e 
CMWVideoFrameBuffer::put_frame_data(
	void *p_data, 
	uint32_t u32_size, 
	uint32_t u32_width,
	uint32_t u32_height
)
{
    if((p_data==NULL) || (u32_size==0))
        return MW_VIDEORENDER_BUF_NONE;

	if (u32_size > m_u32_size)
		return MW_VIDEORENDER_BUF_ERR;

	mw_videorender_result_e t_rslt_e = MW_VIDEORENDER_NO_ERROR;
	mw_render_frame_t *t_p_frame = NULL;
	if (MW_QUEUE_IMMEDIATE == m_map_mode) {
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_lock(&m_lock_full);
#else
		EnterCriticalSection(&m_lock_full);
#endif
		if (!m_queue_full.empty()) {
			t_p_frame = m_queue_full.front();
			m_queue_full.pop();
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_lock(&t_p_frame->m_lock);
#else
			EnterCriticalSection(&t_p_frame->m_lock);
#endif
			t_p_frame->m_width = u32_width;
			t_p_frame->m_height = u32_height;
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
			//may check the size
			memcpy(t_p_frame->m_p_data, p_data, u32_size);
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&t_p_frame->m_lock);
			pthread_mutex_lock(&m_lock_free);
#else
			LeaveCriticalSection(&t_p_frame->m_lock);
			EnterCriticalSection(&m_lock_free);
#endif
			while (!m_queue_full.empty()) {
				mw_render_frame_t *m_p_frame = m_queue_full.front();
				m_queue_full.pop();
				m_queue_free.push(m_p_frame);
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
#else
			LeaveCriticalSection(&m_lock_free);
#endif
			m_queue_full.push(t_p_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_full);
#endif
		}
		else {
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
			pthread_mutex_lock(&m_lock_free);
#else
			LeaveCriticalSection(&m_lock_full);
			EnterCriticalSection(&m_lock_free);
#endif			
			if (!m_queue_free.empty()){
				t_p_frame = m_queue_free.front();
				m_queue_free.pop();
			}
			else{
				t_rslt_e = MW_VIDEORENDER_BUF_NONE;
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
#else
			LeaveCriticalSection(&m_lock_free);
#endif
			if(MW_VIDEORENDER_NO_ERROR != t_rslt_e)
				return t_rslt_e;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_lock(&t_p_frame->m_lock);
#else
			EnterCriticalSection(&t_p_frame->m_lock);
#endif
			t_p_frame->m_width = u32_width;
			t_p_frame->m_height = u32_height;
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
			memcpy(t_p_frame->m_p_data, p_data, u32_size);
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&t_p_frame->m_lock);
			pthread_mutex_lock(&m_lock_full);
#else
			LeaveCriticalSection(&t_p_frame->m_lock);
			EnterCriticalSection(&m_lock_full);
#endif
			m_queue_full.push(t_p_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_full);
#endif
		}
		return t_rslt_e;
    }
	else if (MW_QUEUE_MAILBOX == m_map_mode)
	{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_lock(&m_lock_free);
#else
		EnterCriticalSection(&m_lock_free);
#endif
		if (m_queue_free.empty()) {
			//There is no free buffer available.
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
			pthread_mutex_lock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_free);
			EnterCriticalSection(&m_lock_full);
#endif
			if (!m_queue_full.empty())
			{
				t_p_frame = m_queue_full.front();
				m_queue_full.pop();
			}
			else
			{
				t_rslt_e = MW_VIDEORENDER_BUF_NONE;
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_full);
#endif

			if (MW_VIDEORENDER_NO_ERROR != t_rslt_e)
				return t_rslt_e;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_lock(&t_p_frame->m_lock);
#else
			EnterCriticalSection(&t_p_frame->m_lock);
#endif
			t_p_frame->m_width = u32_width;
			t_p_frame->m_height = u32_height;
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
			//may check the size
			memcpy(t_p_frame->m_p_data, p_data, u32_size);
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&t_p_frame->m_lock);
			pthread_mutex_lock(&m_lock_full);
			pthread_mutex_lock(&m_lock_free);
#else
			LeaveCriticalSection(&t_p_frame->m_lock);
			EnterCriticalSection(&m_lock_full);
			EnterCriticalSection(&m_lock_free);
#endif
			while (!m_queue_full.empty())
			{
				mw_render_frame_t *m_p_frame = m_queue_full.front();
				m_queue_full.pop();
				m_queue_free.push(m_p_frame);
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_free);
			LeaveCriticalSection(&m_lock_full);
#endif
		}
		else {
			t_p_frame = m_queue_free.front();
			m_queue_free.pop();
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
			pthread_mutex_lock(&t_p_frame->m_lock);
#else
			LeaveCriticalSection(&m_lock_free);	
			EnterCriticalSection(&t_p_frame->m_lock);
#endif			
			t_p_frame->m_width = u32_width;
			t_p_frame->m_height = u32_height;
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
			memcpy(t_p_frame->m_p_data, p_data, u32_size);
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;

#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&t_p_frame->m_lock);
#else
			LeaveCriticalSection(&t_p_frame->m_lock);
#endif
		}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_lock(&m_lock_full);
#else
		EnterCriticalSection(&m_lock_full);
#endif
		m_queue_full.push(t_p_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_unlock(&m_lock_full);
#else
		LeaveCriticalSection(&m_lock_full);
#endif
		return t_rslt_e;
	}
	else if (MW_QUEUE_RELAXED_FIFO == m_map_mode)
	{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_lock(&m_lock_free);
#else
		EnterCriticalSection(&m_lock_free);
#endif
		if (m_queue_free.empty()) {
			//There is no free buffer available.
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
			pthread_mutex_lock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_free);
			EnterCriticalSection(&m_lock_full);
#endif
			if (!m_queue_full.empty())
			{
				t_p_frame = m_queue_full.back();
#ifdef LIBMWVIDEORENDER_LINUX_DEF
				pthread_mutex_lock(&t_p_frame->m_lock);
#else
				EnterCriticalSection(&t_p_frame->m_lock);
#endif
				t_p_frame->m_width = u32_width;
				t_p_frame->m_height = u32_height;
				t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
				memcpy(t_p_frame->m_p_data, p_data, u32_size);
				t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
				pthread_mutex_unlock(&t_p_frame->m_lock);
#else
				LeaveCriticalSection(&t_p_frame->m_lock);
#endif
			}
			else
			{
				t_rslt_e = MW_VIDEORENDER_BUF_NONE;
			}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_full);
#endif
		}
		else {
			t_p_frame = m_queue_free.front();
			m_queue_free.pop();
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
			pthread_mutex_lock(&t_p_frame->m_lock);
#else
			LeaveCriticalSection(&m_lock_free);
			EnterCriticalSection(&t_p_frame->m_lock);
#endif
			t_p_frame->m_width = u32_width;
			t_p_frame->m_height = u32_height;
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
			//may check the size
			memcpy(t_p_frame->m_p_data, p_data, u32_size);
			t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&t_p_frame->m_lock);
			pthread_mutex_lock(&m_lock_full);
#else
			LeaveCriticalSection(&t_p_frame->m_lock);
			EnterCriticalSection(&m_lock_full);
#endif
			m_queue_full.push(t_p_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_full);
#else
			LeaveCriticalSection(&m_lock_full);
#endif
		}
		return t_rslt_e;
	}
	else if (MW_QUEUE_FIFO == m_map_mode)
	{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_lock(&m_lock_free);
#else
		EnterCriticalSection(&m_lock_free);
#endif
		if (m_queue_free.empty()) {
			//There is no free buffer available.
#ifdef LIBMWVIDEORENDER_LINUX_DEF
			pthread_mutex_unlock(&m_lock_free);
#else
			LeaveCriticalSection(&m_lock_free);
#endif
			return MW_VIDEORENDER_BUF_NONE;
		}
		else {
			t_p_frame = m_queue_free.front();
			m_queue_free.pop();
		}
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_unlock(&m_lock_free);
		pthread_mutex_lock(&t_p_frame->m_lock);
#else
		LeaveCriticalSection(&m_lock_free);		
		EnterCriticalSection(&t_p_frame->m_lock);
#endif
		t_p_frame->m_width = u32_width;
		t_p_frame->m_height = u32_height;
		t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLING;
		//may check the size
		memcpy(t_p_frame->m_p_data, p_data, u32_size);
		t_p_frame->m_state = MW_FRAME_BUFFER_STATE_FILLED;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_unlock(&t_p_frame->m_lock);
		pthread_mutex_lock(&m_lock_full);
#else
		LeaveCriticalSection(&t_p_frame->m_lock);
		EnterCriticalSection(&m_lock_full);
#endif
		m_queue_full.push(t_p_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_unlock(&m_lock_full);
#else
		LeaveCriticalSection(&m_lock_full);
#endif
		return t_rslt_e;
	}

}

mw_videorender_result_e 
CMWVideoFrameBuffer::get_frame_buffer(mw_render_frame_t **pp_frame)
{
    mw_videorender_result_e t_ret=MW_VIDEORENDER_NO_ERROR;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_lock(&m_lock_full);
#else
    EnterCriticalSection(&m_lock_full);
#endif
    if(m_queue_full.empty()){
        *pp_frame=NULL;
#ifdef LIBMWVIDEORENDER_LINUX_DEF
		pthread_mutex_unlock(&m_lock_full);
#else
        LeaveCriticalSection(&m_lock_full);
#endif
		return MW_VIDEORENDER_BUF_ERR;
    }
    *pp_frame=m_queue_full.front();
    m_queue_full.pop();
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_unlock(&m_lock_full);
#else
    LeaveCriticalSection(&m_lock_full);
#endif
    return t_ret;
}

void 
CMWVideoFrameBuffer::put_frame_buffer(mw_render_frame_t **pp_frame)
{
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_lock(&m_lock_free);
#else
    EnterCriticalSection(&m_lock_free);
#endif
    m_queue_free.push(*pp_frame);
#ifdef LIBMWVIDEORENDER_LINUX_DEF
	pthread_mutex_unlock(&m_lock_free);
#else
    LeaveCriticalSection(&m_lock_free);
#endif
}
