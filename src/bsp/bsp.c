#include "bsp.h"
#include "bsp_config.h"
#include "m_event_queue.h"
#include "utility.h"

#include <dk_buttons_and_leds.h>
#include <zigbee/zigbee_app_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bsp, CONFIG_BSP_MODULE_LOG_LEVEL);

static void on_led_handler(struct k_timer* timer);
K_TIMER_DEFINE(led_timer, on_led_handler, NULL);
static struct k_spinlock lock;

typedef union
{
	uint8_t flag;
	struct
	{
		uint8_t			 network_open:1;
	};
} indicate_flag_t;

typedef enum
{
	bs_init,
	bs_pressed,
	bs_double_click,
} button_state_t;

typedef struct
{
	uint64_t 		tick;
	uint8_t			button_state;
	button_state_t 	state;
} button_t;

static struct {
	bool 			 clear_leds;
	bsp_indication_t stable_state;
	uint32_t		 led_state;
	indicate_flag_t  indicate_flag;

	button_t		buttons[CONFIG_BSP_BUTTON_COUNT];
} m_cfg;

static void on_long_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(long_timer, on_long_timer_handler, NULL);

static void on_dblclk_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(dblclk_timer, on_dblclk_timer_handler, NULL);

#define LONG_TIMER_STARTED()   ( k_timer_remaining_get(&long_timer)   != 0 )
#define DBLCLK_TIMER_STARTED() ( k_timer_remaining_get(&dblclk_timer) != 0 )

static void handle_button_event(uint8_t index, uint8_t button_action);

static void notify_event(event_type_t event, uint8_t index)
{
	push_event1(event, index);
}

static void on_long_timer_handler(struct k_timer *timer)
{
	int remain_count = 0;
	
	for (uint32_t i=0; i<CONFIG_BSP_BUTTON_COUNT; i++)
	{
		button_t* p_button = &m_cfg.buttons[i];
		if (p_button->state == bs_pressed ||
			p_button->state == bs_double_click)
		{
			if (k_uptime_elapse(&p_button->tick) >= CONFIG_BSP_LONG_CLICK_TIMEOUT_MS)
			{
				LOG_INF("Button %d Long push", i);
				p_button->state = bs_init;						// init state
				
				// generate long touch event
				notify_event(EVT_long_click, i);
			}
			else
			{
				remain_count++;
			}		
		}
	}

	if (!remain_count)
	{
		k_timer_stop(&long_timer);
		LOG_ERR("Stop long");
	}
}

static void on_dblclk_timer_handler(struct k_timer *timer)
{
	int64_t delta;
	int remain_count = 0;

	for (uint32_t i=0; i<CONFIG_BSP_BUTTON_COUNT; i++)
	{
		button_t* p_button = &m_cfg.buttons[i];
		if (p_button->state == bs_double_click)
		{
			delta = k_uptime_elapse(&p_button->tick);
			if (delta >= CONFIG_BSP_DBL_CLK_TOLERANCE_MS)
			{
				LOG_INF("Clicked : %d\n", i);
				p_button->state = bs_init;
				
				// generate click event
				notify_event(EVT_click, i);
			}
			else
			{
				remain_count++;
			}
		}	
	}

	if (!remain_count)
	{
		k_timer_stop(&dblclk_timer);
		LOG_ERR("Stop dblclk");
	}
}

static void handle_button_event(uint8_t index, uint8_t button_action)
{
	button_t* p_button = &m_cfg.buttons[index];
	int64_t delta;

	// notify button event
	notify_event(button_action?EVT_press:EVT_release, index);
	
	switch (p_button->state)
	{
		case bs_init :		// initial state
			if (button_action)
			{				
				LOG_INF("Button %d push", index);
				p_button->tick = k_uptime_get();
				p_button->state = bs_pressed;					// click detect
				// start long push timer when previously stopped 
				if (!LONG_TIMER_STARTED())
				{
					LOG_DBG("Start long touch timer : %d", index);
					k_timer_start(&long_timer, K_MSEC(CONFIG_BSP_LONG_CLICK_TIMEOUT_MS), K_NO_WAIT);
				}
			}
			else
			{
				LOG_DBG("Button %d released", index);
				p_button->state = bs_init;						// init state
			}
			break;
		case bs_pressed:	// pressed state
			if (button_action == 0)
			{
				delta = k_uptime_elapse(&p_button->tick);
				LOG_ERR("min btn : %lld , %d", delta, CONFIG_BSP_MIN_BUTTON_DURATION_MS);
				if (delta >= CONFIG_BSP_MIN_BUTTON_DURATION_MS)
				{
					p_button->state = bs_double_click;
					
					// save last button click time
					p_button->tick = k_uptime_get();
					
					if (!DBLCLK_TIMER_STARTED())
					{
						LOG_DBG("Start dblclk timer %d", index);
						k_timer_start(&dblclk_timer, K_MSEC(CONFIG_BSP_DBL_CLK_CHK_PERIOD_MS), K_MSEC(CONFIG_BSP_DBL_CLK_CHK_PERIOD_MS));
					}
					else
					{
						LOG_DBG("No double-click");
					}
				}
				else
				{
					LOG_DBG("Press init");
					p_button->state = bs_init;						// init state
				}
			}
			break;
		case bs_double_click : // check double click
			if (button_action == 0)
			{
				delta = k_uptime_elapse(&p_button->tick);
				if (delta >= CONFIG_BSP_MIN_BUTTON_DURATION_MS)
				{
					// generate double click event
					LOG_DBG("Button %d double click", index);
					
					notify_event(EVT_double_click, index);
					
					// save last button click time
					p_button->tick = k_uptime_get();
				}
				p_button->state = bs_init;						// init state
			}
			else if (button_action == 1)
			{
				LOG_DBG("Dblclk %d pressed", index);

				// save last button click time
				p_button->tick = k_uptime_get();
				
				if (!LONG_TIMER_STARTED())
				{
					LOG_DBG("Start long touch timer %d", index);
					k_timer_start(&long_timer, K_MSEC(CONFIG_BSP_LONG_CLICK_TIMEOUT_MS), K_NO_WAIT);
				}
			}
			
			break;
	}

}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons that has changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	for (uint8_t i=0; i<CONFIG_BSP_BUTTON_COUNT; i++)
	{
		if (has_changed & BIT(i))
		{
			handle_button_event(i, !!(button_state & BIT(i)));
		}
	}
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

static void set_led(uint8_t led_index, uint32_t value)
{
	m_cfg.led_state &= ~(1<<led_index);
	m_cfg.led_state |= ((!!(value))<<led_index);

	dk_set_led(led_index, value);
}

static void set_led_off(uint8_t led_index)
{
	set_led(led_index, 0);
}

static void set_led_on(uint8_t led_index)
{
	set_led(led_index, 1);
}

static void set_leds(uint32_t value)
{
	m_cfg.led_state = 	  value 
						| (m_cfg.indicate_flag.network_open  ? (BSP_INDICATE_LED_NET_OPEN) : 0);

	dk_set_leds(value);
}

static uint32_t get_led_state(int index)
{
	return !!(m_cfg.led_state & (1<<index));
}

static void on_led_handler(struct k_timer* timer)
{
	bsp_indication_set(m_cfg.stable_state);
}

void bsp_indication_set(bsp_indication_t indicate)
{
	uint32_t interval;

	k_spinlock_key_t key = k_spin_lock(&lock);

	//LOG_INF("Indication state : %d", indicate);

	if (m_cfg.clear_leds)
	{
		set_leds(0);
		m_cfg.clear_leds = false;
	}

	switch (indicate)
	{
	case BSP_INDICATE_IDLE  :  
		k_timer_stop(&led_timer);
		set_leds(0);
		m_cfg.stable_state = indicate;
		break;
	case BSP_INDICATE_READY : 
		if (get_led_state(BSP_INDICATE_LED_ADV)) 
		{
			interval = BSP_INTERVAL_ADV_OFF_INTVERVAL_MS; 
			set_led_off(BSP_INDICATE_LED_ADV);
		}
		else
		{
			interval = BSP_INTERVAL_ADV_ON_INTVERVAL_MS; 
			set_led_on(BSP_INDICATE_LED_ADV);
		}
		m_cfg.stable_state = indicate;
		k_timer_start(&led_timer, K_MSEC(interval), K_NO_WAIT);
		break;
	case BSP_INDICATE_NETWORK_OPEN   : 
		m_cfg.indicate_flag.network_open = 1;
		set_led_on(BSP_INDICATE_LED_NET_OPEN);
		break;
	case BSP_INDICATE_NETWORK_CLOSED : 
		m_cfg.indicate_flag.network_open = 0;
		set_led_off(BSP_INDICATE_LED_NET_OPEN);
		break;
	case BSP_INDICATE_IDENTIFY_TOGGLE : 
		set_led(BSP_INDICATE_LED_IDENTIFY, 
			get_led_state(BSP_INDICATE_LED_IDENTIFY) ? 0 : 1);
		break;
	case BSP_INDICATE_IDENTIFY_STOP  : 
		set_led_off(BSP_INDICATE_LED_IDENTIFY);
		break;
	case BSP_INDICATE_NETWORK_STEERING : 
		set_led_on(BSP_INDICATE_LED_NETWORK_STATE);
		break;
	case BSP_INDICATE_NETWORK_STEERING_FINISH  : 
		set_led_off(BSP_INDICATE_LED_NETWORK_STATE);
		break;
	case BSP_INDICATE_ALERT :
		interval = BSP_INTERVAL_ALERT_INTVERVAL_MS;
		m_cfg.indicate_flag.flag = 0;
		set_leds(m_cfg.led_state ? 0 : 0xff);
		k_timer_start(&led_timer, K_MSEC(interval), K_NO_WAIT);
		break;
	default:
		break;
	}

	k_spin_unlock(&lock, key);	
}

int bsp_init(void)
{
	// Initialize 
	configure_gpio();

    return 0;
}