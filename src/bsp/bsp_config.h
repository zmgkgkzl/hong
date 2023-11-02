#pragma once

//////////////////////////////////////////////////////
// indication type
typedef enum
{
    BSP_INDICATE_IDLE,
    BSP_INDICATE_READY,
    BSP_INDICATE_NETWORK_OPEN,
    BSP_INDICATE_NETWORK_CLOSED,
    BSP_INDICATE_IDENTIFY_TOGGLE,
    BSP_INDICATE_IDENTIFY_STOP,
    BSP_INDICATE_NETWORK_STEERING,          // indicating that device successfully joined Zigbee network
    BSP_INDICATE_NETWORK_STEERING_FINISH,
    BSP_INDICATE_ALERT,
} bsp_indication_t;

//////////////////////////////////////////////////////
// define timeout
#define BSP_INTERVAL_ADV_ON_INTVERVAL_MS        100
#define BSP_INTERVAL_ADV_OFF_INTVERVAL_MS       900
#define BSP_INTERVAL_ALERT_INTVERVAL_MS         100

//////////////////////////////////////////////////////
// define LED index
#define BSP_INDICATE_LED_ADV            0
#define BSP_INDICATE_LED_NET_OPEN       1
#define BSP_INDICATE_LED_IDENTIFY       3
#define BSP_INDICATE_LED_NETWORK_STATE  2
