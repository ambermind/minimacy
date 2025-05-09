/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
#ifndef _MINIMACY_
#define _MINIMACY_

#define VERSION_MINIMACY "1.3.8"

#ifdef ON_WINDOWS
#define DEVICE_MODE "Windows"
#ifdef _DEBUG
#define DBG_MEM
#endif
#include <windows.h>
#include <windowsx.h>
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
//#define WITH_DECODE_MP3
#define WITH_NATIVE_FONT
#define GROUP_COMMON_ANSI
#define USE_CONSOLE_OUT_ANSI
#define USE_CONSOLE_IN_WIN
#define USE_TIME_MS_WIN
#define USE_RANDOM_WIN
#define USE_THREAD_WIN

//#define USE_MEMORY_C
//#define MEMORY_C_SIZE (1024*480)
////#define MEMORY_C_SIZE (1024*1024*16)
//#define MEMORY_SAFE_SIZE (32*1024)
//#define FORGET_PARSER
//// #define USE_FS_ROMDISK0 "../baremetal/nothing/nothing_romdisk0.h"

#define USE_WORKER_ASYNC // when USE_WORK_ASYNC is not defined, no UI is possible as the event loop runs in a dedicated worker and never returns

#define USE_SOCKET_WIN
#define USE_ETH_STUB
#define USE_FS_SYSTEMDIR_WINDOWS
#define USE_FS_ANSI_WIN
#define USE_AUDIO_ENGINE
#define USE_SERIAL_WIN
// #define USE_MATH_C

#endif

#ifdef ON_NOTHING
#define DEVICE_MODE "BareMetal"
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024 * 1024 * 2)
#define USE_STDARG_ANSI
#define USE_RANDOM_C
#define USE_TIME_STUB
#define USE_TIME_MS_STUB
#define USE_ETH_STUB
#define USE_FS_ROMDISK0 "../baremetal/nothing/nothing_romdisk0.h"
#define USE_FS_SYSTEMDIR_STUB
#define USE_SERIAL_STUB
#ifdef WIN32
#define USE_CONSOLE_OUT_ANSI
#else
// #define USE_CONSOLE_OUT_STUB
#endif
// #define USE_CONSOLE_IN_ANSI
#define USE_CONSOLE_IN_STUB
#endif

#ifdef ON_UEFI
#define DEVICE_MODE "Uefi"
#define WITH_UART
#define WITH_SECTOR_STORAGE
#define USE_ETH_UEFI
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024 * 1024 * 128)
#define USE_STDARG_ANSI
#define USE_RANDOM_UEFI
#define USE_TIME_UEFI
#define USE_TIME_MS_UEFI
#define USE_SOCKET_UEFI
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_FS_SYSTEMDIR_STUB
#define USE_FS_ROMDISK0 "../baremetal/uefi/uefi_romdisk0.h"
// #define USE_UEFI_MANUAL_BLIT
#define USE_SERIAL_STUB
#define USE_HOST_ONLY_FUNCTIONS
#define USE_BOOTLOADER

#define USE_KEYBOARD // accept keyboard input as well as the uart input
// uncomment the following line to display the console on the monitor as well as on the uart
// this may be very slow on some devices
// and the char code '8' will put some trouble on the
// #define USE_CONSOLE_MONITOR

#endif

#ifdef ON_ESP32
#define DEVICE_MODE "ESP32"
#define ATOMIC_32
#define WITH_UART
#define USE_THREAD_STUB
// #define USE_MEMORY_C
#define USE_TYPES_C
#define USE_MATH_C
// #define USE_STR_C
#define USE_MINMAX_C
#define USE_SOCKET_STUB
// #define HIDE_COMPILER_LISTING
// #define MEMORY_C_SIZE (1024*1024*2)
#define USE_STDARG_ANSI
#define USE_RANDOM_C
#define USE_TIME_ANSI
#define USE_TIME_MS_ANSI
#define USE_ETH_STUB
#define USE_FS_ROMDISK0 "esp32_romdisk0.h"
#define USE_FS_SYSTEMDIR_STUB
#define USE_SERIAL_STUB
#define USE_CONSOLE_OUT_ANSI
#define USE_CONSOLE_IN_UART
#define USE_BOOTLOADER
#include <string.h>
#endif

#ifdef ON_STM32F7
#define DEVICE_MODE "ON_STM32F7"
#define ATOMIC_32
#define WITH_UART
#define WITH_ACTIVITY_LED
#define WITH_SDRAM
#define WITH_SECTOR_STORAGE
#define GROUP_BAREMETAL
#define USE_MEMORY_C
#ifdef WITH_SDRAM
#define MEMORY_C_START (char *)0xC0000000
#define MEMORY_C_SIZE (1024 * 1024 * 16)
#define MEMORY_SAFE_SIZE (512 * 1024)
#else
#define MEMORY_C_SIZE (450 * 1024)
#define MEMORY_SAFE_SIZE (32 * 1024)
#endif
#define USE_STDARG_ANSI
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_ETH_STUB
#define USE_FS_SYSTEMDIR_STUB
#define USE_FS_ROMDISK0 "../baremetal/st/stm32f769/Core/Src/stm32_romdisk0.h"
#define USE_SERIAL_STUB
#define USE_BOOTLOADER
#define USE_HOST_ONLY_FUNCTIONS
#endif

#ifdef ON_ILABS_CHALLENGER_RP2350_BCONNECT
#define DEVICE_MODE "IlabsChallengerRP2350BConnect"
#define FLASH_SIZE (1024 * 1024 * 8)
#define USE_PSRAM
#define MEMORY_C_START (char *)0x11000000
#define MEMORY_SAFE_SIZE (512 * 1024)
// #define MEMORY_C_SIZE (1024*472)
// #define MEMORY_SAFE_SIZE (32*1024)
#define GROUP_RP2350
#endif

#ifdef ON_SPARKFUN_PRO_MICRO_RP2350
#define DEVICE_MODE "SparkFunProMicroRP2350"
#define FLASH_SIZE (1024 * 1024 * 16)
#define USE_PSRAM
#define MEMORY_C_START (char *)0x11000000
#define MEMORY_SAFE_SIZE (512 * 1024)
#define GROUP_RP2350
#endif

#ifdef ON_ADAFRUIT_FEATHER_RP2350_PSRAM
#define DEVICE_MODE "AdafruitFeatherRP2350"
#define FLASH_SIZE (1024 * 1024 * 8)
#define USE_PSRAM
#define MEMORY_C_START (char *)0x11000000
#define MEMORY_SAFE_SIZE (512 * 1024)
//#define MEMORY_C_SIZE (1024 * 472)
//#define MEMORY_SAFE_SIZE (32 * 1024)
#define GROUP_RP2350
#endif

#ifdef ON_PICO_2
#define DEVICE_MODE "Pico2"
#define FLASH_SIZE (1024 * 1024 * 4)
#define MEMORY_C_SIZE (1024 * 472)
#define MEMORY_SAFE_SIZE (32 * 1024)
#define GROUP_RP2350
#endif

#ifdef GROUP_RP2350
#define ATOMIC_32
#define WITH_UART
#define WITH_ACTIVITY_LED
#define GROUP_BAREMETAL

#define USE_STDARG_ANSI
#define USE_ETH_STUB
#define USE_FS_ROMDISK0 "../baremetal/raspberry-2350/rp2350_romdisk0.h"
#define USE_FS_SYSTEMDIR_STUB
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_BOOTLOADER
#define USE_HOST_ONLY_FUNCTIONS
#endif

#ifdef ON_RPI3
#define DEVICE_MODE "RPi3"
#define MEMORY_C_SIZE (1024 * 1024 * 920)
extern volatile unsigned char _end;
#define MEMORY_C_START ((char *)&_end) + 4096 * 8
#define GROUP_RPI
#define USE_ETH_STUB
#define USE_SERIAL_STUB
#endif

#ifdef ON_RPI4
#define DEVICE_MODE "RPi4"

// use this if your raspberry is less than 1GB:
extern volatile unsigned char _end;
#define MEMORY_C_SIZE (1024 * 1024 * 920)
#define MEMORY_C_START ((char *)&_end) + 4096 * 33

// #define MEMORY_C_SIZE (1024*1024*1024)
// #define MEMORY_C_START 0x40000000

// #define MEMORY_C_SIZE (1024*1024*32)

#define GROUP_RPI
#define USE_ETH_RPI4
#define NEED_ALIGN
#endif

#ifdef ON_RPI5
#define DEVICE_MODE "RPi5"

extern volatile unsigned char _end;
#define MEMORY_C_SIZE (1024 * 1024 * 920)
#define MEMORY_C_START ((char *)&_end) + 4096 * (128+1)

#define GROUP_RPI
#define USE_ETH_STUB
#define USE_SERIAL_STUB
#define WITH_POWER_SWITCH
#define NEED_ALIGN
#endif

#ifdef GROUP_RPI
#define WITH_UART
#define WITH_ACTIVITY_LED
#define GROUP_BAREMETAL
#define USE_STDARG_GCC
#define USE_TIME_RPI
#define USE_TIME_MS_RPI
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_BOOTLOADER
#define USE_FS_SYSTEMDIR_STUB
#define USE_FS_ROMDISK0 "../baremetal/raspberry/pi_romdisk0.h"
#define USE_SOFT_CURSOR
#define USE_HOST_ONLY_FUNCTIONS
// #define NEED_ALIGN
#endif

#ifdef ON_UNIX_X11GL
#define ON_UNIX
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#endif

#ifdef ON_UNIX
#define DEVICE_MODE "Unix"
#define GROUP_FULL_ANSI
#define GROUP_UNIX
#define USE_FS_SYSTEMDIR_UNIX
#define USE_DEVICE_UNIX
#define USE_X11
#define USE_ALSA
#define USE_ETH_UNIX
#endif

#ifdef ON_UNIX_BM
#define DEVICE_MODE "UnixBareMetal"
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024 * 1024 * 128)
#define USE_ETH_STUB
#define USE_SERIAL_STUB
#define USE_STDARG_ANSI
#define USE_RANDOM_C
#define USE_TIME_ANSI
//#define USE_TIME_STUB
#define USE_TIME_MS_ANSI
//#define USE_TIME_MS_STUB
#define USE_ETH_STUB
#define USE_FS_ROMDISK0 "../baremetal/nothing/nothing_romdisk0.h"
#define USE_FS_SYSTEMDIR_STUB
#define USE_SERIAL_STUB
#define USE_CONSOLE_OUT_ANSI
#define USE_CONSOLE_IN_STUB

#define NEED_ALIGN

#endif

#ifdef ON_RPIOS
#define DEVICE_MODE "RPiOS"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_FULL_ANSI
#define GROUP_UNIX
#define USE_FS_SYSTEMDIR_UNIX
#define USE_DEVICE_UNIX
#define USE_X11
#define USE_ALSA
#define USE_ETH_UNIX
#endif

#ifdef ON_MACOS_X11GL
#define ON_MACOS_CMDLINE
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#endif

#ifdef ON_MACOS_CMDLINE
#define DEVICE_MODE "MacOsCmdLine"
#define GROUP_FULL_ANSI
#define GROUP_UNIX
#define USE_FS_SYSTEMDIR_UNIX
#define USE_MACOS
#define USE_X11
#define USE_AUDIOTOOLBOX
#define DEPRECATED_SEM
#define USE_ETH_STUB
#define USE_DEVICE_UNIX
#endif

#ifdef ON_MACOS
#define DEVICE_MODE "MacOs"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_FULL_ANSI
#define GROUP_UNIX
#define USE_MACOS
#define USE_COCOA
#define USE_AUDIOTOOLBOX
#define DEPRECATED_SEM
#define USE_ETH_STUB
#define USE_DEVICE_UNIX
#endif

#ifdef ON_IOS
#define DEVICE_MODE "IOs"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_UNIX
#define GROUP_COMMON_ANSI
#define USE_CONSOLE_OUT_ANSI
#define USE_CONSOLE_IN_STUB
// #define USE_CONSOLE_IN_ANSI
#define USE_TIME_MS_ANSI
#define USE_GLES
#define USE_AUDIOTOOLBOX
#define DEPRECATED_SEM
#define USE_ETH_STUB
#endif

#ifdef ON_ANDROID
#define DEVICE_MODE "Android"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_UNIX
#define GROUP_COMMON_ANSI
#define USE_CONSOLE_OUT_ANDROID
#define USE_CONSOLE_IN_STUB
// #define USE_CONSOLE_IN_ANSI
#define USE_TIME_MS_ANSI
#define USE_GLES
#define USE_OPENSLES
#define USE_ETH_STUB
#endif

#ifdef GROUP_BAREMETAL
#define USE_THREAD_STUB
#define USE_MEMORY_C
#define USE_TYPES_C
#define USE_MATH_C
#define USE_STR_C
#define USE_MINMAX_C
#define USE_SOCKET_STUB
#define HIDE_COMPILER_LISTING
#define FORGET_PARSER
#endif

#ifdef GROUP_UNIX
// #define WITH_DECODE_MP3
#define USE_THREAD_UNIX
#define USE_WORKER_ASYNC
#define USE_SOCKET_UNIX
#define USE_MINMAX_C
#define USE_RANDOM_UNIX
#define USE_FS_ANSI_UNIX
#define USE_SERIAL_UNIX
int startInThread(int argc, const char **argv);
#endif

#ifdef GROUP_FULL_ANSI
#define GROUP_COMMON_ANSI
#define USE_CONSOLE_OUT_ANSI
#define USE_CONSOLE_IN_ANSI
#define USE_TIME_MS_ANSI
#endif

#ifdef GROUP_COMMON_ANSI
#define USE_MEMORY_ANSI
#define USE_STDARG_ANSI
#define USE_TIME_ANSI
#define USE_MATH_ANSI
#define USE_STR_ANSI
#endif

#ifdef USE_FS_ANSI_WIN
#define USE_FS_ANSI
#endif

#ifdef USE_FS_ANSI_UNIX
#define USE_FS_ANSI
#endif

#ifdef WITH_UI
#ifdef USE_X11
#define WITH_NATIVE_FONT
#endif
#endif

#ifdef USE_MEMORY_ANSI
#include <stdio.h>
#include <stdint.h>

#ifdef DBG_MEM
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif
#include <signal.h>
#endif

#ifdef USE_CONSOLE_OUT_ANSI
#include <stdio.h>
#endif

#ifdef WITH_UART
void uartPut(char *s, int len);
void uartPutChar(unsigned int c);
int uartReadable();
int uartWritable();
int uartGet();
#endif

#ifdef WITH_SECTOR_STORAGE
int storageRead(int index, char *buffer, int start, int nb);
int storageWrite(int index, char *buffer, int start, int len);
int storageNbSectors(int index);
int storageSectorSize(int index);
int storageWritable(int index);
int storageCount();
#endif

#ifdef WITH_ACTIVITY_LED
void hwActivityLedSet(int val);
#endif

#ifdef USE_BOOTLOADER
char *bootDiskLoader();
#endif

#include "hw_thread.h"
#include "vm_memory.h"
#include "util_ansi.h"
#include "compiler_globals.h"
#include "compiler_locals.h"
#include "compiler_parser.h"
#include "compiler.h"
#include "hw.h"
#include "hw_fs.h"
#include "system_tmp.h" // should be removed in the final product
#include "system.h"
#include "system_worker.h"
#include "system_file.h"
#include "system_bignum.h"
#include "system_core.h"
#include "system_2d.h"
#include "system_serial.h"
#include "system_socket.h"
#include "system_event.h"
#include "system_ui.h"
#include "system_ui_gl.h"
#include "system_audio.h"
#include "util.h"
#include "util_2d.h"
#include "util_buffer.h"
#include "util_convert.h"
#include "util_hashmap.h"
#include "vm_interpreter.h"
#include "vm_opcodes.h"
#include "vm_term.h"
#include "vm_thread.h"
#include "vm_types.h"

#define BOOT_FILE "bios"

int start(int argc, const char **argv);

#endif
