#pragma once

#include "layout.h"

struct tinywl_server;
struct tinywl_toplevel;

void tinywl_server_quit(struct tinywl_server *server);
/** Run `cmd` via `/bin/sh -c` (double-forked); inherits environment (e.g. WAYLAND_DISPLAY). */
void tinywl_server_spawn_sh(const char *cmd);
void tinywl_server_cycle_focus(struct tinywl_server *server);
void tinywl_server_set_layout(struct tinywl_server *server, enum layout_mode mode);
void tinywl_server_scroll_viewport(struct tinywl_server *server, double delta);
