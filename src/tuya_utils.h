#ifndef TUYA_UTILS_H
#define TUYA_UTILS_H

#include <cJSON.h>
#include <syslog.h>
#include <tuya_error_code.h>
#include <system_interface.h>
#include <mqtt_client_interface.h>
#include <tuyalink_core.h>
#include "arg_struct.h"

int tuya_init(tuya_mqtt_context_t *client, int *ret, struct arguments arguments);

void send_available_memory(tuya_mqtt_context_t *context, int memory);

void transfer_data_from_cloud(tuya_mqtt_context_t *context, const tuyalink_message_t *msg, cJSON *root);

int send_lua_data(tuya_mqtt_context_t *context, const char *data);

#endif
