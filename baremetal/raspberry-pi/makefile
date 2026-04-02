# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2022, Sylvain Huet, Ambermind
# Minimacy (r) System


CC= aarch64-none-elf-gcc
CCLD=aarch64-none-elf-gcc
CCCP=aarch64-none-elf-objcopy

#INC = -IC:\home\dev\arm\arm-gnu-toolchain-11.3.rel1-mingw-w64-i686-aarch64-none-elf\aarch64-none-elf\include

ROOT:= ../..
OBJDIR := $(ROOT)/obj
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

CFLAGS = -O2 -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -Wno-strict-aliasing

TARGET= minimacyRpi4

help:
	@echo ""
	@echo "Welcome to bare-metal Rapsberry Pi Minimacy! Bare-metal made easy!!!"
	@echo ""
	@echo "available targets"
	@echo "-----------------"
	@echo "make rpi4         : build minimacyRpi4.img for rpi4" 
	@echo "make clean        : clean build directory"
	@echo "make copy-d4      : copy minimacyRpi4.img on D drive" 
	@echo "make copy-e4      : copy minimacyRpi4.img on E drive" 

rpi4: CFLAGS:= $(CFLAGS) -D ON_RPI4
rpi4: rpi_common

$(OBJDIR)/raspberry-pi-start.o: raspberry-pi-start.S
	$(CC) $(CFLAGS) -c raspberry-pi-start.S -o $(OBJDIR)/raspberry-pi-start.o

rpi_common: $(OBJ) $(OBJDIR)/raspberry-pi-start.o 
	$(CCLD) -nostdlib $(OBJDIR)/raspberry-pi-start.o $(OBJ) -T raspberry-pi-link.ld -o ../../bin/$(TARGET).elf
	$(CCCP) -O binary ../../bin/$(TARGET).elf ../../bin/$(TARGET).img

$(OBJDIR)/%.o: %.c raspberry-pi-native-romdisk.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) 2>/dev/null

copy-d4:
	cp ../../bin/minimacyRpi4.img d:/
copy-e4:
	cp ../../bin/minimacyRpi4.img e:/
