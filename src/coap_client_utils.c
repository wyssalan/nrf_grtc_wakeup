#include <zephyr/kernel.h>
#include <net/coap_utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket.h>
#include <openthread/thread.h>
#include <openthread/nat64.h>
#include <stdio.h>

#include "coap_client_utils.h"
// NVS Non Volatile Storage
#include "storage_nvs.h"

#define RESPONSE_POLL_PERIOD 100
#define COAP_CLIENT_WORKQ_STACK_SIZE 2048
#define COAP_CLIENT_WORKQ_PRIORITY 5

#define COAP_PORT 5683

LOG_MODULE_REGISTER(coap_client_utils);
K_THREAD_STACK_DEFINE(coap_client_workq_stack_area, COAP_CLIENT_WORKQ_STACK_SIZE);

// static bool is_connected;
// static struct k_work_q coap_client_workq;
// static struct k_work send_work;
// static struct k_work toggle_MTD_SED_work;
// static struct k_work on_connect_work;
// static struct k_work on_disconnect_work;
static struct sockaddr_in6 rpi_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(COAP_PORT),
	.sin6_scope_id = 0U};
/* Options supported by the server */
static const char *const sensor_option[] = {"sensors", NULL};
mtd_mode_toggle_cb_t on_mtd_mode_toggle;

static float temp, hum;
static uint16_t co2;

void set_rpi_ipv6_from_ipv4(uint32_t ipv4_address)
{
	otError error;
	struct otIp4Address rpi_ip4_addr = {
		.mFields.m32 = ipv4_address};
	struct otIp6Address rpi_ip6_addr = {};

	// Hier nehmen wir an, dass der NAT64-Mechanismus für den aktuellen OpenThread-Stack aktiv ist
	error = otNat64SynthesizeIp6Address(openthread_get_default_instance(), &rpi_ip4_addr, &rpi_ip6_addr);
	memcpy(rpi_addr.sin6_addr.s6_addr, &rpi_ip6_addr.mFields, sizeof(rpi_addr.sin6_addr.s6_addr));

	if (error != OT_ERROR_NONE)
	{
		LOG_ERR("Failed to synthesize IPv6 address for IPv4");
	}
	else
	{
		LOG_INF("IPv6 address synthesized successful");
	}
	// storage_nvs_write((uint16_t)2, rpi_addr.sin6_addr.s6_addr, sizeof(rpi_addr.sin6_addr.s6_addr));
	return;
}

// static bool is_mtd_in_med_mode(otInstance *instance)
// {
// 	return otThreadGetLinkMode(instance).mRxOnWhenIdle;
// }

// static coap_reply_t rpi_response(const struct coap_packet *response, struct coap_reply *reply, const struct sockaddr *from)
// {
//     if (response == NULL) {
//         LOG_ERR("CoAP response is NULL");
//         return 1;
//     }

//     uint8_t *payload;
//     uint16_t payload_size = 0u;

//     payload = coap_packet_get_payload(response, &payload_size);

//     if (payload != NULL) {
//         LOG_INF("Received response payload: %.*s", payload_size, payload);
//     }

//     return 0;
// }

static void send_measured_data()
{
	char json_payload[128];
	snprintf(json_payload, sizeof(json_payload),
			 "{\"temp\":%u,\"hum\":%u,\"co2\":%u}",
			 (unsigned int)temp, (unsigned int)hum, (unsigned int)co2);

	// ARG_UNUSED(item);
	// if (storage_nvs_read((uint16_t)2, rpi_addr.sin6_addr.s6_addr, sizeof(rpi_addr.sin6_addr.s6_addr)) <= 0)
	// {
		// uint32_t rpi_ipv4 = 0xA0552485;  //ACHTUNG LITTLE ENDIAN UND BIG ENDIAN VERKEHRT
		uint32_t rpi_ipv4 = 0x852455A0;
		set_rpi_ipv6_from_ipv4(rpi_ipv4); // Synthetisiere die IPv6-Adresse
	// }

	LOG_INF("IPv6: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			rpi_addr.sin6_addr.s6_addr[0], rpi_addr.sin6_addr.s6_addr[1],
			rpi_addr.sin6_addr.s6_addr[2], rpi_addr.sin6_addr.s6_addr[3],
			rpi_addr.sin6_addr.s6_addr[4], rpi_addr.sin6_addr.s6_addr[5],
			rpi_addr.sin6_addr.s6_addr[6], rpi_addr.sin6_addr.s6_addr[7],
			rpi_addr.sin6_addr.s6_addr[8], rpi_addr.sin6_addr.s6_addr[9],
			rpi_addr.sin6_addr.s6_addr[10], rpi_addr.sin6_addr.s6_addr[11],
			rpi_addr.sin6_addr.s6_addr[12], rpi_addr.sin6_addr.s6_addr[13],
			rpi_addr.sin6_addr.s6_addr[14], rpi_addr.sin6_addr.s6_addr[15]);

	// coap_send_request(COAP_METHOD_POST, (const struct sockaddr *)&rpi_addr, sensor_option, &payload, sizeof(payload), NULL);
	coap_send_request(COAP_METHOD_POST, (const struct sockaddr *)&rpi_addr, sensor_option, (uint8_t *)json_payload, strlen(json_payload), NULL);
	LOG_INF("COAP-Message sent");
	return;
}

static void toggle_minimal_sleepy_end_device(void)
{
	otError error;
	otLinkModeConfig mode;
	struct openthread_context *context = openthread_get_default_context();

	// __ASSERT_NO_MSG(context != NULL);

	openthread_api_mutex_lock(context);
	mode = otThreadGetLinkMode(context->instance);
	mode.mRxOnWhenIdle = !mode.mRxOnWhenIdle;
	error = otThreadSetLinkMode(context->instance, mode);
	openthread_api_mutex_unlock(context);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set MLE link mode configuration");
	} else {
		on_mtd_mode_toggle(mode.mRxOnWhenIdle);
	}
}

// static void update_device_state(void)
// {
// 	struct otInstance *instance = openthread_get_default_instance();
// 	otLinkModeConfig mode = otThreadGetLinkMode(instance);
// 	on_mtd_mode_toggle(mode.mRxOnWhenIdle);
// }

// static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
// 									void *user_data)
// {
// 	if (flags & OT_CHANGED_THREAD_ROLE)
// 	{
// 		switch (otThreadGetDeviceRole(ot_context->instance))
// 		{
// 		case OT_DEVICE_ROLE_CHILD:
// 		case OT_DEVICE_ROLE_ROUTER:
// 		case OT_DEVICE_ROLE_LEADER:
// 			k_work_submit_to_queue(&coap_client_workq, &on_connect_work);
// 			is_connected = true;
// 			break;

// 		case OT_DEVICE_ROLE_DISABLED:
// 		case OT_DEVICE_ROLE_DETACHED:
// 		default:
// 			k_work_submit_to_queue(&coap_client_workq, &on_disconnect_work);
// 			is_connected = false;
// 			break;
// 		}
// 	}
// }
// static struct openthread_state_changed_cb ot_state_chaged_cb = {
// 	.state_changed_cb = on_thread_state_changed};

// static void submit_work_if_connected(struct k_work *work)
// {
// 	if (is_connected)
// 	{
// 		k_work_submit_to_queue(&coap_client_workq, work);
// 	}
// 	else
// 	{
// 		LOG_INF("Connection is broken");
// 	}
// }

void coap_client_utils_init()
{

	coap_init(AF_INET6, NULL);

	// if (storage_nvs_read((uint16_t)1, unique_local_addr.sin6_addr.s6_addr, sizeof(unique_local_addr.sin6_addr.s6_addr)) > 0)
	// {
	// 	LOG_INF("Stored IP has been loaded");
	// }

	// k_work_queue_init(&coap_client_workq);

	// k_work_queue_start(&coap_client_workq, coap_client_workq_stack_area,
	//    K_THREAD_STACK_SIZEOF(coap_client_workq_stack_area),
	//    COAP_CLIENT_WORKQ_PRIORITY, NULL);

	// k_work_init(&on_connect_work, on_connect);
	// k_work_init(&on_disconnect_work, on_disconnect);
	// k_work_init(&send_work, send_measured_data);

	// openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
	openthread_start(openthread_get_default_context());

	// if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
	// 	k_work_init(&toggle_MTD_SED_work,
	// 		    toggle_minimal_sleepy_end_device);
	// 	update_device_state();
	// }
	return;
}

void coap_client_send_measured_data(float *rec_co2, float *rec_temp, float *rec_hum)
{
	co2 = *rec_co2;
	temp = *rec_temp;
	hum = *rec_hum;
	// submit_work_if_connected(&send_work);
	send_measured_data();
	// toggle_minimal_sleepy_end_device();
	LOG_INF("Return to main Program");
	return;
}

// void coap_client_toggle_minimal_sleepy_end_device(void)
// {
// 	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
// 		k_work_submit_to_queue(&coap_client_workq, &toggle_MTD_SED_work);
// 	}
// }
