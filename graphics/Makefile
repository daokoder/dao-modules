PROJECT = dao_graphics

include ../../Makefile.common

DAO_INCS += -I../../kernel
DAO_LIBS += -L../..

# Build libdao_graphics target, so that other program or module can link against it:
TARGET = lib$(PROJECT).$(DAO_DLL_EXT)

ifdef WIN32
  TARGET = $(PROJECT).$(DAO_DLL_EXT)
endif

OBJECTS = dao_graphics.o dao_geometry.o dao_triangulator.o

all: $(TARGET)

dao_graphics.o: dao_graphics.c
	$(DAO_CC) -c $(DAO_CFLAGS) $(DAO_INCS) dao_graphics.c

dao_geometry.o: dao_geometry.c
	$(DAO_CC) -c $(DAO_CFLAGS) $(DAO_INCS) dao_geometry.c

dao_triangulator.o: dao_triangulator.c
	$(DAO_CC) -c $(DAO_CFLAGS) $(DAO_INCS) dao_triangulator.c
	
$(TARGET): $(OBJECTS)
	$(DAO_CC) $(DAO_DLLFLAGS) $(OBJECTS) $(DAO_LIBS) $(DAO_LFLAGS_DAO2) -o $(TARGET)

clean:
	-$(DEL_FILE) *.o
