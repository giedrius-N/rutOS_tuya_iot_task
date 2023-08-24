#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_utils.h"
#include <syslog.h>
#include <dirent.h>
#include <string.h>
#include "helpers.h"

int load_lua_files(lua_State *L[], const char *scriptDirectory, int *count) {
    DIR *dir;
    int i = 0;
    if ((dir = opendir(scriptDirectory)) != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL && i != MAX_LUA_SCRIPTS) {
            if (ent->d_type == DT_REG && strstr(ent->d_name, ".lua") != NULL) {
                lua_State *new_L = luaL_newstate();
                luaL_openlibs(new_L);

                char script_path[1024];
                snprintf(script_path, sizeof(script_path), "%s%s", scriptDirectory, ent->d_name);
                
                if (luaL_loadfile(new_L, script_path) || lua_pcall(new_L, 0, 0, 0)) {
                    syslog(LOG_ERR, "Error loading Lua file: %s", lua_tostring(new_L, -1));
                    lua_pop(new_L, 1);
                }

                lua_getglobal(new_L, "get_data");

                if (!lua_isfunction(new_L, -1)) {
                    syslog(LOG_ERR, "Function 'get_data' does not exist in %s Lua script.", script_path);
                    lua_close(new_L);
                    continue;
                }

                L[i++] = new_L;
            }
        }
        closedir(dir);
    } else {
        syslog(LOG_ERR, "Unable to open directory");
    }

    *count = i;

    return 0;
}

int execute_lua(lua_State *Lstates[], int count, tuya_mqtt_context_t *context)
{
    for (int i = 0; i < count; i++) {
		lua_getglobal(Lstates[i], "get_data");
        if (lua_pcall(Lstates[i], 0, 1, 0) != 0) {
            continue;
        }
        if (lua_isstring(Lstates[i], -1)) {
            char result[150];
            strcpy(result, lua_tostring(Lstates[i], -1));
            
            if(!isJsonValid(result)) {
                continue;
            }

            send_lua_data(context, result);
            syslog(LOG_INFO, "Result from Lua get_data method: %s", result);
        }
	}

    return 0;
}

int init_lua(lua_State *Lstates[], int count)
{
    for (int i = 0; i < count; i++) {
		lua_getglobal(Lstates[i], "init");

        if (!lua_isfunction(Lstates[i], -1)) {
            continue;
        }

		if (lua_pcall(Lstates[i], 0, 0, 0) != 0) {
            syslog(LOG_ERR, "Unable to call init function");
		}
	}   
    
    return 0;
}

int deinit_lua(lua_State *Lstates[], int count)
{
    for (int i = 0; i < count; i++) {
		lua_getglobal(Lstates[i], "deinit");

        if (!lua_isfunction(Lstates[i], -1)) {
            continue;
        }

		if (lua_pcall(Lstates[i], 0, 0, 0) != 0) {
            syslog(LOG_ERR, "Unable to call deinit function");
		}
	}

    return 0;
}

int lua_free(lua_State *Lstates[], int count)
{  
    for (int i = 0; i < count; i++) {
		lua_close(Lstates[i]);
	}

    return 0;
}

int lua_handle_params(char *script_name, cJSON *inputParams, tuya_mqtt_context_t *context)
{
    lua_State *L = luaL_newstate();
	luaL_openlibs(L);

    char script_path[150];
    sprintf(script_path, "/usr/lua_modules/actions/%s.lua", script_name);

	int rc = luaL_dofile(L, script_path);

    if (rc != 0) {
		syslog(LOG_INFO, "Error running script: %s, unable to find %s", lua_tostring(L, -1), script_path);
		lua_close(L);
        return 1;
	}

    lua_getglobal(L, "init");

    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != 0) {
            syslog(LOG_ERR, "Unable to call init function");
		}
    }

    lua_newtable(L);

    int i = 0;
    if (inputParams != NULL && cJSON_IsObject(inputParams)) {
		cJSON *item = NULL;
			
		cJSON_ArrayForEach(item, inputParams) {

			const char *key = item->string; 

			cJSON *value = cJSON_GetObjectItem(inputParams, key);
			if (value != NULL) {
				if (cJSON_IsString(value)) {
                    lua_pushstring(L, value->valuestring);
                    lua_setfield(L, -2, key);
				} else if (cJSON_IsNumber(value)) {
                    lua_pushnumber(L, value->valueint);
                    lua_setfield(L, -2, key);
				}  
                i++;
			}

		}
	}

    if (checkAction(script_name, "execute") == 0) {
        if (lua_execute(L, context) != 0) {
            goto deinit;
        }
    } else if (checkAction(script_name, "sendData") == 0) {
        if (lua_send_data(L, context) != 0) {
            goto deinit;
        }
    } else {
        syslog(LOG_INFO, "Unable to find `execute` or `sendData`");
    }

    

    deinit:
        lua_getglobal(L, "deinit");

        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                syslog(LOG_ERR, "Unable to call deinit function");
            }
        }

    lua_close(L);

    return 0;
}

int lua_execute(lua_State *L, tuya_mqtt_context_t *context)
{
    lua_getglobal(L, "execute");
    if (!lua_isfunction(L, -1)) {
        return 1;
    }
	lua_pushvalue(L, -2);

    

	if (lua_pcall(L, 1, 1, 0) != 0) {
		syslog(LOG_INFO, "Error calling Lua function: %s", lua_tostring(L, -1));
		return 1;
	}

	if (lua_isstring(L, -1)) {
		const char *response = lua_tostring(L, -1);
        char *responseString[100];
        getResultValue(response, responseString);
        char status[200];
		sprintf(status, "{\"status\":{\"value\":\"%s\"}}", responseString);
		tuyalink_thing_property_report_with_ack(context, NULL, status);
	}

    return 0;
}

int lua_send_data(lua_State *L, tuya_mqtt_context_t *context)
{
    lua_getglobal(L, "get_data");

    if (!lua_isfunction(L, -1)) {
        return 1;
    }

    if (lua_pcall(L, 0, 1, 0) != 0) {
        lua_close(L);
        return -1;
    }
    if (lua_isstring(L, -1)) {
        char result[150];
        strcpy(result, lua_tostring(L, -1));
            
        if(!isJsonValid(result)) {
            syslog(LOG_INFO, "Result from Lua get_data method: %s", result);
            lua_close(L);
            return 0;
        }

        send_lua_data(context, result);
        syslog(LOG_INFO, "Result from Lua get_data method: %s", result);
    }

    return 0;
}