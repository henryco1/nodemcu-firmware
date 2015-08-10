#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include "c_stdlib.h"
#include "c_string.h"
#include "driver/spi.h"

/**
 * Code is based on https://github.com/CHERTS/esp8266-devkit/blob/master/Espressif/examples/EspLightNode/user/ws2801.c
 * and provides a similar api as the ws2812 module.
 * The current implementation allows the caller to use
 * any combination of GPIO0, GPIO2, GPIO4, GPIO5 as clock and data.
 * Also supports HSPI (Hardware-SPI) which reduces CPU load a lot
 * and supports much higher bandwidth.
 */

u32 max7219_dout = 2; // gpio4
u32 max7219_cs = 3;   // gpio0
u32 max7219_clk = 4;  // gpio2

static void max7219_strip(u8 const *data, u32 len) {
    u32 dout_mask = 1 << pin_num[max7219_dout];
    u32 clk_mask  = 1 << pin_num[max7219_clk];
    u8 mask = 0x80;
    u8 const *end = data + len;
    
    while (true) {
        if (*data & mask) {
            GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, dout_mask); // Set pin high
            //c_printf("1");
        } else {
            GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, dout_mask); // Set pin low
            //c_printf("0");
        }


        GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, clk_mask); // Set pin high
        GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, clk_mask); // Set pin low

        if (!(mask >>= 1)) { // Next bit/byte
            data++;
            if (data >= end) {
                break;
            }
            mask = 0x80;
        }
    }
}

static void max7219_write_reg(u8 const reg, u8 const data) {
    u8 buf[2] = {reg, data};

    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << pin_num[max7219_cs]); // CS=low

    max7219_strip(buf, sizeof(buf));

    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << pin_num[max7219_cs]); // CS=high
}

static void enable_pin(u32 pin){
    platform_gpio_mode(pin, PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_FLOAT);
    platform_gpio_write(pin, 0);
}

static u32 max7219_init(lua_State* L) {
    c_printf("MAX7219 init: dout=%d, cs=%d, clk=%d\n",
        max7219_dout, max7219_cs, max7219_clk);

    enable_pin(max7219_dout);
    enable_pin(max7219_cs);
    enable_pin(max7219_clk);
}

static int max7219_init_lua(lua_State* L) {
    max7219_init(L);
    lua_pushinteger(L, 0);
    return 1;
}

static int ICACHE_FLASH_ATTR max7219_write_lua(lua_State* L) {
    size_t length;
    const char *buffer = luaL_checklstring(L, 1, &length);

    if (length != 2) {
        return luaL_error(L, "wrong arg range");
    }
    max7219_write_reg(buffer[0], buffer[1]);

    lua_pushinteger(L, length);

    return 1;
}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE max7219_map[] =
{
    { LSTRKEY("write"),    LFUNCVAL(max7219_write_lua)},
    { LSTRKEY("init"),     LFUNCVAL(max7219_init_lua)},
    { LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_max7219(lua_State *L) {
    LREGISTER(L, "max7219", max7219_map);
    return 1;
}

