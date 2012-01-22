# change to your local directories!
PD_APP_DIR = /Applications/Pd-extended.app/Contents/Resources
PD_DIR = /Users/matthias/Pd-0.42.5-extended/pd
GEM_DIR = /Users/matthias/Gem-0.93.1
# build flags

INCLUDES = -I$(PD_DIR)/include
CPPFLAGS = -g -O2 -fPIC -freg-struct-return -Os -falign-loops=32 -falign-functions=32 -falign-jumps=32 -funroll-loops -ffast-math  -arch i386 -mmmx -fpascal-strings 

#linux doesnt work yet
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
 CPPFLAGS += -DLINUX
 INCLUDES += `pkg-config --cflags libfreenect`
 LDFLAGS =  -export_dynamic -shared
 LIBS = `pkg-config --libs libfreenect`
 EXTENSION = pd_linux
endif
ifeq ($(UNAME),Darwin)
 CPPFLAGS += -DDARWIN
 INCLUDES +=  -I/sw/include/libfreenect -I$(GEM_DIR)/src -I$(PD_DIR)/src -I$(PD_DIR) -I./
 LDFLAGS = -c -arch i386 
 LIBS =  -lm -lfreenect
 EXTENSION = pd_darwin
endif

.SUFFIXES = $(EXTENSION)

SOURCES = pix_freenect

all:
	g++ $(LDFLAGS) $(INCLUDES) $(CPPFLAGS) -o $(SOURCES).o -c $(SOURCES).cc
	g++ -o $(SOURCES).$(EXTENSION) -undefined dynamic_lookup -arch i386 -dynamiclib -mmacosx-version-min=10.3 -undefined dynamic_lookup -arch i386 ./*.o -L/sw/lib -lpthread -lfreenect -L$(PD_DIR)/bin -L$(PD_DIR)
	rm -fr ./*.o
deploy:
	mkdir build/$(SOURCES)
	./embed-MacOSX-dependencies.sh .
	mv *.dylib build/$(SOURCES)
	mv *.$(EXTENSION) build/$(SOURCES)
	cp *.pd build/$(SOURCES)
	rm -fr $(PD_APP_DIR)/extra/$(SOURCES)
	mv build/$(SOURCES) $(PD_APP_DIR)/extra/
clean:
	rm -f $(SOURCES)*.o
	rm -f $(SOURCES)*.$(EXTENSION)
distro: clean all
	rm *.o
