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

static void max7219_spi_write32(u8 spi_no, u32 data) {
    
    while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);    // Correct 

    // Set SPI to use 32 bits
    WRITE_PERI_REG(
        SPI_USER1(spi_no),
        ((31&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S) |
        ((31&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S));

    // Is this if-case necessary?
    // if(READ_PERI_REG(SPI_USER(spi_no))&SPI_WR_BYTE_ORDER)
    // Copy data to W0
    WRITE_PERI_REG(SPI_W0(spi_no), data);

    // Send
    SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
}

static void max7219_strip_hspi(u8 const * data, u16 len) {
    // TODO: write it a bit nicer
    while (len>3) {
        u32 data32 = *((u32 const *) data);
        max7219_spi_write32(HSPI, data32);
        len -= 4;
        data += 4;
    }
    while (len--) {
        platform_spi_send_recv(HSPI, *(data++));
    }
}

static u32 max7219_init(lua_State* L) {
    // TODO: clock seems to be broken in app/driver/spi.c
    return platform_spi_setup(1, PLATFORM_SPI_MASTER, PLATFORM_SPI_CPOL_HIGH,
              PLATFORM_SPI_CPHA_HIGH, PLATFORM_SPI_DATABITS_8, 0);
}

static int max7219_init_lua(lua_State* L) {
    max7219_init(L);
    lua_pushinteger(L, 0);
    return 1;
}

static int ICACHE_FLASH_ATTR max7219_write(lua_State* L) {
    size_t length;
    const char *buffer = luaL_checklstring(L, 1, &length);

    max7219_strip_hspi(buffer, length);

    lua_pushinteger(L, length);

    return 1;
}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE max7219_map[] =
{
    { LSTRKEY("write"),    LFUNCVAL(max7219_write)},
    { LSTRKEY("init"),     LFUNCVAL(max7219_init_lua)},
    { LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_max7219(lua_State *L) {
    LREGISTER(L, "max7219", max7219_map);
    return 1;
}

