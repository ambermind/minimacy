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

#define VERSION_MINIMACY "0.2.0"

#ifdef ON_UNIX
#include<stdio.h>
#include<signal.h>
#else
#define ON_WINDOWS
#ifdef _DEBUG
#define DBG_MEM
#endif
#endif

#include<stdio.h>
#include<stdint.h>
#ifdef DBG_MEM
#define _CRTDBG_MAP_ALLOC
#include<stdlib.h>
#include<crtdbg.h>
#else
#include<stdlib.h>
#endif

#include<string.h>
#include<math.h>
#include<stdarg.h>
#include<time.h>

#ifdef ON_UNIX
#include<sys/types.h>
#include<sys/time.h>
#define strnicmp strncasecmp
#define strcmpi strcasecmp
#define _vsnprintf vsnprintf

#ifdef USE_IOS
#define DEVICE_MODE "ios"
#elif USE_COCOA
#define DEVICE_MODE "macos"
#elif ON_MACOS
#define DEVICE_MODE "unixOnMac"
#else
#define DEVICE_MODE "unix"
#endif

#ifdef WITH_UI

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xatom.h>
#endif
#endif
#endif

#ifdef ON_WINDOWS
#include<windows.h>
#include<windowsx.h>
#define WITH_UI
#define WITH_GLES2
#define DEVICE_MODE "windows"
#endif

#include "hw_thread.h"
#include "vm_memory.h"
#include "compiler_globals.h"
#include "compiler_locals.h"
#include "compiler_parser.h"
#include "compiler.h"
#include "hw.h"
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


#define LS_ERR_SN -1
#define LS_ERR_TYPE -2

#define BOOT_FILE "bios"

int startInThread(int argc, char** argv);
#endif
