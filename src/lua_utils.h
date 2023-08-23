#ifndef LUA_UTILS_H
#define LUA_UTILS_H
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "tuya_utils.h"

int load_lua_files(lua_State *L[], const char *scriptDirectory, int *count);
int execute_lua(lua_State *Lstates[], int count, tuya_mqtt_context_t *context);
int init_lua(lua_State *Lstates[], int count);
int deinit_lua(lua_State *Lstates[], int count);
int lua_free(lua_State *Lstates[], int count);

#endif
