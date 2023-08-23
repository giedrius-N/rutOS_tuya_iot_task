#include "helpers.h"
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cJSON.h>

int daemonize()
{
	pid_t pid, sid;

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "ERROR: Forking process failed. Unable to create child process.");
		return 1;
	}
	if (pid > 0) {
		return 1;
	}

	umask(0);

	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR, "ERROR: Session creation failed. Unable to create new session.");
		return 1;
	}
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	return 0;
}

int isJsonValid(const char* jsonString) {
    cJSON* json = cJSON_Parse(jsonString);
    if (json == NULL) {
		syslog(LOG_ERR, "Failed to parse JSON. Invalid JSON data: %s", jsonString);
		cJSON_Delete(json);
        return 0;
    }
    cJSON_Delete(json);
    return 1;
}