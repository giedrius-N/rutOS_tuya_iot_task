#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "tuya_log.h"
#include <syslog.h>
#include "arg_parser.h"
#include "ubus_invoke.h"
#include "tuya_utils.h"
#include "helpers.h"

volatile sig_atomic_t g_signal_flag = 1;
void sig_handler()
{
	g_signal_flag = 0;
}

int main(int argc, char **argv)
{
	openlog("Tuya IoT", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);

	struct arguments arguments;

	if (argp_parse(&argp, argc, argv, 0, NULL, &arguments) != 0) {
		syslog(LOG_ERR, "ERROR: Failed to parse command-line arguments.");
		goto cleanup;
	}

	struct ubus_context *ctx;
	uint32_t id;

	struct MemData memory = { 0 };

	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		goto cleanup;
	}

	if (arguments.daemonize) {
		if(daemonize()){
			goto cleanup;
		}
	}

	tuya_mqtt_context_t client_instance;
	tuya_mqtt_context_t *client;
	int ret = OPRT_OK;

	

	client = &client_instance;

	if (tuya_init(client, ret, arguments)) {
		goto cleanup;
	}

	while (g_signal_flag) {
		tuya_mqtt_loop(client);
		if (ubus_lookup_id(ctx, "system", &id) ||
		    ubus_invoke(ctx, id, "info", NULL, board_cb, &memory, 3000)) {
			syslog(LOG_ERR, "Cannot request memory info from ubus");
			continue;
		}
		send_available_memory(client, memory.free);

		sleep(2);
	}

	tuya_mqtt_disconnect(client);
	ret = tuya_mqtt_deinit(client);
	cleanup:
		closelog();
		ubus_free(ctx);

	return ret;
}