// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"


#include <stdio.h>

#include"../../src/minimacy.h"
#include "raspberry-2350.h"

// from rp2350-datasheet
//#define SIO_BASE 0xd0000000	// already defined in sdk
#define GPIO_IN (SIO_BASE+0x04)
#define GPIO_HI_IN (SIO_BASE+0x08)

#ifdef GPIO_UART1_TX
int Uart1FD;

int _serialCheckReadable(Socket* s)
{
	if (s && s->fd==Uart1FD) return uart_is_readable(uart1);
	return 1;
}
int _serialCheckWritable(Socket* s)
{
	if (s && s->fd==Uart1FD) return 1;
	return 0;
}

int fun_serialOpen(Thread* th)
{
	Socket* s;
	LINT stop = STACK_INT(th, 0);
	LINT parity = STACK_INT(th, 1);
	LINT bits = STACK_INT(th, 2);
	LINT bds = STACK_INT(th, 3);
	LB* v = STACK_PNT(th, 4);
	if (!v) FUN_RETURN_NIL;
	if (strcmp(STR_START(v),"uart1")) FUN_RETURN_NIL;

	if (Uart1FD) FUN_RETURN_NIL;	// already open

	uart_init(uart1,bds);
	if (!stop) stop=1;	// 0 -> 1 bit
	if (parity) parity=3-parity;	// exchange 1 and 2 (ODD and EVEN)
	uart_set_format(uart1, bits, stop, parity);

	Uart1FD=socketNextFd();
	s = _socketCreate(Uart1FD); if (!s) return EXEC_OM;
	s->checkReadable= _serialCheckReadable;
	s->checkWritable= _serialCheckWritable;
	FUN_RETURN_PNT((LB*)s);
}
int fun_serialClose(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && s->fd==Uart1FD) {
		s->fd=INVALID_SOCKET;
		Uart1FD=0;
	}
	return 0;
}
int fun_serialWrite(Thread* th)
{
	LINT sent,len=0;
	char *p;

	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s)|| (s->fd != Uart1FD)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));

	if (len==0) FUN_RETURN_INT(start);
	sent=len;
	p=STR_START(src) + start;
	while(len--) {
		uart_tx_wait_blocking(uart1);
		uart_putc_raw(uart1,*(p++));
	}
	FUN_RETURN_INT(start + sent);
}
int fun_serialRead(Thread* th) 
{
	char buffer[32];	// RX fifo is 32 bytes wide
	int len=0;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if ((!s)|| (s->fd !=Uart1FD)) FUN_RETURN_NIL;

	while((len<32) && uart_is_readable(uart1)) buffer[len++]=uart_getc(uart1);
	FUN_RETURN_STR(buffer, len);
}
int fun_serialSocket(Thread* th) {
	return 0;
}
int fun_serialList(Thread* th) {
	char label[32];
	int n=0;
	snprintf(label,31,"TX:gpio%d RX:gpio%d",GPIO_UART1_TX, GPIO_UART1_RX);
	STACK_PUSH_STR_ERR(th, "uart1", -1, EXEC_OM);
	STACK_PUSH_STR_ERR(th, label, -1, EXEC_OM);
	FUN_MAKE_ARRAY(2, DBG_TUPLE);
	n++;
	FUN_PUSH_NIL;
	while (n--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}
#else
int fun_serialList(Thread* th) FUN_RETURN_NIL
int fun_serialOpen(Thread* th) FUN_RETURN_NIL
int fun_serialClose(Thread* th) FUN_RETURN_NIL
int fun_serialWrite(Thread* th) FUN_RETURN_NIL
int fun_serialRead(Thread* th) FUN_RETURN_NIL
int fun_serialSocket(Thread* th) FUN_RETURN_NIL

#endif
//#define FLASH_PAGE_SIZE        256
//#define FLASH_SECTOR_SIZE      4096

void xpi_cache_clean()	// write out any data stored in the cache to external memory
{
	for(int j=1;j<2048*8;j+=8) *(((char*)0x19000000)+j)=1;
}
void xpi_cache_invalidate()	// clear the cache
{
	for(int j=0;j<2048*8;j+=8) *(((char*)0x19000000)+j)=1;
}
int fun_flashSize(Thread* th) {
	FUN_RETURN_INT(FlashSize);
}

int fun_flashRead(Thread* th) {
	char* flashStart=(char*)XIP_BASE;
	LINT len=STACK_INT(th,0);
	LINT start=STACK_INT(th,1);
	if (start<0 || len<=0 || (start+len>FlashSize)) FUN_RETURN_NIL;
//	PRINTF(LOG_DEV,"read %d at %x\n",start,(int)flashStart);
	FUN_RETURN_STR(flashStart+start,len)
}

char FlashBuffer[FLASH_SECTOR_SIZE];

static int __no_inline_not_in_flash_func(fun_flashWrite)(Thread* th) {
	LINT start=STACK_INT(th,0);
	LB* src=STACK_PNT(th,1);
	if (start<0 || (start&(FLASH_SECTOR_SIZE-1)) || (start+FLASH_SECTOR_SIZE>FlashSize)) FUN_RETURN_NIL;
	if ((!src)||(STR_LENGTH(src)!=FLASH_SECTOR_SIZE)) FUN_RETURN_NIL;
	memcpy(FlashBuffer,STR_START(src),FLASH_SECTOR_SIZE);

	uint32_t intr_stash = save_and_disable_interrupts();
#ifdef USE_PSRAM
	xpi_cache_clean();
#endif
	flash_range_erase(start, FLASH_SECTOR_SIZE);
	flash_range_program(start, FlashBuffer, FLASH_SECTOR_SIZE);
	restore_interrupts_from_disabled(intr_stash);
#ifdef USE_PSRAM
	psram_init();
#endif
	FUN_RETURN_TRUE
}

int fun_gpioInit(Thread* th) {
	LINT gpio=STACK_INT(th,0);
	gpio_init(gpio);
	FUN_RETURN_INT(gpio);
}
int fun_gpioDeinit(Thread* th) {
	LINT gpio=STACK_INT(th,0);
	gpio_deinit(gpio);
	FUN_RETURN_INT(gpio);
}
int fun_gpioSetDir(Thread* th) {
	LINT dir=STACK_BOOL(th,1);
	LINT gpio=STACK_INT(th,0);
	gpio_set_dir(gpio,dir);
	FUN_RETURN_INT(gpio);
}
int fun_gpioGet(Thread* th) {
	LINT gpio=STACK_INT(th,0);
	int val=gpio_get(gpio);
	FUN_RETURN_BOOL(val);
}
int fun_gpioPut(Thread* th) {
	LINT val=STACK_BOOL(th,1);
	LINT gpio=STACK_INT(th,0);
	gpio_put(gpio,val);
	STACK_DROP(th);
	return 0;
}

int fun_gpioSetPulls(Thread* th) {
	LINT up=STACK_BOOL(th,2);
	LINT down=STACK_BOOL(th,1);
	LINT gpio=STACK_INT(th,0);
	gpio_set_pulls(gpio,up,down);
	FUN_RETURN_INT(gpio);
}

int fun_gpioWatcherAddress(Thread* th) {
	LINT gpio=STACK_INT(th,0);
	FUN_RETURN_INT((gpio>=32)?GPIO_HI_IN:GPIO_IN);
}
int fun_gpioWatcherMask(Thread* th) {
	LINT gpio=STACK_INT(th,0);
	FUN_RETURN_INT(1<<(gpio&31));
}


LINT I2cBaudrates[2];	// we need to memorize the last baudrate, as there is no sdk function to get it back

int fun_i2cInit(Thread* th) {
	LINT i2c=STACK_INT(th,3);
	LINT baudrate=STACK_INT(th,2);
	LINT sda_pin=STACK_INT(th,1);
	LINT scl_pin=STACK_INT(th,0);
	if (i2c!=0 && i2c!=1) FUN_RETURN_NIL;
	if (baudrate<=0) FUN_RETURN_NIL;
	i2c_init(i2c?i2c1:i2c0, baudrate);
	I2cBaudrates[i2c]=baudrate;
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(sda_pin);
	gpio_pull_up(scl_pin);
	FUN_RETURN_INT(i2c);
}
int fun_i2cSetBaudrate(Thread* th) {
	LINT i2c=STACK_INT(th,1);
	LINT baudrate=STACK_INT(th,0);
	if (i2c!=0 && i2c!=1) FUN_RETURN_NIL;
	if (baudrate<=0) FUN_RETURN_NIL;
	i2c_set_baudrate(i2c?i2c1:i2c0, baudrate);
	I2cBaudrates[i2c]=baudrate;
	FUN_RETURN_INT(i2c);
}
int fun_i2cBaudrate(Thread* th) {
	LINT i2c=STACK_INT(th,0);
	if (i2c!=0 && i2c!=1) FUN_RETURN_NIL;
	if (I2cBaudrates[i2c]<=0) FUN_RETURN_NIL;
	FUN_RETURN_INT(I2cBaudrates[i2c]);
}

int fun_i2cWrite(Thread* th) {
	int result;
	LINT i2c=STACK_INT(th,2);
	LINT addr=STACK_INT(th,1);
	LB* data=STACK_PNT(th,0);
	if (i2c!=0 && i2c!=1) FUN_RETURN_NIL;
	if ((!data)||(!STR_LENGTH(data))) FUN_RETURN_NIL;
	result = i2c_write_blocking(i2c?i2c1:i2c0, addr, STR_START(data),STR_LENGTH(data),0);
	FUN_RETURN_INT(result);
}
int fun_i2cRead(Thread* th) {
	char buffer[256];
	LINT i2c=STACK_INT(th,2);
	LINT addr=STACK_INT(th,1);
	LINT len=STACK_INT(th,0);
	if (i2c!=0 && i2c!=1) FUN_RETURN_NIL;
	if ((len<=0)||(len>256)) FUN_RETURN_NIL;
	len=i2c_read_blocking(i2c?i2c1:i2c0, addr, buffer, len, 0);
	if(len<=0) FUN_RETURN_NIL;
	FUN_RETURN_STR(buffer,len);
}
int fun_spiInit(Thread* th) {
	LINT spi=STACK_INT(th,4);
	LINT baudrate=STACK_INT(th,3);
	LINT clk_pin=STACK_INT(th,2);
	LINT mosi_pin=STACK_INT(th,1);
	LINT miso_pin=STACK_INT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	spi_init(spi?spi1:spi0, baudrate);
    gpio_set_function(clk_pin, GPIO_FUNC_SPI);
	gpio_pull_up(clk_pin);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
	gpio_pull_up(mosi_pin);
	if (!STACK_IS_NIL(th,0)) {
		gpio_set_function(miso_pin, GPIO_FUNC_SPI);
		gpio_pull_up(miso_pin);
	}
	FUN_RETURN_INT(spi);
}

int fun_spiSetBaudrate(Thread* th) {
	LINT spi=STACK_INT(th,1);
	LINT baudrate=STACK_INT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	baudrate=spi_set_baudrate(spi?spi1:spi0, baudrate);
	FUN_RETURN_INT(baudrate);
}

int fun_spiWriteByte(Thread* th) {
	LINT result=0;
	LINT spi=STACK_INT(th,1);
	LINT val=STACK_INT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	spi_write_read_blocking(spi?spi1:spi0, (char*)&val, (char*)&result, 1);
	FUN_RETURN_INT(result);
}
int fun_spiWrite(Thread* th) {
	int result;
	LINT spi=STACK_INT(th,1);
	LB* data=STACK_PNT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	if ((!data)||(!STR_LENGTH(data))) FUN_RETURN_NIL;
	result=spi_write_blocking(spi?spi1:spi0, STR_START(data),STR_LENGTH(data));
	FUN_RETURN_INT(result);
}
int fun_spiWriteRead(Thread* th) {
	LB* input;
	LINT spi=STACK_INT(th,1);
	LB* data=STACK_PNT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	if ((!data)||(!STR_LENGTH(data))) FUN_RETURN_NIL;
	input=memoryAllocStr(NULL,STR_LENGTH(data)); if (!input) return EXEC_OM;
	spi_write_read_blocking(spi?spi1:spi0, STR_START(data),STR_START(input),STR_LENGTH(data));
	FUN_RETURN_PNT(input);
}
int fun_spiRead(Thread* th) {
	LB* input;
	LINT spi=STACK_INT(th,2);
	LINT byte=STACK_INT(th,1);
	LINT len=STACK_INT(th,0);
	if (spi!=0 && spi!=1) FUN_RETURN_NIL;
	if (len<=0) FUN_RETURN_NIL;
	input=memoryAllocStr(NULL,len); if (!input) return EXEC_OM;
	spi_read_blocking(spi?spi1:spi0, (unsigned char)byte,STR_START(input),len);
	FUN_RETURN_PNT(input);
}

int fun_pwmInit(Thread* th) {
	int slice_num;
	LINT pin=STACK_INT(th,5);
	LINT wrap=STACK_INT(th,4);
	LINT chan=STACK_INT(th,3);
	LINT level=STACK_INT(th,2);
	LINT clkdiv=STACK_INT(th,1);
	LINT enabled=STACK_BOOL(th,0);
	gpio_set_function(pin, GPIO_FUNC_PWM);
	slice_num = pwm_gpio_to_slice_num(pin);
	pwm_set_wrap(slice_num, wrap);
	pwm_set_chan_level(slice_num, chan, level);
	pwm_set_clkdiv(slice_num, clkdiv);
	pwm_set_enabled(slice_num, enabled);
	FUN_RETURN_INT(pin);
}

int fun_read32(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0);
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	val= *(int volatile *) addr;
	FUN_RETURN_INT(val)
}


int hostOnlyFunctionsInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "flashSize", fun_flashSize, "fun -> Int" },
		{ NATIVE_FUN, "flashRead", fun_flashRead, "fun Int Int -> Bytes" },
		{ NATIVE_FUN, "flashWrite", fun_flashWrite, "fun Bytes Int -> Bool" },

		
		{ NATIVE_FUN, "gpioInit", fun_gpioInit, "fun Int -> Int" },
		{ NATIVE_FUN, "gpioDeinit", fun_gpioDeinit, "fun Int -> Int" },
		{ NATIVE_FUN, "gpioSetDir", fun_gpioSetDir, "fun Bool Int -> Int" },
		{ NATIVE_FUN, "gpioGet", fun_gpioGet, "fun Int -> Bool" },
		{ NATIVE_FUN, "gpioPut", fun_gpioPut, "fun Bool Int -> Int" },
		{ NATIVE_FUN, "gpioSetPulls", fun_gpioSetPulls, "fun Bool Bool Int -> Int" },

		{ NATIVE_FUN, "gpioWatcherAddress", fun_gpioWatcherAddress, "fun Int -> Int" },
		{ NATIVE_FUN, "gpioWatcherMask", fun_gpioWatcherMask, "fun Int -> Int" },

		{ NATIVE_INT, "I2c0", (void*)0, "I2c" },
		{ NATIVE_INT, "I2c1", (void*)1, "I2c" },
		{ NATIVE_FUN, "i2cInit", fun_i2cInit, "fun I2c Int Int Int -> I2c" },
		{ NATIVE_FUN, "i2cBaudrate", fun_i2cBaudrate, "fun I2c -> Int" },
		{ NATIVE_FUN, "i2cSetBaudrate", fun_i2cSetBaudrate, "fun I2c Int -> I2c" },
		{ NATIVE_FUN, "i2cWrite", fun_i2cWrite, "fun I2c Int Str -> Int" },
		{ NATIVE_FUN, "i2cWriteBytes", fun_i2cWrite, "fun I2c Int Bytes -> Int" },
		{ NATIVE_FUN, "i2cRead", fun_i2cRead, "fun I2c Int Int -> Str" },

		{ NATIVE_INT, "Spi0", (void*)0, "Spi" },
		{ NATIVE_INT, "Spi1", (void*)1, "Spi" },
		{ NATIVE_FUN, "spiInit", fun_spiInit, "fun Spi Int Int Int Int -> Spi" },
		{ NATIVE_FUN, "spiSetBaudrate", fun_spiSetBaudrate, "fun Spi Int -> Int" },
		{ NATIVE_FUN, "spiWriteByte", fun_spiWriteByte, "fun Spi Int -> Int" },
		{ NATIVE_FUN, "spiWrite", fun_spiWrite, "fun Spi Str -> Int" },
		{ NATIVE_FUN, "spiWriteRead", fun_spiWriteRead, "fun Spi Str -> Str" },
		{ NATIVE_FUN, "spiRead", fun_spiRead, "fun Spi Int Int -> Str" },
		{ NATIVE_FUN, "spiWriteBytes", fun_spiWrite, "fun Spi Bytes -> Int" },
		{ NATIVE_FUN, "spiWriteReadBytes", fun_spiWriteRead, "fun Spi Bytes -> Str" },
		{ NATIVE_FUN, "spiReadBytes", fun_spiRead, "fun Spi Int Int -> Bytes" },

		//{ NATIVE_INT, "PWM_CHAN_A", (void*)0, "Int" },
		//{ NATIVE_INT, "PWM_CHAN_B", (void*)1, "Int" },
		//{ NATIVE_FUN, "pwmInit", fun_pwmInit, "fun Int Int Int Int Int Bool-> Int" },

		//{ NATIVE_FUN, "read32", fun_read32, "fun Int -> Int"},

	};
	pkgAddType(system, "Spi");
	pkgAddType(system, "I2c");
	NATIVE_DEF(nativeDefs);
	pkgAddConstPnt(system, "flashUniqueId", memoryAllocStr(FlashUniqueId,8), MM.Str);
	Uart1FD=0;
	I2cBaudrates[0]=I2cBaudrates[1]=0;
	return 0;
}
