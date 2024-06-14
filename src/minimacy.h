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

#define VERSION_MINIMACY "1.3.4"


#ifdef ON_WINDOWS
#define DEVICE_MODE "Windows"
#ifdef _DEBUG
#define DBG_MEM
#endif
#include<windows.h>
#include<windowsx.h>
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

//#define MEMORY_C_SIZE (1024*1024*64)
//#define USE_MEMORY_C
//#define USE_WORKER_SYNC	// when USE_WORK_SYNC is set, no UI is possible as the event loop runs in a dedicated worker and never returns
#define USE_WORKER_ASYNC
#define USE_SOCKET_WIN
#define USE_ETH_STUB
#define USE_FS_SYSTEMDIR_WINDOWS
#define USE_FS_ANSI_WIN
#define USE_AUDIO_ENGINE
#define USE_SERIAL_WIN
//#define USE_MATH_C
#endif

#ifdef ON_NOTHING
#define DEVICE_MODE "BareMetal"
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024*1024*128)
#define USE_STDARG_ANSI
#define USE_RANDOM_C
#define USE_TIME_STUB
#define USE_TIME_MS_STUB
#define USE_ETH_STUB
#define USE_FS_ROMDISK0 "nothing_romdisk0.h"
#define USE_FS_SYSTEMDIR_STUB
#define USE_SERIAL_STUB
#ifdef WIN32
#define USE_CONSOLE_OUT_ANSI
#else
#define USE_CONSOLE_OUT_STUB
#endif
#define USE_CONSOLE_IN_STUB
#endif

#ifdef ON_UEFI
#define DEVICE_MODE "Uefi"
#define WITH_UART
#define USE_ETH_UEFI
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024*1024*128)
#define USE_STDARG_ANSI
#define USE_RANDOM_UEFI
#define USE_TIME_UEFI
#define USE_TIME_MS_UEFI
#define USE_SOCKET_UEFI
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_FS_SYSTEMDIR_STUB
//#define USE_FS_ROMDISK0 "../baremetal/uefi/uefi_romdisk0.h"
//#define USE_UEFI_MANUAL_BLIT
#define USE_FS_UEFI
#define USE_SERIAL_STUB
#endif

#ifdef ON_STM32
#define DEVICE_MODE "STM32"
#define ATOMIC_32
#define WITH_UART
#define GROUP_BAREMETAL
#define MEMORY_C_SIZE (1024*1024*128)
#define USE_STDARG_ANSI
#define USE_RANDOM_C
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_ETH_STUB
#define USE_FS_SYSTEMDIR_STUB
#define USE_FS_ROMDISK0 "../baremetal/stm32/stm32_romdisk0.h"
#define USE_SERIAL_STUB
#define USE_BOOTLOADER
#endif

#ifdef ON_RPI3
#define DEVICE_MODE "RPi3"
#define MEMORY_C_SIZE (1024*1024*920)
extern volatile unsigned char _end;
#define MEMORY_C_START ((char*)&_end)+4096*8
#define GROUP_RPI
#define USE_ETH_STUB
#endif

#ifdef ON_RPI4
#define DEVICE_MODE "RPi4"

// use this if your raspberry is less than 1GB:
extern volatile unsigned char _end;
#define MEMORY_C_SIZE (1024*1024*920)
#define MEMORY_C_START ((char*)&_end)+4096*33

//#define MEMORY_C_SIZE (1024*1024*1024)
//#define MEMORY_C_START 0x40000000

//#define MEMORY_C_SIZE (1024*1024*32)

#define GROUP_RPI
#define USE_ETH_RPI4
#endif

#ifdef GROUP_RPI
#define WITH_UART
#define WITH_ACTIVITY_LED
#define GROUP_BAREMETAL
#define USE_STDARG_GCC
#define USE_RANDOM_RPI
#define USE_TIME_RPI
#define USE_TIME_MS_RPI
#define USE_CONSOLE_OUT_UART
#define USE_CONSOLE_IN_UART
#define USE_BOOTLOADER
#define USE_FS_SYSTEMDIR_STUB
#define USE_FS_ROMDISK0 "../baremetal/raspberry/pi_romdisk0.h"
#define USE_SERIAL_STUB
#define USE_SOFT_CURSOR
//#define NEED_ALIGN
#endif

#ifdef GROUP_BAREMETAL
#define USE_THREAD_STUB
#define USE_WORKER_SYNC
#define USE_MEMORY_C
#define USE_TYPES_C
#define USE_MATH_C
#define USE_STR_C
#define USE_MINMAX_C
#define USE_SOCKET_STUB
#define HIDE_COMPILER_LISTING
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
#define USE_X11
#define USE_ALSA
#define USE_ETH_UNIX
#endif

#ifdef ON_RPIOS
#define DEVICE_MODE "RPiOS"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_FULL_ANSI
#define GROUP_UNIX
#define USE_FS_SYSTEMDIR_UNIX
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
#endif

#ifdef ON_IOS
#define DEVICE_MODE "IOs"
#define WITH_UI
#define WITH_GL
#define WITH_AUDIO
#define GROUP_UNIX
#define GROUP_FULL_ANSI
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
#define USE_CONSOLE_IN_ANSI
#define USE_TIME_MS_ANSI
#define USE_GLES
#define USE_OPENSLES
#define USE_ETH_STUB
#endif

#ifdef GROUP_UNIX
//#define WITH_DECODE_MP3
#define USE_THREAD_UNIX
#define USE_WORKER_ASYNC
#define USE_SOCKET_UNIX
#define USE_MINMAX_C
#define USE_RANDOM_UNIX
#define USE_FS_ANSI_UNIX
#define USE_SERIAL_UNIX
int startInThread(int argc, char** argv);
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
#include<stdio.h>
#include<stdint.h>

   #ifdef DBG_MEM
   #define _CRTDBG_MAP_ALLOC
   #include<stdlib.h>
   #include<crtdbg.h>
   #else
   #include<stdlib.h>
   #endif
#include<signal.h>
#endif


#ifdef USE_CONSOLE_OUT_ANSI
#include<stdio.h>
#endif

#ifdef WITH_UART
void uartPut(char* s, int len);
void uartPutChar(unsigned int c);
int uartReadable();
int uartWritable();
int uartGet();
#endif

#ifdef WITH_SECTOR_STORAGE
int storageRead(int index, char* buffer, int start, int nb);
int storageWrite(int index, char* buffer, int start, int nb);
#endif

#ifdef WITH_ACTIVITY_LED
void hwActivityLedSet(int val);
#endif

#ifdef USE_BOOTLOADER
char* bootDiskLoader();
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
#include "system_tmp.h"	// should be removed in the final product
#include "system.h"
#include "system_worker.h"
#include "system_file.h"
#include "system_bignum.h"
#include "system_core.h"
#include "system_2d.h"
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

int start(int argc, char** argv);

#endif
