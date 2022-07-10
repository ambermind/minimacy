OBJDIR = ../../../obj/zlib
vpath %.c .
vpath %.h .

CORE= $(addprefix $(OBJDIR)/, \
mgzip.o adler32.o compress.o crc32.o deflate.o inffast.o inflate.o inftrees.o trees.o zutil.o )

TARGET= ../../../obj/zlib.a
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
