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

int getResultValue(char *jsonStr, char *responseString) {
    cJSON *root = cJSON_Parse(jsonStr);

    if (root != NULL) {
        cJSON *result = cJSON_GetObjectItem(root, "result");

        if (cJSON_IsString(result)) {
			sprintf(responseString, "%s", result->valuestring);
            cJSON_Delete(root);
            return 0;
        } else {
            cJSON_Delete(root);
            return 1;
        }
    } else {
        return 1;
    }
}

int checkAction(const char *input, const char *searchString) {
    char *underscore = strchr(input, '_'); 

    if (underscore != NULL) {
        size_t substring_length = strlen(underscore + 1);

        char action[substring_length + 1];
        strncpy(action, underscore + 1, substring_length);
        action[substring_length] = '\0';

        if (strstr(action, searchString) != NULL) {
            return 0;
        } else {
            return 1;
        }
    }

	return 1;
}