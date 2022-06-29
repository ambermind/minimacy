OBJDIR = ../../../obj/pnglib
vpath %.c .
vpath %.h .

CORE= $(addprefix $(OBJDIR)/, \
mpng.o mgzip.o png.o pngerror.o pnggccrd.o pngget.o pngmem.o pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o pngtrans.o pngvcrd.o pngwio.o pngwrite.o pngwtran.o pngwutil.o\
adler32.o compress.o crc32.o deflate.o infback.o inffast.o inflate.o inftrees.o trees.o zutil.o )

TARGET= ../../../obj/pnglib.a
OBJ= $(CORE)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCC) -c $< -o $@

$(TARGET) : $(OBJ)
	$(AR) $(OPTIONS) -crv $@ $^
	ranlib $(TARGET)

$(OBJ): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

clean :
	rm -f *.o
	rm -f $(TARGET)
