#include "bsp.h"
#include "m_coordinator.h"
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>
#include <zboss_api.h>
#include <zb_mem_config_max.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>
#include "zb_range_extender.h"
#include "m_event_queue.h"
#include "config.h"

/* Device endpoint, used to receive ZCL commands. */
#define ZIGBEE_COORDINATOR_ENDPOINT            10

/* Type of power sources available for the device.
 * For possible values see section 3.2.2.2.8 of ZCL specification.
 */
#define COORDINATOR_INIT_BASIC_POWER_SOURCE    ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE

/* If set to ZB_TRUE then device will not open the network after forming or reboot. */
#define ZIGBEE_MANUAL_STEERING                 ZB_FALSE

#define ZIGBEE_PERMIT_LEGACY_DEVICES           ZB_FALSE

#ifndef ZB_COORDINATOR_ROLE
#error Define ZB_COORDINATOR_ROLE to compile coordinator source code.
#endif

LOG_MODULE_REGISTER(coord, LOG_LEVEL_INF);

/* Main application customizable context.
 * Stores all settings and static values.
 */
struct zb_device_ctx {
	zb_zcl_basic_attrs_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
};

/* Zigbee device application context storage. */
static struct zb_device_ctx dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source);

ZB_DECLARE_RANGE_EXTENDER_CLUSTER_LIST(
	nwk_coordinator_clusters,
	basic_attr_list,
	identify_attr_list);

ZB_DECLARE_RANGE_EXTENDER_EP(
	nwk_coordinator_ep,
	ZIGBEE_COORDINATOR_ENDPOINT,
	nwk_coordinator_clusters);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(
	nwk_coordinator,
	nwk_coordinator_ep);


/**@brief Function for initializing all clusters attributes. */
static void app_clusters_attr_init(void)
{
	/* Basic cluster attributes data. */
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = COORDINATOR_INIT_BASIC_POWER_SOURCE;

	/* Identify cluster attributes data. */
	dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
}

/**@brief Function to toggle the identify LED.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	bsp_indication_set(BSP_INDICATE_IDENTIFY_TOGGLE);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED. */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED. */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		bsp_indication_set(BSP_INDICATE_IDENTIFY_STOP);
	}
}

/**@brief Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	if (ZB_JOINED()) {
		/* Check if endpoint is in identifying mode,
		 * if not, put desired endpoint in identifying mode.
		 */
		if (dev_ctx.identify_attr.identify_time ==
		    ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

			zb_ret_t zb_err_code = zb_bdb_finding_binding_target(
				ZIGBEE_COORDINATOR_ENDPOINT);

			if (zb_err_code == RET_OK) {
				LOG_INF("Enter identify mode");
			} else if (zb_err_code == RET_INVALID_STATE) {
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			} else {
				ZB_ERROR_CHECK(zb_err_code);
			}
		} else {
			LOG_INF("Cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	} else {
		LOG_WRN("Device not in a network - cannot enter identify mode");
	}
}

/**@brief Callback used in order to visualise network steering period.
 *
 * @param[in]   param   Not used. Required by callback type definition.
 */
static void steering_finished(zb_uint8_t param)
{
	ARG_UNUSED(param);

	LOG_INF("Network steering finished");
	bsp_indication_set(BSP_INDICATE_NETWORK_STEERING_FINISH);
}



/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Read signal description out of memory buffer. */
	zb_zdo_app_signal_hdr_t *sg_p = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sg_p);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);
	zb_ret_t zb_err_code;
	zb_bool_t comm_status;
	zb_time_t timeout_bi;

	switch (sig) {
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
		/* BDB initialization completed after device reboot,
		 * use NVRAM contents during initialization.
		 * Device joined/rejoined and started.
		 */
		if (status == RET_OK) 
		{
			if (ZIGBEE_MANUAL_STEERING == ZB_FALSE) 
			{
				LOG_INF("Start network steering");
				comm_status = bdb_start_top_level_commissioning(
					ZB_BDB_NETWORK_STEERING);
				ZB_COMM_STATUS_CHECK(comm_status);
			} 
			else 
			{
				LOG_INF("Coordinator restarted successfully");
			}
		} 
		else 
		{
			LOG_ERR("Failed to initialize Zigbee stack using NVRAM data (status: %d)",
				status);
		}
		break;

	case ZB_BDB_SIGNAL_STEERING:
		if (status == RET_OK) 
		{
			if (ZIGBEE_PERMIT_LEGACY_DEVICES == ZB_TRUE) 
			{
				LOG_INF("Allow pre-Zigbee 3.0 devices to join the network");
				zb_bdb_set_legacy_device_support(1);
			}

			/* Schedule an alarm to notify about the end of steering period.
			 */
			LOG_INF("Network steering started");
			zb_err_code = ZB_SCHEDULE_APP_ALARM(
				steering_finished, 0,
				ZB_TIME_ONE_SECOND *
				ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW);
			ZB_ERROR_CHECK(zb_err_code);
		}
		break;

	case ZB_ZDO_SIGNAL_DEVICE_ANNCE: 
	{
		zb_zdo_signal_device_annce_params_t *dev_annce_params =
			ZB_ZDO_SIGNAL_GET_PARAMS(
				sg_p, zb_zdo_signal_device_annce_params_t);

		LOG_INF("New device commissioned or rejoined (short: 0x%04hx)",
			dev_annce_params->device_short_addr);

		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(steering_finished, ZB_ALARM_ANY_PARAM);
		if (zb_err_code == RET_OK) 
		{
			LOG_INF("Joining period extended.");
			zb_err_code = ZB_SCHEDULE_APP_ALARM(
				steering_finished, 0,
				ZB_TIME_ONE_SECOND *
				ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW);
			ZB_ERROR_CHECK(zb_err_code);
		}
	} 
	break;

	default:
		/* Call default signal handler. */
		ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
		break;
	}

	/* Update network status LED. */
	if (ZB_JOINED() &&
	    (ZB_SCHEDULE_GET_ALARM_TIME(steering_finished, ZB_ALARM_ANY_PARAM,
					&timeout_bi) == RET_OK))
	{
		bsp_indication_set(BSP_INDICATE_NETWORK_STEERING);
	} 
	else 
	{
		bsp_indication_set(BSP_INDICATE_NETWORK_STEERING_FINISH);
	}

	/*
	 * All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

int m_coordinator_init(void)
{
	// Register device context (endpoints). 
	ZB_AF_REGISTER_DEVICE_CTX(&nwk_coordinator);

	app_clusters_attr_init();

	// Register handlers to identify notifications. 
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ZIGBEE_COORDINATOR_ENDPOINT, identify_cb);

	// Start Zigbee default thread 
	zigbee_enable();

    return 0;    
}

/////////////////////////////////////////////////////////////////////////////////
// Event handler
static void on_identify(void)
{
	LOG_INF("--- Identify mode ---");
	ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
}

static void on_network_reopen(void)
{
	zb_bool_t comm_status;

	LOG_INF("--- Reopen network ---");

	(void)(ZB_SCHEDULE_APP_ALARM_CANCEL(steering_finished, ZB_ALARM_ANY_PARAM));

	comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
	if (comm_status) 
	{
		LOG_INF("Top level commissioning restated");
	} 
	else 
	{
		LOG_INF("Top level commissioning hasn't finished yet!");
	}
}


static void on_long_click(event_t const* e)
{
	if (e->param1 != FACTORY_RESET_BUTTON)
		return;

	LOG_INF("--- Factory Reset ---");
	ZB_SCHEDULE_APP_CALLBACK(zb_bdb_reset_via_local_action, 0);
}

static void on_click(event_t const* e)
{
	switch (e->param1)
	{
		case IDENTIFY_MODE_BUTTON : on_identify(); break;
		case ZIGBEE_NETWORK_REOPEN_BUTTON : on_network_reopen(); break;
	}
}

static void evt_handler(event_t const* e, void* p_context)
{
	switch (e->event)
	{
		case EVT_click 		: on_click(e); 			break;
		case EVT_long_click : on_long_click(e); 	break;
		default : break;
	}
}

REGISTER_EVT_HANDLER() = {
    .handler = evt_handler,
    .p_context = NULL,
};
