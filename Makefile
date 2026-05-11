PREFIX ?= /usr/local
DESTDIR ?=
PKG_CONFIG ?= pkg-config
WAYLAND_PROTOCOLS := $(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)

CFLAGS += -std=c11 -Wall -Wextra -D_GNU_SOURCE -DWLR_USE_UNSTABLE
CFLAGS += -Iprotocol
CFLAGS += $(shell $(PKG_CONFIG) --cflags wlroots-0.19 scenefx-0.4 wayland-server xkbcommon glesv2 pixman-1)

LIBS = $(shell $(PKG_CONFIG) --libs wlroots-0.19 scenefx-0.4 wayland-server xkbcommon glesv2 pixman-1)

PROTOCOLS = protocol/xdg-shell-protocol.h
SRCS = src/compositor.c src/config.c src/layout.c
OBJS = $(SRCS:.c=.o)

all: tinywl

protocol/xdg-shell-protocol.h: $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml
	@mkdir -p protocol
	wayland-scanner server-header $< $@

tinywl: $(PROTOCOLS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

src/%.o: src/%.c $(PROTOCOLS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f tinywl $(OBJS) $(PROTOCOLS)

install: tinywl share/tinywl/config.conf
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m755 tinywl "$(DESTDIR)$(PREFIX)/bin/tinywl"
	install -d "$(DESTDIR)$(PREFIX)/share/tinywl"
	install -m644 share/tinywl/config.conf "$(DESTDIR)$(PREFIX)/share/tinywl/config.conf"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/tinywl"
	rm -f "$(DESTDIR)$(PREFIX)/share/tinywl/config.conf"

.PHONY: all clean install uninstall test-nested

# Nested compositor + multiple Kitty windows + FIFO-driven scroller/float tests.
# Optional: TINYWL_TEST_KITTY_COUNT (default 5), TINYWL_TEST_USE_NIX=0.
test-nested:
	bash scripts/nested-layout-test.sh
