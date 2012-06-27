#
# Atlante test makefile
# Author: Ignacio Mellado
# Last revision: 11/11/2010
#
# IMPORTANT: If you are a makefile rookie and you want to customize this one for your own
# tests, please just modify variables in the area marked as "customizable parameters".
#
# Description: 
# It generates the test executable from the compilation of all valid source files 
# in the source directory. Valid sources have any of the extensions: c, cpp, cc, c++.
# Each source depends on the framework library and on all the valid header files.
# Valid header files are located in the include path and have any of the extensions: 
# h, hpp, h++.
#
# The followig targets are defined:
#   all - It compiles all source files and links them to generate the executable.
#   clean - It deletes the executable and all compiled objects.
#

# Windows OS detection
_WINNAME := $(shell set OS)
GOTWINDOWS = $(patsubst OS=%,yes,$(_WINNAME))

# --------- Start of customizabe parameters ---------
# The name of the executable. On Windows systems, the extension ".exe" will be appended.
EXENAME := test
# The path on which all paths are referenced
ROOTPATH := .
# The path of the framework library
ATLANTE_ROOTPATH := $(ROOTPATH)/../../../atlante
ATLANTE_INCLUDEPATH := $(ATLANTE_ROOTPATH)
ATLANTE_LIBPATH = $(ATLANTE_ROOTPATH)/lib/gcc
# The path where the source files reside
SOURCEPATH := $(ROOTPATH)/..
# The path where the header files are
INCLUDEPATH := $(ROOTPATH)
# Where to save binaries in general
BINARIESPATH := $(ROOTPATH)
# Where to save the compiled objects
OBJPATH := $(BINARIESPATH)/obj
# Where to save the executable
EXEPATH := $(BINARIESPATH)

# Compiler to be used
CC := g++
# Compilation options
CFLAGS := -c -O
# Include parameters
INCLUDES := -I/usr/include -I$(ATLANTE_INCLUDEPATH)
# Linking parameters
ifeq ($(GOTWINDOWS), yes)
	# This works for mingw on Windows
	LDFLAGS := -L$(ATLANTE_LIBPATH) -latlante -Wl,--enable-auto-import
else
	# This works for Linux
	LDFLAGS := -L$(ATLANTE_LIBPATH) -L/usr/lib -latlante -lc -lpthread
endif
# ---------- End of customizable parameters ----------

# Allowed source and header extensions.
# The modification of this list is not enough to add/delete valid extensions. 
# Some lines below implement actions based on extensions and do not use this variables.
SOURCE_EXTENSIONS := cpp c cc c++
HEADER_EXTENSIONS := h h++ hpp

#vpath $(foreach EXT,$(SOURCE_EXTENSIONS),%%.$(EXT)) $(SOURCEPATH)
#vpath $(foreach EXT,$(HEADER_EXTENSIONS),%%.$(EXT)) $(INCLUDEPATH)
#vpath $(foreach EXT,$(HEADER_EXTENSIONS),%%.$(EXT)o) $(OBJPATH)
vpath %.cpp $(SOURCEPATH)
vpath %.c $(SOURCEPATH)
vpath %.c++ $(SOURCEPATH)
vpath %.cc $(SOURCEPATH)
vpath %.h $(INCLUDEPATH)
vpath %.hpp $(INCLUDEPATH)
vpath %.h++ $(INCLUDEPATH)
vpath %.cppo $(OBJPATH)
vpath %.co $(OBJPATH)
vpath %.cco $(OBJPATH)
vpath %.c++o $(OBJPATH)

SOURCE_MASKS_SEARCH := $(addprefix *., $(SOURCE_EXTENSIONS))
HEADER_MASKS_SEARCH := $(addprefix *., $(HEADER_EXTENSIONS))
#TARGET_MASKS := $(foreach EXT,$(SOURCE_EXTENSIONS),.$(EXT).o)

SOURCES:=$(wildcard $(addprefix $(SOURCEPATH)/,$(SOURCE_MASKS_SEARCH)))
HEADERS:=$(wildcard $(addprefix $(INCLUDEPATH)/,$(HEADER_MASKS_SEARCH)))

OBJECTS := $(strip $(foreach EXT,$(SOURCE_EXTENSIONS),$(patsubst %.$(EXT),%.$(EXT)o,$(filter %.$(EXT),$(notdir $(SOURCES))))))
OBJECTSWITHPATH := $(addprefix $(OBJPATH)/,$(OBJECTS))

ifeq ($(GOTWINDOWS), yes)
	EXECUTABLE := $(EXEPATH)/$(EXENAME).exe
else
	EXECUTABLE := $(EXEPATH)/$(EXENAME)
endif

.PHONY: all clean

all:  $(EXECUTABLE)

clean:
	@echo Cleaning objects...
ifeq ($(GOTWINDOWS), yes)
	@del /Q $(subst /,\,$(OBJECTSWITHPATH))
	@del /Q $(subst /,\,$(EXECUTABLE))
else
	@rm -f $(OBJECTSWITHPATH)
	@rm -f $(EXECUTABLE)
endif

$(EXECUTABLE): $(ATLANTE_LIBPATH)/libatlante.a $(OBJECTS)
	@echo Linking [$@]...	
	@$(CC) $(OBJECTSWITHPATH) -o $(EXECUTABLE) $(LDFLAGS)

#$(TARGET_MASKS): $(HEADERS) $(ATLANTE_INCLUDEPATH)/atlante.h

UNIVERSAL_DEPENDENCIES := $(HEADERS) $(ATLANTE_INCLUDEPATH)/atlante.h

%.co: %.c $(UNIVERSAL_DEPENDENCIES)
	@echo Compiling [$<]...
	@$(CC) $(CFLAGS) $< -o $(OBJPATH)/$(notdir $@) $(INCLUDES)

%.cppo: %.cpp $(UNIVERSAL_DEPENDENCIES)
	@echo Compiling [$<]...
	@$(CC) $(CFLAGS) $< -o $(OBJPATH)/$(notdir $@) $(INCLUDES)

%.cco: %.cc $(UNIVERSAL_DEPENDENCIES)
	@echo Compiling [$<]...
	@$(CC) $(CFLAGS) $< -o $(OBJPATH)/$(notdir $@) $(INCLUDES)

%.c++o: %.c++ $(UNIVERSAL_DEPENDENCIES)
	@echo Compiling [$<]...
	@$(CC) $(CFLAGS) $< -o $(OBJPATH)/$(notdir $@) $(INCLUDES)

$(ATLANTE_LIBPATH)/libatlante.a:
ifeq ($(GOTWINDOWS), yes)
	@gmake -C $(ATLANTE_ROOTPATH) all
else
	@make -C $(ATLANTE_ROOTPATH) all
endif

