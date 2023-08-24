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
#include "helpers.h"
#include "lua_utils.h"

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

	int scripts_count = 0;
	lua_State *Lstates[MAX_LUA_SCRIPTS];

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

	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);

	struct arguments arguments;
	arguments.daemonize = false;

	if (argp_parse(&argp, argc, argv, 0, NULL, &arguments) != 0) {
		syslog(LOG_ERR, "ERROR: Failed to parse command-line arguments.");

		goto closing_log;
	}

	if (arguments.daemonize) {
		if(daemonize()){
			goto closing_log;
		}
	}

	client = &client_instance;

	if (tuya_init(client, &ret, arguments)) {
		if (ret == -1) {
			goto closing_log;
		}
		goto tuya_cleanup;
	}

	load_lua_files(Lstates, "/usr/lua_modules/", &scripts_count);
	init_lua(Lstates, scripts_count);

	while (g_signal_flag) {
		tuya_mqtt_loop(client);
		execute_lua(Lstates, scripts_count, client);

		sleep(2);
	}

	deinit_lua(Lstates, scripts_count);
	lua_free(Lstates, scripts_count);

	tuya_mqtt_disconnect(client);
	tuya_cleanup:
		ret = tuya_mqtt_deinit(client);
	closing_log:
		closelog();

	return ret;
}