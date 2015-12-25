#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "c_stdlib.h"
#include "c_string.h"
#include "user_interface.h"

#include "ledmate_render.h"

int ICACHE_FLASH_ATTR ws2812_writegrb(char pin, const char *buffer, int length);

#define width 144
#define height 8
#define bpp 3

struct {
    uint8_t buf[width*height*bpp];
    int32_t buf_length;
    int32_t t;
} ledmate;

static int ICACHE_FLASH_ATTR lua_ledmate_push_msg(lua_State* L) {
    size_t length;
    const char *buffer = luaL_checklstring(L, 1, &length);

    c_printf("ledmate.push_msg: [%s]:%d\n", buffer, length);

    ledmate_push_msg(buffer, length);
}

static int ICACHE_FLASH_ATTR lua_ledmate_run(lua_State* L) {
    ledmate_render(ledmate.t);
    ledmate.t++;
    ws2812_writegrb(3, ledmate.buf, ledmate.buf_length);

    return 0;
}

static const LUA_REG_TYPE ledmate_map[] =
{
    { LSTRKEY( "push_msg" ), LFUNCVAL( lua_ledmate_push_msg )},
    { LSTRKEY( "run" ),      LFUNCVAL( lua_ledmate_run )},
    { LNILKEY, LNILVAL}
};

int luaopen_ledmate(lua_State *L) {
    ledmate.buf_length = width*height*bpp;
    ledmate.t = 0;
    ledmate_init(ledmate.buf, width, height);
    return 0;
}

NODEMCU_MODULE(LEDMATE, "ledmate", ledmate_map, luaopen_ledmate);
