PKG_CONFIG?=pkg-config
WAYLAND_PROTOCOLS!=$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER!=$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS="scenefx-0.4" "wlroots-0.19" wayland-server xkbcommon glesv2 pixman-1
CFLAGS_PKG_CONFIG!=$(PKG_CONFIG) --cflags $(PKGS)
CFLAGS+=$(CFLAGS_PKG_CONFIG) -std=c11 -Wall -Wextra -D_GNU_SOURCE -Isrc -Iprotocol
LIBS!=$(PKG_CONFIG) --libs $(PKGS)

all: tinywl

protocol/xdg-shell-protocol.h:
	@mkdir -p protocol
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

tinywl.o: tinywl.c protocol/xdg-shell-protocol.h
	$(CC) -c $< -g $(CFLAGS) -DWLR_USE_UNSTABLE -o $@
tinywl: tinywl.o
	$(CC) -g $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f tinywl tinywl.o protocol/xdg-shell-protocol.h

.PHONY: all clean
