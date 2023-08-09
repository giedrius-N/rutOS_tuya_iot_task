#include "ubus_invoke.h"
#include <syslog.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <cJSON.h>

void device_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
    struct Device *device_list = (struct Device *)req->priv;
    struct blob_attr *tb;
    int rem;
    int i = 0;

    blobmsg_for_each_attr(tb, blobmsg_data(msg), rem) {
        struct blob_attr *dev_attrs[3];
        blobmsg_parse(device_policy, sizeof(device_policy) / sizeof(device_policy[0]), dev_attrs,
                      blobmsg_data(tb), blobmsg_data_len(tb));

        if (!dev_attrs[0] || !dev_attrs[1] || !dev_attrs[2]) {
            syslog(LOG_ERR, "Incomplete device information received");
            continue;
        }

        strncpy(device_list[i].port, blobmsg_get_string(dev_attrs[0]), sizeof(device_list[i].port));
        strncpy(device_list[i].vendor_id, blobmsg_get_string(dev_attrs[1]), sizeof(device_list[i].vendor_id));
        strncpy(device_list[i].product_id, blobmsg_get_string(dev_attrs[2]), sizeof(device_list[i].product_id));

        i++;
    }
	strcpy(device_list[i].port, "-1");
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