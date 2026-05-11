#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

struct tinywl_server;

enum comp_action {
	COMP_ACTION_NONE = 0,
	COMP_ACTION_QUIT,
	COMP_ACTION_CYCLE_FOCUS,
	COMP_ACTION_SET_LAYOUT_FLOAT,
	COMP_ACTION_SET_LAYOUT_SCROLLER,
	COMP_ACTION_SCROLL_VIEWPORT,
	/** Run `arg` with `/bin/sh -c` (double-forked); see share/tinywl/config.conf */
	COMP_ACTION_EXEC,
};

struct comp_keybind {
	uint32_t modifiers;
	xkb_keysym_t keysym;
	enum comp_action action;
	char arg[512];
};

struct comp_axisbind {
	uint32_t modifiers;
	/** `WL_POINTER_AXIS_VERTICAL` or `WL_POINTER_AXIS_HORIZONTAL` */
	uint32_t axis;
	enum comp_action action;
	double step;
};

struct comp_ruleset {
	struct comp_keybind *keys;
	size_t n_keys;
	struct comp_axisbind *axes;
	size_t n_axes;
};

bool comp_ruleset_load(struct comp_ruleset *out, const char *path, bool parse_only);
void comp_ruleset_fini(struct comp_ruleset *r);

bool comp_ruleset_dispatch_key(
	struct comp_ruleset *r, struct tinywl_server *server,
	uint32_t mods, xkb_keysym_t sym, bool pressed);

bool comp_ruleset_dispatch_axis(
	struct comp_ruleset *r, struct tinywl_server *server,
	uint32_t mods, uint32_t axis, double delta);
