OBJDIR = ../../../obj/jpglib
vpath %.c .
vpath %.h .

CORE= $(addprefix $(OBJDIR)/, \
mjpeg.o jcapimin.o jcapistd.o jccoefct.o jccolor.o jcdctmgr.o jchuff.o jcinit.o jcmainct.o jcmarker.o jcmaster.o jcomapi.o jcparam.o jcphuff.o jcprepct.o jcsample.o jctrans.o jdapimin.o jdapistd.o jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o jdinput.o jdmainct.o jdmarker.o jdmaster.o jdmerge.o jdphuff.o jdpostct.o jdsample.o jdtrans.o jerror.o jfdctflt.o jfdctfst.o jfdctint.o jidctflt.o jidctfst.o jidctint.o jidctred.o jmemmgr.o jmemnobs.o jquant1.o jquant2.o jutils.o)


TARGET= ../../../obj/jpglib.a
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

