# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2022, Sylvain Huet, Ambermind
# Minimacy (r) System

UNAME := $(shell uname -s)
IS_WSL := $(shell grep -qi microsoft /proc/version 2>/dev/null && echo yes)

CC := $(shell for c in aarch64-none-elf-gcc aarch64-elf-gcc aarch64-linux-gnu-gcc; do command -v $$c && break; done)
ifeq ($(CC),)
  $(error AArch64 bare-metal GCC not found — apt-get install gcc-aarch64-linux-gnu, brew install aarch64-elf-gcc, or toolchain Arm aarch64-none-elf)
endif
AARCH64 := $(CC:gcc=)
LD      := $(CC)             
CP      := $(AARCH64)objcopy

ROOT:= ../..
OBJDIR := $(ROOT)/obj_pi
vpath %.c $(ROOT)/src
vpath %.h $(ROOT)/src

CORE= $(addprefix $(OBJDIR)/, \
compiler.o \
compiler_call.o \
compiler_globals.o \
compiler_instances.o \
compiler_letgetset.o \
compiler_locals.o \
compiler_loops.o \
compiler_match.o \
compiler_parser.o \
compiler_prog.o \
compiler_step1.o \
compiler_step2.o \
compiler_step3.o \
compiler_step4.o \
compiler_struct.o \
compiler_sum.o \
compiler_term.o \
compiler_try.o \
crypto_aes.o \
crypto_checksum.o \
crypto_des.o \
crypto_hash.o \
crypto_rc4.o \
hw.o \
hw_console.o \
hw_fs.o \
hw_fs_ansi.o \
hw_fs_romdisk.o \
hw_random.o \
hw_thread.o \
hw_time.o \
minimacy.o \
system_bignum.o \
system.o \
system_2d.o\
system_audio.o\
system_binary.o \
system_buffer.o \
system_bytes.o \
system_core.o \
system_convert.o \
system_crypto.o \
system_deflate.o \
system_event.o \
system_file.o \
system_inflate.o \
system_lzw.o \
system_serial.o \
system_socket.o \
system_str.o \
system_tmp.o \
system_ui.o \
system_ui_gl.o \
system_ui_x11.o \
system_worker.o \
util.o \
util_2d.o \
util_ansi_math.o \
util_ansi_str.o \
util_bignum.o \
util_bignum_ec.o \
util_bignum_opti.o \
util_buffer.o \
util_convert.o \
util_hashmap.o \
util_xml.o \
vm_interpreter.o \
vm_malloc.o \
vm_memory.o \
vm_opcodes.o \
vm_term.o \
vm_thread.o \
vm_types.o )

EXTRA= $(addprefix $(OBJDIR)/, \
raspberry-pi.o \
raspberry-pi-hw.o \
raspberry-pi-mbox.o \
raspberry-pi-mmu.o \
raspberry-pi-shared-mem.o \
raspberry-pi-system.o \
raspberry-pi-uart.o \
)

OBJ= $(CORE) $(EXTRA)

CFLAGS = -O2 -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -Wno-strict-aliasing -fsigned-char

ifeq ($(UNAME),Darwin)
  MINIMACY := $(ROOT)/bin/minimacyMac
else
  MINIMACY := $(ROOT)/bin/minimacy
endif

TARGET_NAME= minimacyRpi4
TARGET_ELF= $(OBJDIR)/$(TARGET_NAME).elf
TARGET_IMG= $(ROOT)/bin/$(TARGET_NAME).img

help:
	@echo ""
	@echo "Welcome to bare-metal Raspberry Pi Minimacy! Bare-metal made easy!!!"
	@echo ""
	@echo "available targets"
	@echo "-----------------"
	@echo "make clean        : clean build directory"
	@echo "make romdisk      : build raspberry-pi-native-romdisk.h"
	@echo "make rpi4         : build $(TARGET_NAME).img for rpi4" 
	@echo "make uart         : start Bootdisk server & console" 
	@echo "make copy4        : copy $(TARGET_NAME).img on removable FAT32 storage" 

romdisk:
	$(MINIMACY) -d $(ROOT)/programs/romdisk/romdisk.native.rpi.mcy
	rm -f $(OBJDIR)/hw_fs_romdisk.o

uart:
	$(MINIMACY) $(ROOT)/programs/romdisk/romdisk.server.mcy

rpi4: CFLAGS:= $(CFLAGS) -D ON_RPI4
rpi4: rpi_common

$(OBJDIR)/raspberry-pi-start.o: raspberry-pi-start.S
	$(CC) $(CFLAGS) -c raspberry-pi-start.S -o $(OBJDIR)/raspberry-pi-start.o

rpi_common: $(OBJ) $(OBJDIR)/raspberry-pi-start.o 
	$(LD) -nostdlib $(OBJDIR)/raspberry-pi-start.o $(OBJ) -T raspberry-pi-link.ld -o $(TARGET_ELF)
	$(CP) -O binary $(TARGET_ELF) $(TARGET_IMG)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) 2>/dev/null

copy4: rpi4
	@echo ""
	@echo "Detect removable FAT32 volume (SD card or USB key) mounted on your system"
ifeq ($(UNAME),Darwin)
	@DST=$$(mount | sed -n 's/^.* on \(.*\) (msdos.*/\1/p' | head -n1); \
	 test -n "$$DST" || { echo "No FAT32 volume mounted"; exit 1; }; \
	 printf "Copy $(TARGET_NAME).img to %s? [Y/n] " "$$DST"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 cp $(TARGET_IMG) "$$DST/"; diskutil eject "$$DST"; \
	 echo "Done, card ejected"
else ifeq ($(IS_WSL),yes)
	@L=$$(powershell.exe -NoProfile -Command "Get-Volume | Where-Object FileSystem -eq FAT32 | Where-Object DriveType -eq Removable | Select -Expand DriveLetter" | tr -d '\r\n '); \
	 test -n "$$L" || { echo "No FAT32 removable drive"; exit 1; }; \
	 M=/mnt/$$(echo $$L | tr A-Z a-z); \
	 printf "Copy $(TARGET_NAME).img to %s:/ ? [Y/n] " "$$L"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 sudo mkdir -p $$M; \
	 mountpoint -q $$M || sudo mount -t drvfs $$L: $$M; \
	 mkdir -p $$M/EFI/BOOT; \
	 cp $(TARGET_IMG) $$M/EFI/BOOT/; \
	 sudo umount $$M; \
	 powershell.exe -NoProfile -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('$$L:').InvokeVerb('Eject')"; \
	 echo "Done, drive ejected"
else ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME)))
	@DRV=$$(powershell.exe -NoProfile -Command "Get-Volume | Where-Object FileSystem -eq FAT32 | Where-Object DriveType -eq Removable | Select -Expand DriveLetter" | tr -d '\r' | head -n1); \
	 test -n "$$DRV" || { echo "No removable FAT32 drive found"; exit 1; }; \
	 printf "Copy $(TARGET_NAME).img to %s:/ ? [Y/n] " "$$DRV"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 cp $(TARGET_IMG) "$$DRV:/"; \
	 powershell.exe -NoProfile -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('$$DRV:').InvokeVerb('Eject')"; \
	 echo "Done, drive ejected"
else
	@DST=$$(mount | sed -n 's/^.* on \(.*\) type vfat .*/\1/p' | head -n1); \
	 test -n "$$DST" || { echo "No FAT32 volume mounted"; exit 1; }; \
	 printf "Copy $(TARGET_NAME).img to %s? [Y/n] " "$$DST"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 cp $(TARGET_IMG) "$$DST/"; sync; \
	 DEV=$$(findmnt -no SOURCE "$$DST"); \
	 if command -v udisksctl >/dev/null 2>&1; then \
	   udisksctl unmount -b "$$DEV" >/dev/null 2>&1; \
	   PARENT=/dev/$$(lsblk -no pkname "$$DEV"); \
	   udisksctl power-off -b "$$PARENT" >/dev/null 2>&1 && echo "Ejected $$PARENT"; \
	 else \
	   sudo umount "$$DST" && echo "Unmounted (install udisks2 for safe power-off)"; \
	 fi
endif
