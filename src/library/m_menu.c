#include "m_menu.h"

#define MODULE menu
LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_LOG_LEVEL);

static struct
{
	bool initialized;
	menu_config_t const* p_menu_current;
} m_cfg;

#define execute(fn_name) do {\
							err_code = 0; \
							if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->fn_name != NULL) \
								err_code = m_cfg.p_menu_current->fn_name(); \
						} while(0)
#define execute_param(fn_name, param) do {\
							err_code = 0; \
							if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->fn_name != NULL) \
								err_code = m_cfg.p_menu_current->fn_name(param); \
						} while(0)
#define execute_param2(fn_name, param1, param2) do {\
							err_code = 0; \
							if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->fn_name != NULL) \
								err_code = m_cfg.p_menu_current->fn_name(param1, param2); \
						} while(0)

#define execute_evt(e, context) do {\
							err_code = 0; \
							if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->evt_handler) \
								err_code = m_cfg.p_menu_current->evt_handler(e, context); \
						} while(0)

static menu_config_t const* get_menu(menu_id_t id)
{
	STRUCT_SECTION_FOREACH2(menu_config_t, p_config) {
		if (p_config->menu_id == id)
			return p_config;
	}        
	
	LOG_ERR("No menu id found : %d", id);
	
	return NULL;
}

static int set_menu(menu_config_t const* p_new_menu, uint32_t enter_param)
{
	int err_code;
	menu_id_t prev_menu_id = menu_id_none;

	if (m_cfg.p_menu_current != NULL)
		prev_menu_id = m_cfg.p_menu_current->menu_id;
	
	// leave
	execute(leave_handler);		
	if (err_code != 0)
	{
		LOG_ERR("Error on menu %d, leave()\n", err_code);
	}		

    LOG_DBG("Set menu : %s", p_new_menu->name);

	// change current menu to specified
	m_cfg.p_menu_current = p_new_menu;
	
	// enter
	execute_param2(enter_handler, enter_param, prev_menu_id);		
	if (err_code != 0)
	{
		LOG_ERR("Error on menu %d, enter()\n", err_code);
	}		
	
	// notify event
	push_event2(EVT_menu_changed, 
		m_cfg.p_menu_current!=NULL?m_cfg.p_menu_current->menu_id:menu_id_none, 
		prev_menu_id);
	
	return err_code;
}


menu_config_t const* get_menu_config(menu_id_t id)
{
	return get_menu(id);
}

// run idle-time process
int m_menu_idle(void)
{
	int err_code;
	
	// run current idle handler
	execute(idle_handler);
	
	STRUCT_SECTION_FOREACH2(menu_config_t, p_config) {
		if (p_config->idle_handler != NULL
			&& p_config->call_idle)
		{
			p_config->idle_handler();
		}
	}        
	
	return err_code;
}

void m_menu_boradcast_event(event_t const * e, void* p_context)
{
	STRUCT_SECTION_FOREACH2(menu_config_t, p_config) {
		if (p_config->idle_handler != NULL
			&& p_config->call_idle)
		{
		if (p_config->evt_handler)
			p_config->evt_handler(e, p_context);
		}
	}    
}

// change menu to specified menu id
int m_menu_change(menu_id_t menu_id, uint32_t enter_param)
{
	if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->menu_id == menu_id)
	{
		LOG_INF("Already in the menu : %d\n", menu_id);
		return 0;
	}
	
	menu_config_t const* new_menu = get_menu(menu_id);
	if(new_menu == NULL)
		return -ESRCH;

	return set_menu(new_menu, enter_param);
}

// test currently select menu is specified menu
bool m_menu_is_current_menu(uint32_t menu_id)
{
	if (m_cfg.p_menu_current != NULL && m_cfg.p_menu_current->menu_id == menu_id)
		return true;
	return false;
}

menu_config_t const* m_menu_get_current(void)
{
	return m_cfg.p_menu_current;
}


void m_menu_close(void)
{
	if (m_cfg.p_menu_current != NULL)
		set_menu(NULL, 0);
}

__WEAK menu_id_t menu_get_initial_menu(void)
{
	return menu_id_none;
}

// initialize menu module
int m_menu_init(void)
{
	int err_code;
	
	UNUSED_VARIABLE(err_code);
	
	menu_config_t const* primary_menu  = NULL;
	
	STRUCT_SECTION_FOREACH2(menu_config_t, p_config) {
		if (p_config->init_handler)
			p_config->init_handler();
		
		// check primary
		if (p_config->primary)
		{
			if (primary_menu != NULL)
			{
				LOG_ERR("Primary menu already exists(new id=%d)!\n", 
					p_config->menu_id);
			}
			else
			{
				primary_menu = p_config;
			}
		}
	}  

	m_cfg.initialized = true;
	
	// retrieve initial menu from realized instance
	menu_id_t initial_menu = menu_get_initial_menu();
	if (initial_menu != menu_id_none)
	{
		primary_menu = get_menu(initial_menu);
	}

	if (primary_menu)
		set_menu(primary_menu, 0);
	
	return 0;
}

//////////////////////////////////////////////////////////////////
static void evt_handler(event_t const* evt, void* p_context)
{
	int err_code;
	
	UNUSED_VARIABLE(err_code);
	
	if (!m_cfg.initialized)
		return;
	
	if (evt->event == EVT_change_menu)
	{
		m_menu_change(evt->param1, evt->param2);
		return;
	}
			
	execute_evt(evt, p_context);
}

REGISTER_EVT_HANDLER() = {
    .handler = evt_handler,
    .p_context = NULL,
};