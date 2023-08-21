#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_utils.h"
#include <syslog.h>

int get_data_lua(lua_State *L)
{   
    //luaL_openlibs(L);
    syslog(LOG_INFO, "1");
    if (luaL_dofile(L, "/scripts/test.lua") != 0) {
        syslog(LOG_ERR, "Unable to load Lua script");
        return 1;
    }
    syslog(LOG_INFO, "2");
	lua_getglobal(L, "test_lua");
    syslog(LOG_INFO, "3");

    if (lua_pcall(L, 0, 1, 0) != 0) {
        //lua_close(L);
        syslog(LOG_INFO, "Cannot call lua");
        return 1;
    }
    syslog(LOG_INFO, "4");
    if (lua_isstring(L, -1)) {
        char *result[300];
        syslog(LOG_INFO, "5");
        strcpy(result, lua_tostring(L, -1));
        syslog(LOG_INFO, "Result from Lua get_data method: %s", result);
    }

    syslog(LOG_INFO, "1");

    return -1;
}