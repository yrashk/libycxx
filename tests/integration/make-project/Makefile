# A plain Makefile project (tests/integration/run.sh builds it two ways):
#   make CXX=<prefix>/bin/ycxx-c++ CC=<prefix>/bin/ycxx-cc        libycxx's compilers
#   make CXX=g++-16 CC=gcc-16 PKG=libycxx                          the plain compiler and pkg-config
# (PKG_CONFIG_PATH must find libycxx.pc: <prefix>/lib/pkgconfig).
CXX ?= c++
CC ?= cc
CXXFLAGS ?= -O2 -std=c++17
CFLAGS ?= -O2
PKG ?=
ifneq ($(PKG),)
PKG_CFLAGS := $(shell pkg-config --cflags $(PKG))
PKG_LIBS := $(shell pkg-config --libs $(PKG))
endif

app: main.o util.o cpart.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(PKG_LIBS) -pthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PKG_CFLAGS) -pthread -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f app *.o
.PHONY: clean
