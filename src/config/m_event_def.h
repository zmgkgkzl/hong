#pragma once

typedef enum
{
    EVT_press, 
    EVT_release,
    EVT_click,
    EVT_double_click,
    EVT_long_click,

    EVT_change_menu,
    EVT_menu_changed,
} event_type_t;

const char* event_type_2_str(event_type_t evt);