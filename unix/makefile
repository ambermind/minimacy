#----------------------DO NOT TOUCH BELOW
export CC= gcc
export AR= ar
CCL=$(CC)

ROOT:= ..
OBJDIR := $(ROOT)/obj
BINDIR := $(ROOT)/bin
TARGET := $(BINDIR)/minimacy

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

CFLAG=-O3 -D ON_UNIX

OBJ= $(CORE)
LIBS= -lm -lpthread

TARGETX11GL= $(TARGET)X11GL
LIBSX11GL  = $(LIBS) -lX11 -lXext -lasound -lGL -lGLU 
CFLAGX11GL = -O3 -D ON_UNIX_X11GL
LIBDIRX11GL= -L/usr/X11R6/lib
INCX11GL   = -I/usr/X11R6/include

TARGETRPIOS= $(TARGET)RPiOS64

TARGETBM=$(TARGET)bm
CFLAGBM = -O3 -D ON_UNIX_BM

help:
	@echo ""
	@echo "available targets"
	@echo "-----------------"
	@echo "make nox          : build non graphic release, like a server" 
	@echo "make x11gl        : build X11/openGL release"
	@echo "make all          : build nox and x11gl targets"
	@echo "make pi           : build Raspberry Pi OS 64 bits release"
	@echo "make bm           : build baremetal target"
	@echo "make romdisk      : build baremetal romdisk"
	@echo "make clean        : clean build directory" 
	@echo "make clean-nox    : clean nox target" 
	@echo "make clean-x11gl  : clean x11gl target"
	@echo "make clean-pi     : clean pi target"
	@echo "make clean-bm     : clean baremetal target"
	@echo "make clean-all    : clean all targets"

nox: $(TARGET)
x11gl: CFLAG:= $(CFLAGX11GL)
x11gl: INC:= $(INCX11GL)
x11gl: $(TARGETX11GL)

all:
	make clean-all
	make nox
	make clean-x11gl
	make x11gl

pi: CFLAG:= -O3 -D ON_RPIOS
pi: INC:= $(INCX11GL)
pi: $(TARGETRPIOS)

romdisk:
	../bin/minimacy ../baremetal/nothing/romdiskNothingMaker.mcy
	rm -f $(OBJDIR)/hw_fs_romdisk.o

bm: $(TARGETBM)
bm: CFLAG:= $(CFLAGBM)

$(TARGET) : $(OBJ)
	$(CCL) -o $@ $^ $(INC) $(LIBDIR) $(LIBS)
	@echo "Done --> " $(TARGET)

$(TARGETX11GL) : $(OBJ)
	$(CCL) -o $@ $^ $(INCX11GL) $(LIBDIRX11GL) $(LIBSX11GL)
	@echo "Done --> " $(TARGETX11GL)

$(TARGETRPIOS): $(OBJ)
	$(CCL) -o $@ $^ $(INCX11GL) $(LIBDIRX11GL) $(LIBSX11GL)
	@echo "Done --> " $(TARGETRPIOS)

$(TARGETBM) : $(OBJ)
	$(CCL) -o $@ $^ $(INC) $(LIBDIR) $(LIBS)
	@echo "Done --> " $(TARGETBM)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAG) $(INC) -c $< -o $@

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) 2>/dev/null

clean-nox: clean
	rm -rf $(TARGET)

clean-x11gl:clean 
	rm -rf $(TARGETX11GL)

clean-pi: clean
	rm -rf $(TARGETRPIOS)

clean-bm: clean
	rm -rf $(TARGETBM)

clean-all: clean clean-nox clean-x11gl clean-pi clean-bm
