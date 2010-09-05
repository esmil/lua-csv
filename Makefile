CC      = gcc
CFLAGS  = -march=native -O2 -Wall -Wextra -pipe -pedantic
STRIP   = strip
INSTALL = install

PREFIX = /usr/local

LUA_DIR = $(PREFIX)
LUA_SHAREDIR=$(LUA_DIR)/share/lua/5.1
LUA_LIBDIR=$(LUA_DIR)/lib/lua/5.1

ifdef NDEBUG
DEFINES+=-DNDEBUG
endif

clibs = csv

.PHONY: all strip clean
.PRECIOUS: %.pic.o

all: $(clibs:%=%.so)

%.pic.o: %.c
	@echo '  CC $@'
	@$(CC) $(CFLAGS) -fPIC -nostartfiles -I'$(LUA_SHAREDIR)' $(DEFINES) -c $< -o $@

%.so: %.pic.o
	@echo '  LD $@'
	@$(CC) $(CFLAGS) $(LDFLAGS) -shared -L'$(LUA_LIBDIR)' -llua $^ -o $@

install: csv.so csv.lua
	@echo '  INSTALL -d $(DESTDIR)$(LUA_LIBDIR)'
	@$(INSTALL) -d $(DESTDIR)$(LUA_LIBDIR)
	@echo '  INSTALL -d $(DESTDIR)$(LUA_LIBDIR)/csv'
	@$(INSTALL) -d $(DESTDIR)$(LUA_LIBDIR)/csv
	@echo '  INSTALL csv.lua'
	@$(INSTALL) csv.lua $(DESTDIR)$(LUA_LIBDIR)
	@echo '  INSTALL csv_core.so'
	@$(INSTALL) csv.so $(DESTDIR)$(LUA_LIBDIR)/csv/core.so

strip_%: %.so
	@echo '  STRIP $<'
	@$(STRIP) $<

strip: $(clibs:%=strip_%)

clean:
	rm -f $(clibs:%=%.so) *.o *.c~ *.h~
