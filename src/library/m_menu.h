#pragma once

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "macros_common.h"
#include "m_event_queue.h"
#include "m_menu_id.h"

typedef int (*menu_handler_t)(void);
typedef int (*menu_handler_enter_t)(uint32_t param, menu_id_t prev_menu_id);
typedef int (*menu_handler_evt_t)(event_t const * e, void* p_context);

typedef struct
{
	menu_id_t			    menu_id;		// menu id
	const char*				name;
	
	uint8_t					primary:1;		// indicates primary service, only one instance can have primary
	uint8_t					call_idle:1;	// indicates idle handler must be called when not in the menu
	uint8_t					reserved:7;		
	
	menu_handler_t			init_handler;	// called when the menu is being initialized
	menu_handler_enter_t	enter_handler;	// called when menu is being selected
	menu_handler_t 			leave_handler;	// called when menu is being leaved
	menu_handler_evt_t		evt_handler;	// event handler
	menu_handler_t 			idle_handler;	// called when the system is in idle state
} menu_config_t;

#define REGISTER_MENU_HANDLER() \
    static const STRUCT_SECTION_ITERABLE2(menu_config_t, cfg_var)

int     m_menu_init(void);
void    m_menu_close(void);
int     m_menu_idle(void);
int     m_menu_change(menu_id_t menu_id, uint32_t enter_param);
void 	m_menu_boradcast_event(event_t const * e, void* p_context);
bool    m_menu_is_current_menu(uint32_t menu_id);

menu_config_t const* m_menu_get_current(void);
menu_config_t const* get_menu_config(menu_id_t id);

#define change_menu(id)					push_event1(EVT_change_menu, id)
#define change_menu_param(id, param)	push_event2(EVT_change_menu, id, param)
