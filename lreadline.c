/* BSD-license */

#include "lua.h"
#include "lauxlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>

#define MYNAME "lreadline"
#define MYVERSION MYNAME " library for " LUA_VERSION
#define MYTYPE "lreadline.lib"

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
/*** Adapted from Lua 5.2.0 */
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup+1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup+1));
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3));
    }
    lua_pop(L, nup);  /* remove upvalues */
}
#endif

static const luaL_Reg lreadlinelib[] = {
    {NULL, NULL}
};

LUALIB_API int luaopen_lreadline_core (lua_State *L) {

    luaL_newmetatable(L,MYTYPE);
    luaL_setfuncs(L,lreadlinelib,0);
    lua_pushliteral(L,"version");/** version */
    lua_pushliteral(L,MYVERSION);
    lua_settable(L,-3);
    lua_pushliteral(L,"__index");
    lua_pushvalue(L,-2);
    lua_settable(L,-3);

    return 1;
}

/*
  Local Variables:
  c-basic-offset:4
  c-file-style:"bsd"
  indent-tabs-mode:nil
  End:
*/
