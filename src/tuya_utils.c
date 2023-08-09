#include "tuya_utils.h"
#include "tuya_cacert.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "ubus_invoke.h"

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

void transfer_data_from_cloud(tuya_mqtt_context_t *context, const tuyalink_message_t *msg, cJSON *root)
{
	cJSON *input_params = cJSON_GetObjectItem(root, "inputParams");

	if (input_params == NULL) {
		syslog(LOG_ERR, "Failed to get 'inputParams from JSON");
		return;
	}

	cJSON *port_on = cJSON_GetObjectItem(input_params, "port_on");
	cJSON *pin_on = cJSON_GetObjectItem(input_params, "pin_on");
	if (port_on != NULL && pin_on != NULL) {
		if (turn_pin_on(port_on, pin_on)){
			syslog(LOG_ERR, "Failed to turn on pin");
		}
	} else {
		syslog(LOG_ERR, "Failed to get turn on values from inputParams");
	}

	cJSON *port_off = cJSON_GetObjectItem(input_params, "port_off");
	cJSON *pin_off = cJSON_GetObjectItem(input_params, "pin_off");
	if (port_off != NULL && pin_off != NULL) {
		if (turn_pin_off(port_off, pin_off)){
			syslog(LOG_ERR, "Failed to turn off pin");
		}
	} else {
		syslog(LOG_ERR, "Failed to get turn off values from inputParams");
	}

	cJSON *get_devices_list = cJSON_GetObjectItem(input_params, "get_dev_list_i");
	if (get_devices_list != NULL) {
		send_devices_list(context, get_devices_list);
	} else {
		syslog(LOG_ERR, "Unable to get devices list identifier");
	}

	return;
}

int turn_pin_on(cJSON *port_on, cJSON *pin_on)
{
	char *port[20];
	int pin = pin_on->valueint;
	strncpy(port, port_on->valuestring, 15);

	struct ubus_context *ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to UBUS");
		return -1;
	}

	uint32_t id;
	struct blob_buf buf = {0};
    blob_buf_init(&buf, 0);
    blobmsg_add_string(&buf, "port", port);
    blobmsg_add_u32(&buf, "pin", pin);

	struct ubus_object_data obj;
	if (ubus_lookup_id(ctx, "esp", &id) ||
		ubus_invoke(ctx, id, "on", buf.head, ubus_response_cb, NULL, 3000)) {
		syslog(LOG_ERR, "Failed to turn on port %s pin %d", port, pin);
	}

	blob_buf_free(&buf);
	ubus_free(ctx);

	return 0;
}

int turn_pin_off(cJSON *port_off, cJSON *pin_off)
{
	char *port[20];
	int pin = pin_off->valueint;
	strncpy(port, port_off->valuestring, 15);

	struct ubus_context *ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to UBUS");
		return -1;
	}

	uint32_t id;
	struct blob_buf buf = {0};
    blob_buf_init(&buf, 0);
    blobmsg_add_string(&buf, "port", port);
    blobmsg_add_u32(&buf, "pin", pin);

	struct ubus_object_data obj;
	if (ubus_lookup_id(ctx, "esp", &id) ||
		ubus_invoke(ctx, id, "off", buf.head, ubus_response_cb, NULL, 3000)) {
		syslog(LOG_ERR, "Failed to turn on port %s pin %d", port, pin);
	}

	blob_buf_free(&buf);
	ubus_free(ctx);

	return 0;
}

int send_devices_list(tuya_mqtt_context_t *context, cJSON *get_dev_list_value)
{

	bool get_dev = cJSON_IsTrue(get_dev_list_value);
	if (!get_dev) {
		syslog(LOG_INFO, "Getting devices list is not enabled");
		char send_list[80];
		sprintf(send_list, "{\"devices_array_i\":{\"value\":%s}}", "[]");
		tuyalink_thing_property_report_with_ack(context, NULL, send_list);
		syslog(LOG_INFO, "%s", send_list);
		return -1;
	}

	int rc = 0;
	struct ubus_context *ctx;
    uint32_t id;

    struct Device device_list[10];

    ctx = ubus_connect(NULL);
    if (!ctx) {
        syslog(LOG_ERR, "Failed to connect to ubus");
        return -1;
    }

    if (ubus_lookup_id(ctx, "esp", &id) ||
        ubus_invoke(ctx, id, "devices", NULL, device_cb, device_list, 3000)) {
        syslog(LOG_ERR, "Cannot request device list");
        rc = -1;
    } else {
		int i = 0;
		char devices[150] = "";
		while (strcmp(device_list[i].port, "-1")) {
			char *device[50];
			sprintf(device, "\"Port: %s, VID: %s, PID: %s\", ", device_list[i].port, device_list[i].vendor_id, device_list[i].product_id);
			strcat(devices, device);
			i++;
		}
		char *data[200];
		sprintf(data, "{\"devices_array_i\":{\"value\":[%s]}}", devices);
		tuyalink_thing_property_report_with_ack(context, NULL, data);
	}

    ubus_free(ctx);

    return rc;
}