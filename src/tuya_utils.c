#include "tuya_utils.h"
#include "tuya_cacert.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "ubus_utils.h"

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

	if (!strcmp(actionCode->valuestring, "get_device_list_i")){
		send_devices_list(context);
	} 
	else if (!strcmp(actionCode->valuestring, "turn_on_i")) {
		turn_pin_on(inputParams, context);
	}
	else if (!strcmp(actionCode->valuestring, "turn_off_i")){
		turn_pin_off(inputParams, context);
	}
	else {
		syslog(LOG_ERR, "Failed. Unknown action code");
	}

	return;
}

int turn_pin_on(cJSON *inputParams, tuya_mqtt_context_t *context)
{	
	cJSON *port_on = cJSON_GetObjectItem(inputParams, "port_on");
	cJSON *pin_on = cJSON_GetObjectItem(inputParams, "pin_on");
	if (port_on == NULL && pin_on == NULL) {
			syslog(LOG_ERR, "Failed to get turn on values from inputParams");
			return -1;
	}

	char *port[20];
	int pin = pin_on->valueint;
	strncpy(port, port_on->valuestring, 15);

	uint32_t id;
	struct blob_buf buf = {0};
	blob_buf_init(&buf, 0);
	blobmsg_add_string(&buf, "port", port);
	blobmsg_add_u32(&buf, "pin", pin);

	if (ubus_lookup_id(ctx, "esp", &id) ||
		ubus_invoke(ctx, id, "on", buf.head, ubus_response_cb, NULL, 3000)) {
		char response[100];
		sprintf(response, "Failed to turn on port %s pin %d", port, pin);
		syslog(LOG_ERR, response);

		char status[100];
		sprintf(status, "{\"status\":{\"value\":\"%s\"}}", response);
		tuyalink_thing_property_report_with_ack(context, NULL, status);
	} else {
		char status[100];
		sprintf(status, "{\"status\":{\"value\":\"success\"}}");
		tuyalink_thing_property_report_with_ack(context, NULL, status);
	}


	blob_buf_free(&buf);

	return 0;
}

int turn_pin_off(cJSON *inputParams, tuya_mqtt_context_t *context)
{
	cJSON *port_off = cJSON_GetObjectItem(inputParams, "port_off");
	cJSON *pin_off = cJSON_GetObjectItem(inputParams, "pin_off");
	if (!(port_off != NULL && pin_off != NULL)) {
			syslog(LOG_ERR, "Failed to get turn off values from inputParams");
			return -1;
	}

	char *port[20];
	int pin = pin_off->valueint;
	strncpy(port, port_off->valuestring, 15);

	uint32_t id;
	struct blob_buf buf = {0};
	blob_buf_init(&buf, 0);
	blobmsg_add_string(&buf, "port", port);
	blobmsg_add_u32(&buf, "pin", pin);

	if (ubus_lookup_id(ctx, "esp", &id) ||
		ubus_invoke(ctx, id, "off", buf.head, ubus_response_cb, NULL, 3000)) {
		char response[100];
		sprintf(response, "Failed to turn off port %s pin %d", port, pin);
		syslog(LOG_ERR, response);

		char status[100];
		sprintf(status, "{\"status\":{\"value\":\"%s\"}}", response);
		tuyalink_thing_property_report_with_ack(context, NULL, status);
	} else {
		char status[100];
		sprintf(status, "{\"status\":{\"value\":\"success\"}}");
		tuyalink_thing_property_report_with_ack(context, NULL, status);
	}
	

	blob_buf_free(&buf);

	return 0;
}

int send_devices_list(tuya_mqtt_context_t *context)
{
	int rc = 0;
    uint32_t id;
	char devices_json_string[200] = "";

    if (ubus_lookup_id(ctx, "esp", &id) ||
        ubus_invoke(ctx, id, "devices", NULL, device_cb, devices_json_string, 3000)) {
        char response[100] = "Cannot request device list";
		syslog(LOG_ERR, response);
		char status[100];
		sprintf(status, "{\"status\":{\"value\":\"%s\"}}", response);
		tuyalink_thing_property_report_with_ack(context, NULL, status);
        rc = -1;
    } else {
		char jsonDataString[200] = "";
		if(get_device_json_string(devices_json_string, &jsonDataString, context)) {
			syslog(LOG_ERR, "Unable to get device json string");
			if (!strcmp(jsonDataString, "")){
				char status[100];
				sprintf(status, "{\"status\":{\"value\":\"No devices found\"}}");
				tuyalink_thing_property_report_with_ack(context, NULL, status);
			}
		} else {
			tuyalink_thing_property_report_with_ack(context, NULL, jsonDataString);
			char status[100];
			sprintf(status, "{\"status\":{\"value\":\"Success: device list sent\"}}");
			tuyalink_thing_property_report_with_ack(context, NULL, status);
		}
	}

    return rc;
}

int get_device_json_string(char *json_string, char *outputstring, tuya_mqtt_context_t *context)
{	
	cJSON *data_root = cJSON_Parse(json_string);
    if (data_root == NULL) {
		cJSON_Delete(data_root);
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 1;
    }

    cJSON *devices_array = cJSON_GetObjectItem(data_root, "devices");
    if (!cJSON_IsArray(devices_array)) {
        printf("'devices' is not an array.\n");
        cJSON_Delete(data_root);
        return 1;
    }

	int count = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON *devices = cJSON_CreateObject();
	cJSON *values = cJSON_CreateArray();

    cJSON *device = NULL;
    cJSON_ArrayForEach(device, devices_array) {
        cJSON *port = cJSON_GetObjectItem(device, "port");
        cJSON *vendor_id = cJSON_GetObjectItem(device, "vendor_id");
        cJSON *product_id = cJSON_GetObjectItem(device, "product_id");

        if (port && vendor_id && product_id) {
			char *device_info[50];
			sprintf(device_info, "\"Port: %s, VID: %s, PID: %s\", ", port->valuestring, vendor_id->valuestring, product_id->valuestring);
			cJSON_AddItemToArray(values, cJSON_CreateString(device_info));
			count++;
        }
    }

	if (count != 0) {
		cJSON_AddItemToObject(devices, "value", values);
		cJSON_AddItemToObject(root, "devices_array_i", devices);

		char *json_data = cJSON_Print(root);
		strncpy(outputstring, json_data, 200);
		free(json_data);
	} else {
		syslog(LOG_INFO, "No devices found");
	}
		
	cJSON_Delete(root);
    cJSON_Delete(data_root);
	
	return 0;
}