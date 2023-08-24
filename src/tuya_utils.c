#include "tuya_utils.h"
#include "tuya_cacert.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "lua_utils.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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
			transfer_data(context, msg, root);
		} else {
			syslog(LOG_ERR, "Failed: JSON parse was not successful");
		}

		cJSON_Delete(root);
		break;
	}

	default:
		break;
	}	
}

int tuya_init(tuya_mqtt_context_t *client, int *ret, struct arguments arguments) {
    *ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t){ 
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
	if (*ret != OPRT_OK) {
		syslog(LOG_ERR, "Tuya MQTT Initialization Error: %d", ret);
		return -1;
	}

	*ret = tuya_mqtt_connect(client);
	if (*ret != OPRT_OK) {
		syslog(LOG_ERR, "ERROR: Failed to connect to Tuya service. Error code: %d", ret);
		return 1;
	}

    return 0;
}

int send_lua_data(tuya_mqtt_context_t *context, const char *data)
{
	tuyalink_thing_property_report_with_ack(context, NULL, data);
	return 0;
}

void transfer_data(tuya_mqtt_context_t *context, const tuyalink_message_t *msg, cJSON *root)
{
	cJSON *actionCode = cJSON_GetObjectItem(root, "actionCode");

	if (actionCode == NULL) {
		syslog(LOG_ERR, "Failed to get 'actionCode' from JSON");
		return;
	}
	cJSON *inputParams = cJSON_GetObjectItem(root, "inputParams");
	if (inputParams == NULL ){
		syslog(LOG_ERR, "Failed to get 'inputParams' from JSON");
		return;
	}

	if (lua_handle_params(actionCode->valuestring, inputParams, context) != 0) {
			syslog(LOG_ERR, "Unable to call lua_handle_params");	
	}

	return;
}