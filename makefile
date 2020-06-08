ifneq ($(OS_NAME),)
	TARGET_OS = $(OS_NAME)
else
	ifeq ($(OS),Windows_NT)
		TARGET_OS = Windows
	else ifeq ($(PLATFORM),iPhoneOS)
		TARGET_OS = iPhoneOS
	else ifeq ($(PLATFORM),iPhoneSimulator)
		TARGET_OS = iPhoneSimulator
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Darwin)
			TARGET_OS = Mac
		else
			TARGET_OS = Linux
		endif
	endif
endif

ifeq ($(TARGET_OS),Windows)
	IMPLIB   := libaergo-0.1
	LIBRARY  := libaergo-0.1.dll
	WINLIB   := $(IMPLIB).lib
	CFLAGS   := $(CFLAGS) -DMBEDTLS_PLATFORM_C
	LDFLAGS  := $(LDFLAGS) -static-libgcc -static-libstdc++
	WARNINGS = -Wno-pointer-sign -Wno-discarded-qualifiers
else ifeq ($(TARGET_OS),iPhoneOS)
	LIBRARY = libaergo.dylib
	CFLAGS += -fPIC -fvisibility=hidden
else ifeq ($(TARGET_OS),iPhoneSimulator)
	LIBRARY = libaergo.dylib
	CFLAGS += -fPIC -fvisibility=hidden
else
	ifeq ($(TARGET_OS),Mac)
		LIBRARY  = libaergo.0.dylib
		LIBNICK  = libaergo.dylib
		INSTNAME = $(LIBPATH)/$(LIBNICK)
		CURR_VERSION   = 0.1.0
		COMPAT_VERSION = 0.1.0
		WARNINGS = -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers
	else
		LIBRARY  = libaergo.so.0.1
		LIBNICK  = libaergo.so
		SONAME   = $(LIBNICK)
		WARNINGS = -Wno-pointer-sign -Wno-discarded-qualifiers
	endif
	IMPLIB   = aergo
	prefix  ?= /usr/local
	LIBPATH  = $(prefix)/lib
	INCPATH  = $(prefix)/include
	EXEPATH  = $(prefix)/bin
	CFLAGS  += -fPIC -fvisibility=hidden
endif

LDLIBS  = -lsecp256k1-vrf


CC    ?= gcc
STRIP ?= strip
AR    ?= ar


all:    $(LIBRARY) static

debug:  $(LIBRARY) static

debug:  export CFLAGS :=  -g -DDEBUG_MESSAGES  $(CFLAGS)

static: libaergo.a

ios:    libaergo.dylib static



aergo.o: account.c account.h aergo-int.h aergo.c aergo.h base58.c base58.h blockchain.pb.c blockchain.pb.h conversions.c endianess.c linked_list.c sha256.c socket.c mbedtls/bignum.c nanopb/pb_common.c nanopb/pb_encode.c nanopb/pb_decode.c
	$(CC) -c  aergo.c  $(CFLAGS) -Inanopb -std=c99 $(WARNINGS)


libaergo.a: aergo.o
	$(AR) rcs $@ $^


# Linux / Unix
libaergo.so.0.1: aergo.o
	$(CC) -shared -Wl,-soname,$(SONAME)  -o $@  $^  $(LDLIBS) $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) $@
endif
	ln -sf $(LIBRARY) $(LIBNICK)

# OSX
libaergo.0.dylib: aergo.o
	$(CC) -dynamiclib -install_name "$(INSTNAME)" -current_version $(CURR_VERSION) -compatibility_version $(COMPAT_VERSION)  -o $@  $^  $(LDLIBS) $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) -x $@
endif
	ln -sf $(LIBRARY) $(LIBNICK)

# iOS
libaergo.dylib: aergo.o
	$(CC) -dynamiclib  -o $@  $^  $(LDLIBS) $(LDFLAGS)
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) -x $@
endif

# Windows
libaergo-0.1.dll: aergo.o
	$(CC) -shared  -o $@  $^  -Wl,--out-implib,$(IMPLIB).lib $(LDLIBS) $(LDFLAGS) -lws2_32
ifeq ($(MAKECMDGOALS),valgrind)
else ifeq ($(MAKECMDGOALS),debug)
else
	$(STRIP) $@
endif


install:
	mkdir -p $(LIBPATH)
	cp $(LIBRARY) $(LIBPATH)
	cd $(LIBPATH) && ln -sf $(LIBRARY) $(LIBNICK)
	cp aergo.h $(INCPATH)
	cp aergo.hpp $(INCPATH)
ifeq ($(TARGET_OS),Linux)
	ldconfig
endif

clean:
	rm -f *.o libaergo.a $(LIBNICK) $(LIBRARY) $(WINLIB)
