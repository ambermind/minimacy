// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "hardware/address_mapped.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "hardware/regs/addressmap.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/sync.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "pico/bootrom.h"

#include <stdio.h>

#include "../../src/minimacy.h"
#include "raspberry-2350.h"

#include "raspberry-2350-pio.pio.h"

const char* const Argv[]={"minimacy",RP2350_BOOT};
char FlashUniqueId[8];
unsigned int FlashSize;
#define UART_CONSOLE uart0

void uartInit()
{
	gpio_set_function(GPIO_UART_TX, UART_FUNCSEL_NUM(UART_CONSOLE, GPIO_UART_TX));
	gpio_set_function(GPIO_UART_RX, UART_FUNCSEL_NUM(UART_CONSOLE, GPIO_UART_RX));
	uart_init(UART_CONSOLE,115200);

#ifdef GPIO_UART1_TX
	gpio_set_function(GPIO_UART1_TX, UART_FUNCSEL_NUM(uart1, GPIO_UART1_TX));
	gpio_set_function(GPIO_UART1_RX, UART_FUNCSEL_NUM(uart1, GPIO_UART1_RX));
	uart_init(uart1,9600);
#endif
}

int uartWritable() {
	return 1;
}
void uartPutChar(unsigned int c) {
	if (c==10) {
		uart_tx_wait_blocking(UART_CONSOLE);
		uart_putc_raw(UART_CONSOLE,13);
	}
	uart_tx_wait_blocking(UART_CONSOLE);
	uart_putc_raw(UART_CONSOLE,c);
}

int uartReadable()
{
	return uart_is_readable(UART_CONSOLE);
}
int uartGet()
{
	if (!uart_is_readable(UART_CONSOLE)) return -1;
	return (int)255&uart_getc(UART_CONSOLE);
}

//----------------------
void uartPut(char *s,int len) {
	while(len--) uartPutChar(*s++);
}

void consoleWrite(int user, char* src, int len)
{
	uartPut(src,len);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
	vsnprintf(NULL,0, format, arglist);
}


LINT hwTimeMs()
{
	return to_ms_since_boot(get_absolute_time());
}
void hwTimeInit()
{
}
LINT hwTime()
{
	return hwTimeMs()/1000;
}


void hwRandomInit(void)
{
}

void hwRandomBytes(char* dst, LINT len)
{
	while(len>0)
	{
		int i;
		unsigned int val;
		val=get_rand_32();
		for(i=0;i<4;i++) {
			*(dst++)=(char)val;
			val>>=8;
			len--;
			if (!len) return;
		}
	}
}
int hwHasRandom()
{
	return 1;
}


void hwLedSet(int val)
{
#ifdef LED_PIN
	gpio_put(LED_PIN, val);
#endif
#ifdef LED_WS2812_PIN
	pio_sm_put_blocking(pio0, 0, (val?0xad42ff:0) << 8u);
#endif
}

unsigned int storage_get_flash_capacity() {
    uint8_t txbuf[4] = {0x9f,0xff,0xff,0xff};
    uint8_t rxbuf[4] = {0,0,0,0};
    flash_do_cmd(txbuf, rxbuf, 4);

    return 1 << rxbuf[3];
}
//#define ROM_FUNC_FLASH_DEVINFO16 ROM_TABLE_CODE('E', 'X')
int main()
{
	uartInit();
	termInit();
//PRINTF(LOG_DEV,"\n\n----\n");
	FlashSize = storage_get_flash_capacity();

#ifdef LED_PIN
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
#ifdef LED_WS2812_PIN
	int sm=0;
	uint offset = pio_add_program(pio0, &ws2812_program);
	ws2812_program_init(pio0, sm, offset, LED_WS2812_PIN, 800000, false);
#endif

	//int devInfo=0;
	//rom_get_sys_info((uint32_t *)&devInfo,1,SYS_INFO_FLASH_DEV_INFO);
	//PRINTF(LOG_DEV,"devInfo=%x\n",devInfo);

	//uint16_t *devInfo16=NULL;
	//devInfo16=(uint16_t *)rom_func_lookup_inline(ROM_FUNC_FLASH_DEVINFO16);
	//PRINTF(LOG_DEV,"devInfo16=%x\n",devInfo16);
	//PRINTF(LOG_DEV,"devInfo16 value=%x\n",*devInfo16);

	flash_get_unique_id(FlashUniqueId);
//	PRINTF(LOG_DEV,"flash size: %d\n",FlashSize);
#ifdef USE_PSRAM
	int _psram_size = psram_init();
	MEMORY_PART_UPDATE_SIZE(1,_psram_size);
//	PRINTF(LOG_DEV,"psram=%d\n",_psram_size);
#endif

	start(2,Argv);

	while (true) {
		uartPutChar('.'); sleep_ms(500);
	}
}
