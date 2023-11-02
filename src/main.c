// ZIGBEE coordinator

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "bsp.h"
#include "m_coordinator.h"
#include "m_event_queue.h"
#include "m_menu.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static void initialize(void)
{
	LOG_INF("Starting ZBOSS Coordinator example");
 
	bsp_init();
	m_menu_init();
	
	LOG_INF("ZBOSS Coordinator example started");
	m_coordinator_init();

	bsp_indication_set(BSP_INDICATE_READY);
}

int main(void)
{
	initialize();

	while (1)
	{
		m_event_dispatch();
	}
	
	return 0;
}
 