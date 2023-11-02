#include "m_event_queue.h"

#define MODULE event_queue
LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_LOG_LEVEL);

// Application module message queue.
#define EVT_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(queue, sizeof(event_t), CONFIG_EVENT_QUEUE_SIZE, EVT_QUEUE_BYTE_ALIGNMENT);

int m_event_push(uint32_t evt, uint32_t param1, uint32_t param2, uint32_t param3, void* p_event_data, uint32_t event_data_size)
{
	int err;

    event_t e = {
        .event = evt,
        .param1 = param1,
        .param2 = param2,
        .param3 = param3,
    };

    if (event_data_size != 0 && p_event_data != NULL)
    {
        e.p_event_data = k_malloc(event_data_size);
        e.event_data_size = event_data_size;
        
        if (e.p_event_data == NULL)
        {
            LOG_ERR("Event data alloc error");
            return ENOBUFS;
        }
        memcpy(e.p_event_data, p_event_data, event_data_size);
    }

    do
    {
        err = k_msgq_put(&queue, &e, K_NO_WAIT);

        if (err) 
        {
            LOG_WRN("event_queue could not be enqueued, error code: %d", err);
            break;
        }

    } while (0);
    
    return err;
}

static void __free_event(event_t* e)
{
    if (e==NULL || e->p_event_data == NULL)
        return;
    k_free(e->p_event_data);
    e->p_event_data = NULL;
    e->event_data_size = 0;
}

void m_event_dispatch(void)
{
    static event_t e;
	int err;
    
    do
    {
        err = k_msgq_get(&queue, &e, K_FOREVER);
        if (err) break;

        LOG_INF("EVT : %s(%d) (%d,%d,%d)", 
            event_type_2_str(e.event), e.event,
            e.param1, e.param2, e.param3);

        STRUCT_SECTION_FOREACH2(event_queue_observer_t, p_config) {
            p_config->handler(&e, p_config->p_context);
        }  

        // free memory allocated by event data
        __free_event(&e);
    } while (0);
} 

void m_event_queue_clear(void)
{
    event_t e;

    while (k_msgq_get(&queue, &e, K_NO_WAIT) == 0)
    {
        __free_event(&e);
    }
}

