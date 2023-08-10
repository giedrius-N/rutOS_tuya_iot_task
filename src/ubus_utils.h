#ifndef UBUS_UTILS_H
#define UBUS_UTILS_H

#include <libubox/blobmsg_json.h>
#include <libubus.h>

struct ubus_context *ctx;

struct Device {
    char port[20];
    char vendor_id[5];
    char product_id[5];
};

static const struct blobmsg_policy device_policy[] = {
    [0] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
    [1] = { .name = "vendor_id", .type = BLOBMSG_TYPE_STRING },
    [2] = { .name = "product_id", .type = BLOBMSG_TYPE_STRING },
};

void device_cb(struct ubus_request *req, int type, struct blob_attr *msg);
void ubus_response_cb(struct ubus_request *req, int type, struct blob_attr *msg);

#endif