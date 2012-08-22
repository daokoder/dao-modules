PROJECT = dao_graphics

include ../../Makefile.common

DAO_INCS += -I../../kernel
DAO_LIBS += -L../..

GL_INC =

# Build libdao_graphics target, so that other program or module can link against it:
TARGET = lib$(PROJECT).$(DAO_DLL_EXT)

UNAME = $(shell uname)

ifeq ($(UNAME), Linux)
  DAO_CFLAGS += -DUNIX
  DAO_LFLAGS += -fPIC -shared -Wl,-soname,lib$(PROJECT).so
  GL_INC += -I/usr/include/GL
  DAO_LFLAGS += -lGL -lGLU
  DAO_LFLAGS += -lGLUT
endif

ifeq ($(UNAME), Darwin)
  TARGET = lib$(PROJECT).dylib
  DAO_CFLAGS += -DUNIX -DMAC_OSX
  DAO_LFLAGS += -fPIC -dynamiclib -undefined dynamic_lookup -install_name lib$(PROJECT).dylib

  GL_INC += -I/System/Library/Frameworks/OpenGL.framework/Headers
  GL_INC += -I/System/Library/Frameworks/GLUT.framework/Headers
  DAO_LIBS += -framework OpenGL
  DAO_LIBS += -framework GLUT
endif

DAO_INCS += $(GL_INC)

OBJECTS = dao_graphics.o dao_geometry.o dao_path.o dao_triangulator.o dao_font.o
OBJECTS += dao_opengl.o
OBJECTS += dao_glut.o

all: $(TARGET)

.c.o:
	$(DAO_CC) -c $(DAO_CFLAGS) $(DAO_INCS) -o $@ $<

	
$(TARGET): $(OBJECTS)
	$(DAO_CC) $(DAO_DLLFLAGS) $(OBJECTS) $(DAO_LIBS) $(DAO_LFLAGS) -o $(TARGET)

clean:
	-$(DEL_FILE) *.o
