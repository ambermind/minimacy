# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2022, Sylvain Huet, Ambermind
# Minimacy (r) System

UNAME := $(shell uname -s)
IS_WSL := $(shell grep -qi microsoft /proc/version 2>/dev/null && echo yes)

CC  = x86_64-w64-mingw32-gcc
CCLD= x86_64-w64-mingw32-gcc
SHELL := /bin/bash

ROOT:= ../..
BINDIR := $(ROOT)/bin
OBJDIR := $(ROOT)/obj_uefi
IMGDIR := images
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
hw_fs_romdisk.o \
hw_fs_ansi.o \
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
uefi.o \
uefi-acpi.o \
uefi-eth.o \
uefi-hw.o \
uefi-mem.o \
uefi-path.o \
uefi-random.o \
uefi-pointer.o \
uefi-storage.o \
uefi-time.o \
uefi-uart.o \
)

OBJ= $(CORE) $(EXTRA)

CFLAGS = -ffreestanding -O2 -D ON_UEFI -fno-stack-check -fno-stack-protector -mno-stack-arg-probe -Wno-discarded-qualifiers
LFLAGS = -O2 -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main

ifeq ($(UNAME),Darwin)
  MINIMACY := $(ROOT)/bin/minimacyMac
else
  MINIMACY := $(ROOT)/bin/minimacy
endif

SGDISK := $(firstword $(wildcard /usr/sbin/sgdisk /sbin/sgdisk) sgdisk)

OVMF_FILE := $(firstword $(wildcard \
    /usr/share/ovmf/OVMF.fd \
    /usr/share/qemu/OVMF.fd \
    /usr/share/edk2/ovmf/OVMF.fd \
    /usr/share/OVMF/OVMF.fd) $(ROOT)/bin/OVMF.fd)

QEMU_MEMORY ?= 512M
QEMU = qemu-system-x86_64 -m $(QEMU_MEMORY) -bios $(OVMF_FILE) -cpu max -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0

ISODIR=$(OBJDIR)/iso

TARGET= $(BINDIR)/BOOTX64.EFI
TARGET_IMG= $(IMGDIR)/hd.img
TARGET_GPT= $(IMGDIR)/hd.gpt
TARGET_QCOW2= $(IMGDIR)/hd.qcow2
TARGET_ISO= $(IMGDIR)/cd.iso

SIZE ?= 36M
PORT1 ?= 8080
PORT2 ?= 8443
PORT3 ?= 2222
FORWARD_TEXT= (forward port: $(PORT1), $(PORT2), $(PORT3))

help:
	@echo ""
	@echo "Welcome to bare-metal UEFI Minimacy! Bare-metal made easy on PCs!!!"
	@echo ""
	@echo "(sudo apt install mtools gcc-mingw-w64-x86-64 qemu-system-x86 gdisk dnsmasq-base)"
	@echo "(brew install mtools mingw-w64 qemu coreutils gptfdisk)"
	@echo ""
	@echo "available targets (no sudo)"
	@echo "---------------------------"
	@echo "(3 tcp ports are forwarded, 8080, 8443, 2222, and may be forced with PORT1=1234, ...)"
	@echo "make clean              : clean build" 
	@echo "make clean-all          : clean build, img, gpt, qcow2 and iso" 
	@echo "make img [SIZE=36M]     : make empty hd.img, copy programs/* files. SIZE is optional, default $(SIZE)" 
	@echo "make romdisk            : build uefi-native-romdisk.h"
	@echo "make efi                : build BOOTX64.EFI, copy it into hd.img"
	@echo "make gpt                : make efi and build hd.gpt from hd.img" 
	@echo "make gce                : make gpt, rename as disk.raw and tar into minimacy-gce.tgz (Google Cloud Engine image)" 
	@echo "make qcow2              : make gpt, and build hd.qcow2"
	@echo "make iso                : make efi and build cd.iso from hd.img" 
	@echo "make run-img            : make efi, run qemu with hd.img $(FORWARD_TEXT)" 
	@echo "make run-img-display    : make efi, run qemu with hd.img and display $(FORWARD_TEXT)" 
	@echo "make run-gpt            : make efi, run qemu with hd.gpt $(FORWARD_TEXT)" 
	@echo "make run-gpt-display    : make efi, run qemu with hd.gpt and display $(FORWARD_TEXT)" 
	@echo "make copy               : make efi, copy BOOTX64.EFI and programs/* to removable storage" 
	@echo ""
	@echo "more targets if you need more networking (linux only)"
	@echo "-----------------------------------------------------"
	@echo "make tap-start          : setup a tap network with dhcp (192.168.2.1/24). Need: sudo apt install dnsmasq-base"
	@echo "make tap-stop           : remove the tap network"
	@echo "make run-img-tap        : make efi, run qemu with hd.img and tap network" 
	@echo "make run-img-display-tap: make efi, run qemu with hd.img, display and tap network" 
	@echo "make run-gpt-tap        : make efi, run qemu with hd.gpt and tap network" 
	@echo "make run-gpt-display-tap: make efi, run qemu with hd.gpt, display and tap network" 

img:
	rm -f $(TARGET_IMG)
	mkdir -p $(IMGDIR)
	truncate -s $(SIZE) $(TARGET_IMG)
	mformat -i $(TARGET_IMG) -F -v MINIMACY ::
	mmd -i $(TARGET_IMG) ::/EFI
	mmd -i $(TARGET_IMG) ::/EFI/BOOT
	shopt -s dotglob && mcopy -ospm -i $(TARGET_IMG) programs/* ::/
#	mcopy -ospm -i $(TARGET_IMG) programs ::/

romdisk:
	$(MINIMACY) -d $(ROOT)/programs/romdisk/romdisk.native.uefi.mcy
	rm -f $(OBJDIR)/hw_fs_romdisk.o

efi: $(OBJ) uefi-native-romdisk.h
	$(CCLD) $(LFLAGS) -o $(TARGET) $(OBJ)
	mcopy -o -i $(TARGET_IMG) $(TARGET) ::/EFI/BOOT
#	mattrib -i $(TARGET_IMG) +r ::/EFI/BOOT/$(notdir $(TARGET))

iso: efi
	mkdir -p $(ISODIR)
	cp -f $(TARGET_IMG) $(ISODIR)
	xorriso -as mkisofs -R -f -e $(notdir $(TARGET_IMG)) -no-emul-boot -o $(TARGET_ISO) $(ISODIR)

gpt: efi
	rm -f $(TARGET_GPT)
	SIZE=$$(( $$(wc -c < $(TARGET_IMG)) + 2*1024*1024 )); \
	dd if=/dev/zero of=$(TARGET_GPT) bs=1 count=0 seek=$$SIZE
	$(SGDISK) -n 1:2048:0 -t 1:ef00 $(TARGET_GPT)
	dd if=$(TARGET_IMG) of=$(TARGET_GPT) bs=512 seek=2048 conv=notrunc

gce: gpt
	cp $(TARGET_GPT) $(IMGDIR)/disk.raw
	tar --format=oldgnu -Sczf $(IMGDIR)/minimacy-gce.tar.gz $(IMGDIR)/disk.raw
	rm $(IMGDIR)/disk.raw

qcow2: gpt
	qemu-img convert -f raw -O qcow2 $(TARGET_GPT) $(TARGET_QCOW2)

tap-start:
	sudo bash tap_setup.sh up

tap-stop:
	sudo bash tap_setup.sh down

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) 2>/dev/null
	rm -f $(TARGET)

clean-all: clean
	rm -f $(TARGET_ISO)
	rm -f $(TARGET_GPT)
	rm -f $(TARGET_QCOW2)
	rm -f $(TARGET_IMG)

SERIAL_STDIO  = -serial stdio
DISPLAY_NONE  = -display none
DISPLAY_OPEN  = -device qemu-xhci -device usb-tablet -device usb-mouse
ETHERNET_TAP  = -device virtio-net-pci,netdev=n0 -netdev tap,id=n0,ifname=tap0,script=no,downscript=no
ETHERNET_SLIRP= -nic user,model=virtio-net-pci,hostfwd=tcp::$(PORT1)-:$(PORT1),hostfwd=tcp::$(PORT2)-:$(PORT2),hostfwd=tcp::$(PORT3)-:$(PORT3)

run-img: efi
	$(QEMU) -hda $(TARGET_IMG) $(SERIAL_STDIO) $(DISPLAY_NONE) $(ETHERNET_SLIRP)

run-img-display: efi
	$(QEMU) -hda $(TARGET_IMG) $(SERIAL_STDIO) $(DISPLAY_OPEN) $(ETHERNET_SLIRP)

run-img-tap: efi
	$(QEMU) -hda $(TARGET_IMG) $(SERIAL_STDIO) $(DISPLAY_NONE) $(ETHERNET_TAP)

run-img-display-tap: efi
	$(QEMU) -hda $(TARGET_IMG) $(SERIAL_STDIO) $(DISPLAY_OPEN) $(ETHERNET_TAP)

run-iso: iso
	$(QEMU) -cdrom $(TARGET_ISO) $(SERIAL_STDIO) $(DISPLAY_NONE) $(ETHERNET_SLIRP)

run-gpt: gpt
	$(QEMU) -hda $(TARGET_GPT) $(SERIAL_STDIO) $(DISPLAY_NONE) $(ETHERNET_SLIRP)

run-gpt-display: gpt
	$(QEMU) -hda $(TARGET_GPT) $(SERIAL_STDIO) $(DISPLAY_OPEN) $(ETHERNET_SLIRP)

run-gpt-tap: gpt
	$(QEMU) -hda $(TARGET_GPT) $(SERIAL_STDIO) $(DISPLAY_NONE) $(ETHERNET_TAP)

run-gpt-display-tap: gpt
	$(QEMU) -hda $(TARGET_GPT) $(SERIAL_STDIO) $(DISPLAY_OPEN) $(ETHERNET_TAP)

copy: efi
	@echo ""
	@echo "Detect removable FAT32 volume (USB key) mounted on your system"
ifeq ($(UNAME),Darwin)
	@DST=$$(mount | sed -n 's/^.* on \(.*\) (msdos.*/\1/p' | head -n1); \
	 test -n "$$DST" || { echo "No FAT32 volume mounted"; exit 1; }; \
	 printf "Copy files to %s? [Y/n] " "$$DST"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 mkdir -p "$$DST/EFI/BOOT"; \
	 cp -f $(TARGET) "$$DST/EFI/BOOT/"; \
	 cp -rf programs/. "$$DST"; \
	 diskutil eject "$$DST"; \
	 echo "Done, card ejected"
else ifeq ($(IS_WSL),yes)
	@L=$$(powershell.exe -NoProfile -Command "Get-Volume | Where-Object FileSystem -eq FAT32 | Where-Object DriveType -eq Removable | Select -Expand DriveLetter" | tr -d '\r\n '); \
	 test -n "$$L" || { echo "No FAT32 removable drive"; exit 1; }; \
	 M=/mnt/$$(echo $$L | tr A-Z a-z); \
	 printf "Copy files to %s:/ ? [Y/n] " "$$L"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 sudo mkdir -p $$M; \
	 mountpoint -q $$M || sudo mount -t drvfs $$L: $$M; \
	 mkdir -p $$M/EFI/BOOT; \
	 cp $(TARGET) $$M/EFI/BOOT/; \
	 cp -rf programs/. $$M; \
	 sudo umount $$M; \
	 powershell.exe -NoProfile -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('$$L:').InvokeVerb('Eject')"; \
	 echo "Done, drive ejected"
else ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME)))
	@DRV=$$(powershell.exe -NoProfile -Command "Get-Volume | Where-Object FileSystem -eq FAT32 | Where-Object DriveType -eq Removable | Select -Expand DriveLetter" | tr -d '\r' | head -n1); \
	 test -n "$$DRV" || { echo "No removable FAT32 drive found"; exit 1; }; \
	 printf "Copy files to %s:/ ? [Y/n] " "$$DRV"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 mkdir -p "$$DRV:/EFI/BOOT"; \
	 cp -f $(TARGET) "$$DRV:/EFI/BOOT; \
	 cp -rf programs/. "$$DRV:/"; \
	 powershell.exe -NoProfile -Command "(New-Object -comObject Shell.Application).Namespace(17).ParseName('$$DRV:').InvokeVerb('Eject')"; \
	 echo "Done, drive ejected"
else
	@DST=$$(mount | sed -n 's/^.* on \(.*\) type vfat .*/\1/p' | head -n1); \
	 test -n "$$DST" || { echo "No FAT32 volume mounted"; exit 1; }; \
	 printf "Copy files to %s? [Y/n] " "$$DST"; read a < /dev/tty; \
	 case "$$a" in [nN]*) echo "Cancelled"; exit 1;; esac; \
	 mkdir -p "$$DST/EFI/BOOT"; \
	 cp -f $(TARGET) "$$DST/EFI/BOOT/"; \
	 cp -rf programs/. "$$DST"; sync; \
	 DEV=$$(findmnt -no SOURCE "$$DST"); \
	 if command -v udisksctl >/dev/null 2>&1; then \
	   udisksctl unmount -b "$$DEV" >/dev/null 2>&1; \
	   PARENT=/dev/$$(lsblk -no pkname "$$DEV"); \
	   udisksctl power-off -b "$$PARENT" >/dev/null 2>&1 && echo "Ejected $$PARENT"; \
	 else \
	   sudo umount "$$DST" && echo "Unmounted (install udisks2 for safe power-off)"; \
	 fi
endif

