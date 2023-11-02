#include "m_event_def.h"
#include "macros_common.h"

const char* event_type_2_str(event_type_t evt)
{
    switch (evt)
    {
        case_str(EVT_press);
        case_str(EVT_release);
        case_str(EVT_click);
        case_str(EVT_double_click);
        case_str(EVT_long_click);
        case_str(EVT_change_menu);
        case_str(EVT_menu_changed);
        default : 
            break;
    }
    return "Unknown event";
}