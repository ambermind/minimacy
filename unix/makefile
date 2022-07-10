#-- comment/uncomment the following lines to get a specific release
#X11=yes
#OPENGL=yes

#----------------------DO NOT TOUCH BELOW
I64=yes

export CC= gcc $(ARCH)
export CCP= g++ $(ARCH)
export AR= ar
export OPTIONS=
CCL=$(CC)

OBJDIR := ../obj


vpath %.c ../src
vpath %.h ../src

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
crypto_des.o \
crypto_md5.o \
crypto_rc4.o \
crypto_sha1.o \
crypto_sha2.o \
hw.o \
hw_thread.o \
main.o \
system_bignum.o \
system.o \
system_2d.o\
system_binary.o \
system_buffer.o \
system_bytes.o \
system_core.o \
system_convert.o \
system_crypto.o \
system_event.o \
system_file.o \
system_gzip.o \
system_lzw.o \
system_socket.o \
system_str.o \
system_tmp.o \
system_ui.o \
system_worker.o \
util.o \
util_2d.o \
util_buffer.o \
util_convert.o \
util_hashmap.o \
util_xml.o \
vm_interpreter.o \
vm_memory.o \
vm_opcodes.o \
vm_term.o \
vm_thread.o \
vm_types.o )

HOST_X11=$(addprefix $(OBJDIR)/, \
system_ui_x11.o )

HOST_GL=$(addprefix $(OBJDIR)/, \
system_ui_gl.o )

CFLAG=-O3 -D ON_UNIX

OBJ= $(CORE)
LIBS= -lm -lpthread

TARGET= ../bin/minimacy
ifeq ($(X11),yes)
	TARGET:= $(TARGET)X11
	OBJ:= $(OBJ) $(HOST_X11)
	LIBS:= $(LIBS) -lX11 -lXext
	CFLAG:=$(CFLAG) -D WITH_UI -D USE_X11
	LIBDIR=-L/usr/X11R6/lib
	INC=-I/usr/X11R6/include

ifeq ($(OPENGL),yes)
	TARGET:= $(TARGET)GL
	OBJ:= $(OBJ) $(HOST_GL)
	LIBS:= $(LIBS) -lGL -lGLU 
	CFLAG:=$(CFLAG) -D WITH_GLES2 -D GL_GLEXT_PROTOTYPES
endif
else
endif

ifeq ($(I64),yes)
else
	CFLAG:=$(CFLAG) -D ATOMIC_32
endif

export CFLAGS= $(CFLAG)

STATIC=../obj/zlib.a
ZLIBDIR=../src/libs/zlib

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(TARGET) : $(OBJ)
	@(cd $(ZLIBDIR) && $(MAKE) )
	$(CCL) $(OPTIONS) $(CFLAGS) -o $@ $^ $(STATIC) $(INC) $(LIBDIR) $(LIBS)
	@echo "-------------------------------"
	@echo "---> TARGET=" $(TARGET)
ifeq ($(I64),yes)
	@echo "---> 64 bits"
else
	@echo "---> 32 bits"
endif
ifeq ($(X11),yes)
	@echo "---> X11 version"
ifeq ($(OPENGL),yes)
	@echo "---> with OPENGL"
else
	@echo "---> WITHOUT opengl"
endif
else
	@echo "---> NOX version"
endif

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)


clean:
	rm -f $(TARGET)
	rm -f $(OBJDIR)/* 2>/dev/null

cleanall:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

