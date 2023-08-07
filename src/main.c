#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <argp.h>
#include "arg_parser.h"
#include <stdbool.h>
#include "arg_struct.h"

#include "tuya_utils.h"
#include "ubus_invoke.h"
#include "helpers.h"

volatile sig_atomic_t g_signal_flag = 1;
void sig_handler()
{
	g_signal_flag = 0;
}

int main(int argc, char **argv)
{
	int ret = OPRT_OK;
	tuya_mqtt_context_t client_instance;
	tuya_mqtt_context_t *client;

	struct argp_option options[] = {
		{"prodID", 'p', "STRING", 0, "DeviceID", 0},
		{"devId", 'd', "STRING", 0, "DeviceSecret", 0},
		{"devSec", 's', "STRING", 0, "ProductID", 0},
		{"daemon", 'D', 0, 0, "Daemonize the process", 0},
		{0}
	};

	char args_doc[] = "DeviceID DeviceSecret ProductID";

	struct argp argp = {options, parse_opt, args_doc, NULL};

	openlog("Tuya IoT", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
	struct ubus_context *ctx;
	uint32_t id;

	struct MemData memory = { 0 };
	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		goto closing_log;
	}

	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);

	struct arguments arguments;

	if (argp_parse(&argp, argc, argv, 0, NULL, &arguments) != 0) {
		syslog(LOG_ERR, "ERROR: Failed to parse command-line arguments.");

		goto ubus_cleanup;
	}

	if (arguments.daemonize) {
		if(daemonize()){
			goto ubus_cleanup;
		}
	}

	client = &client_instance;

	if (tuya_init(client, &ret, arguments)) {
		if (ret == -1) {
			goto ubus_cleanup;
		}
		goto tuya_cleanup;
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
	tuya_cleanup:
		ret = tuya_mqtt_deinit(client);
	ubus_cleanup:
		ubus_free(ctx);
	closing_log:
		closelog();
	
	return ret;
}