// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#ifdef ON_MM_2350
//#define XIP_CSI_PIN 8
#define XIP_CSI_PIN 19

#define GPIO_UART_TX 0
#define GPIO_UART_RX 1

#define GPIO_UART1_TX 8
#define GPIO_UART1_RX 9

#define LED_PIN 21
#define RP2350_BOOT "/baremetal/baremetal.mm2350.boot.mcy"
#endif

#ifdef ON_ILABS_CHALLENGER_RP2350_BCONNECT
#define XIP_CSI_PIN 0
#define GPIO_UART_TX 12
#define GPIO_UART_RX 13

#define GPIO_UART1_TX 24
#define GPIO_UART1_RX 25

// two user leds: simple led and WS2812 (RGB)
#define LED_PIN 7
//#define LED_WS2812_PIN 22
#endif

#ifdef ON_SPARKFUN_PRO_MICRO_RP2350
#define XIP_CSI_PIN 19
#define GPIO_UART_TX 0
#define GPIO_UART_RX 1

#define GPIO_UART1_TX 8
#define GPIO_UART1_RX 9

// no simple led, only WS2812 (RGB)
//#define LED_WS2812_PIN 7
#define LED_WS2812_PIN 25	
#endif

#ifdef ON_ADAFRUIT_FEATHER_RP2350_PSRAM
#define XIP_CSI_PIN 8
#define GPIO_UART_TX 0
#define GPIO_UART_RX 1

//#define GPIO_UART1_TX 8
//#define GPIO_UART1_RX 9

// two user leds: simple led and WS2812 (RGB)
#define LED_PIN 7
//#define LED_WS2812_PIN 21
#endif

#ifdef ON_PICO_2
#define GPIO_UART_TX 0
#define GPIO_UART_RX 1

#define GPIO_UART1_TX 8
#define GPIO_UART1_RX 9

#define LED_PIN 25
#endif

#ifndef RP2350_BOOT
#define RP2350_BOOT "/baremetal/baremetal.rp2350.boot.mcy"
#endif

int __no_inline_not_in_flash_func(psram_init)();
extern char FlashUniqueId[8];
extern unsigned int FlashSize;