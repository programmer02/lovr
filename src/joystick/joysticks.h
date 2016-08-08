#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int lovrJoysticksGetJoystickCount(lua_State* L);
int lovrJoysticksGetJoysticks(lua_State* L);

void lovrJoysticksOnJoystickChanged();
extern const luaL_Reg lovrJoysticks[];
int lovrInitJoysticks(lua_State* L);
