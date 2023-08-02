#include "tuya_utils.h"
#include "cJSON.h"
#include "tuya_cacert.h"
#include "tuya_log.h"
#include "tuya_error_code.h"
#include "system_interface.h"
#include "mqtt_client_interface.h"
#include "tuyalink_core.h"
#include <syslog.h>
#include "arg_parser.h"

static void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
	syslog(LOG_INFO, "Connected to Tuya.");
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
	syslog(LOG_INFO, "Disconnected from Tuya.");
}

static void on_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
	switch (msg->type) {
	case THING_TYPE_ACTION_EXECUTE: {
		cJSON *root = cJSON_Parse(msg->data_string);
		if (root != NULL) {
			transfer_data_from_cloud(context, msg, root);
		} else {
			syslog(LOG_ERR, "Failed: JSON parse was not successful");
		}
		break;
	}

	default:
		break;
	}
}

int tuya_init(tuya_mqtt_context_t *client, int ret, struct arguments arguments) {
    ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t){ 
        .host = "m1.tuyacn.com",
		.port = 8883,
		.cacert	= tuya_cacert_pem,
		.cacert_len	= sizeof(tuya_cacert_pem),
		.device_id = arguments.devId,
		.device_secret = arguments.devSec,
		.keepalive = 100,
		.timeout_ms = 2000,
		.on_connected = on_connected,
		.on_disconnect = on_disconnect,
		.on_messages = on_messages 
    });

	ret = tuya_mqtt_connect(client);
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "ERROR: Failed to connect to Tuya service. Error code: %d", ret);
		return 1;
	}
    return 0;
}

void send_available_memory(tuya_mqtt_context_t *context, int memory)
{
	const int megabyte = 1024 * 1024;

	char property_string[80];
	sprintf(property_string, "{\"free_ram\":{\"value\":%d}}", (memory / megabyte));
	tuyalink_thing_property_report_with_ack(context, NULL, property_string);
}

static void set_greeting_val(cJSON *greeting_value, tuya_mqtt_context_t *context){
	char set_greeting[80];
	sprintf(set_greeting, "{\"greeting\":{\"value\":\"%s\"}}", greeting_value->valuestring);
	tuyalink_thing_property_report_with_ack(context, NULL, set_greeting);
}

static void set_test_val(cJSON *test_value, tuya_mqtt_context_t *context) {
	bool test_bool = cJSON_IsTrue(test_value);
		if (test_bool) {
			tuyalink_thing_property_report_with_ack(context, NULL, "{\"daemon_test\":{\"value\":true}}");
		} else {
			tuyalink_thing_property_report_with_ack(context, NULL, "{\"daemon_test\":{\"value\":false}}");
		}

	return;
}

void transfer_data_from_cloud(tuya_mqtt_context_t *context, const tuyalink_message_t *msg, cJSON *root)
{
	cJSON *input_params = cJSON_GetObjectItem(root, "inputParams");

	if (input_params == NULL) {
		syslog(LOG_ERR, "Failed to get 'inputParams from JSON");
		return;
	}
	
	cJSON *greeting_value = cJSON_GetObjectItem(input_params, "greeting_identifier");
	if (greeting_value != NULL) {
		set_greeting_val(greeting_value, context);
	} else {
		syslog(LOG_ERR, "Failed to get greeting value from inputParams");
	}
	
	cJSON *test_value = cJSON_GetObjectItem(input_params, "test_bool_i");
	if (test_value != NULL) {
		set_test_val(test_value, context);
	} else {
		syslog(LOG_ERR, "Failed to get test bool value from inputParams");
	}

	return;
}