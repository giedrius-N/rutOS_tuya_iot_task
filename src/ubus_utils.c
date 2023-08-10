#include "ubus_utils.h"
#include <syslog.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <cJSON.h>

void device_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
    char *devices_json_string = (char *)req->priv;
    
    strcpy(devices_json_string, blobmsg_format_json(msg, true));
}

void ubus_response_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
    if (msg) {
        char *response = blobmsg_format_json(msg, true);
        syslog(LOG_INFO, "UBUS response: %s", response);
        free(response);
    } else {
        syslog(LOG_INFO, "UBUS request failed");
    }
}