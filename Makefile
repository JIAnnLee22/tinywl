PKG_CONFIG?=pkg-config
WAYLAND_PROTOCOLS!=$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER!=$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS="scenefx-0.4" "wlroots-0.19" wayland-server xkbcommon glesv2 pixman-1
CFLAGS_PKG_CONFIG!=$(PKG_CONFIG) --cflags $(PKGS)
CFLAGS+=$(CFLAGS_PKG_CONFIG) -std=c11 -Wall -Wextra -D_GNU_SOURCE -Isrc -Iprotocol
LIBS!=$(PKG_CONFIG) --libs $(PKGS)

PROTOCOL_H=protocol/xdg-shell-protocol.h
WM_OBJS=src/compositor.o src/config.o src/layout.o

# Default: full compositor from src/ (same scenefx patterns as tinywl.c sample).
all: tinywl

$(PROTOCOL_H):
	@mkdir -p protocol
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

src/%.o: src/%.c $(PROTOCOL_H)
	$(CC) -c $< -g $(CFLAGS) -DWLR_USE_UNSTABLE -o $@

tinywl: $(WM_OBJS)
	$(CC) -g $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Single-file scenefx upstream-style sample (root tinywl.c), for API comparison.
tinywl-sample.o: tinywl.c $(PROTOCOL_H)
	$(CC) -c $< -g $(CFLAGS) -DWLR_USE_UNSTABLE -o $@
tinywl-sample: tinywl-sample.o
	$(CC) -g $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f tinywl tinywl-sample $(WM_OBJS) tinywl-sample.o $(PROTOCOL_H)

.PHONY: all clean
