PREFIX ?= /usr/local
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

.PHONY: all clean test-nested

# Nested compositor + kitty + automated layout commands (needs active Wayland session + kitty).
test-nested:
	bash scripts/nested-layout-test.sh
