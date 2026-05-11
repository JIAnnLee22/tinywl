#include "config.h"

#include "compositor.h"
#include "server.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-server-protocol.h>
#include <wlr/types/wlr_keyboard.h>

static const char default_config[] =
	"# tinywl — runtime binds (edit without recompiling)\n"
	"binds=ALT,Escape,quit,\n"
	"binds=ALT,F1,cycle_focus,\n"
	"binds=SUPER+SHIFT,F,set_layout,float\n"
	"binds=SUPER+SHIFT,S,set_layout,scroller\n"
	"axisbind=NONE,vertical,scroll_viewport,40\n";

void comp_ruleset_fini(struct comp_ruleset *r) {
	free(r->keys);
	free(r->axes);
	r->keys = NULL;
	r->axes = NULL;
	r->n_keys = 0;
	r->n_axes = 0;
}

static char *trim(char *s) {
	while (*s && isspace((unsigned char)*s)) {
		++s;
	}
	if (!*s) {
		return s;
	}
	char *e = s + strlen(s) - 1;
	while (e > s && isspace((unsigned char)*e)) {
		*e-- = '\0';
	}
	return s;
}

static uint32_t parse_mods(const char *s) {
	uint32_t m = 0;
	char buf[128];
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	for (char *tok = strtok(buf, "+"); tok; tok = strtok(NULL, "+")) {
		tok = trim(tok);
		if (strcasecmp(tok, "SUPER") == 0 || strcasecmp(tok, "LOGO") == 0) {
			m |= WLR_MODIFIER_LOGO;
		} else if (strcasecmp(tok, "CTRL") == 0 || strcasecmp(tok, "CONTROL") == 0) {
			m |= WLR_MODIFIER_CTRL;
		} else if (strcasecmp(tok, "ALT") == 0 || strcasecmp(tok, "MOD1") == 0) {
			m |= WLR_MODIFIER_ALT;
		} else if (strcasecmp(tok, "SHIFT") == 0) {
			m |= WLR_MODIFIER_SHIFT;
		} else if (strcasecmp(tok, "NONE") == 0) {
			/* no bits */
		}
	}
	return m;
}

static bool parse_action(const char *name, enum comp_action *out) {
	if (strcasecmp(name, "quit") == 0) {
		*out = COMP_ACTION_QUIT;
	} else if (strcasecmp(name, "cycle_focus") == 0) {
		*out = COMP_ACTION_CYCLE_FOCUS;
	} else if (strcasecmp(name, "scroll_viewport") == 0) {
		*out = COMP_ACTION_SCROLL_VIEWPORT;
	} else {
		return false;
	}
	return true;
}

static bool append_key(struct comp_ruleset *r, uint32_t mods, xkb_keysym_t sym,
		enum comp_action act, const char *arg) {
	struct comp_keybind *nk = realloc(r->keys, (r->n_keys + 1) * sizeof(*nk));
	if (!nk) {
		return false;
	}
	r->keys = nk;
	r->keys[r->n_keys].modifiers = mods;
	r->keys[r->n_keys].keysym = sym;
	r->keys[r->n_keys].action = act;
	r->keys[r->n_keys].arg[0] = '\0';
	if (arg) {
		strncpy(r->keys[r->n_keys].arg, arg, sizeof(r->keys[r->n_keys].arg) - 1);
	}
	r->n_keys++;
	return true;
}

static bool append_axis(struct comp_ruleset *r, uint32_t mods, uint32_t axis,
		enum comp_action act, double step) {
	struct comp_axisbind *na = realloc(r->axes, (r->n_axes + 1) * sizeof(*na));
	if (!na) {
		return false;
	}
	r->axes = na;
	r->axes[r->n_axes].modifiers = mods;
	r->axes[r->n_axes].axis = axis;
	r->axes[r->n_axes].action = act;
	r->axes[r->n_axes].step = step > 0 ? step : 40.0;
	r->n_axes++;
	return true;
}

static bool handle_line(struct comp_ruleset *r, char *line, int lineno, bool strict) {
	line = trim(line);
	if (!line[0] || line[0] == '#') {
		return true;
	}
	if (strncasecmp(line, "binds=", 6) == 0) {
		char *rest = line + 6;
		char *comma1 = strchr(rest, ',');
		if (!comma1) {
			goto bad;
		}
		*comma1 = '\0';
		char *comma2 = strchr(comma1 + 1, ',');
		if (!comma2) {
			goto bad;
		}
		*comma2 = '\0';
		char *comma3 = strchr(comma2 + 1, ',');
		char *arg = "";
		if (comma3) {
			*comma3 = '\0';
			arg = trim(comma3 + 1);
		}
		uint32_t mods = parse_mods(trim(rest));
		xkb_keysym_t sym = xkb_keysym_from_name(trim(comma1 + 1), XKB_KEYSYM_NO_FLAGS);
		if (sym == XKB_KEY_NoSymbol) {
			fprintf(stderr, "config line %d: unknown keysym \"%s\"\n", lineno, trim(comma1 + 1));
			if (strict) {
				return false;
			}
			return true;
		}
		enum comp_action act;
		const char *aname = trim(comma2 + 1);
		if (strcasecmp(aname, "set_layout") == 0) {
			if (strcasecmp(arg, "float") == 0) {
				act = COMP_ACTION_SET_LAYOUT_FLOAT;
			} else if (strcasecmp(arg, "scroller") == 0) {
				act = COMP_ACTION_SET_LAYOUT_SCROLLER;
			} else {
				fprintf(stderr, "config line %d: set_layout needs arg float|scroller\n", lineno);
				return strict ? false : true;
			}
			return append_key(r, mods, sym, act, arg);
		}
		if (!parse_action(aname, &act)) {
			fprintf(stderr, "config line %d: unknown action \"%s\"\n", lineno, aname);
			return strict ? false : true;
		}
		return append_key(r, mods, sym, act, arg);
	}
	if (strncasecmp(line, "axisbind=", 9) == 0) {
		char *rest = line + 9;
		char *comma1 = strchr(rest, ',');
		if (!comma1) {
			goto bad;
		}
		*comma1 = '\0';
		char *comma2 = strchr(comma1 + 1, ',');
		if (!comma2) {
			goto bad;
		}
		*comma2 = '\0';
		char *comma3 = strchr(comma2 + 1, ',');
		double step = 40;
		if (comma3) {
			*comma3 = '\0';
			step = strtod(trim(comma3 + 1), NULL);
		}
		uint32_t mods = parse_mods(trim(rest));
		const char *ax = trim(comma1 + 1);
		uint32_t axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
		if (strcasecmp(ax, "horizontal") == 0) {
			axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
		} else if (strcasecmp(ax, "vertical") != 0) {
			fprintf(stderr, "config line %d: axis must be vertical|horizontal\n", lineno);
			return strict ? false : true;
		}
		enum comp_action act;
		if (!parse_action(trim(comma2 + 1), &act) || act != COMP_ACTION_SCROLL_VIEWPORT) {
			fprintf(stderr, "config line %d: axisbind action must be scroll_viewport\n", lineno);
			return strict ? false : true;
		}
		return append_axis(r, mods, axis, act, step);
	}
	return true;
bad:
	fprintf(stderr, "config line %d: malformed\n", lineno);
	return strict ? false : true;
}

static bool load_from_fp(struct comp_ruleset *r, FILE *fp, bool strict) {
	char buf[1024];
	int lineno = 0;
	while (fgets(buf, sizeof buf, fp)) {
		lineno++;
		buf[sizeof(buf) - 1] = '\0';
		if (!handle_line(r, buf, lineno, strict)) {
			return false;
		}
	}
	return true;
}

bool comp_ruleset_load(struct comp_ruleset *out, const char *path, bool parse_only) {
	comp_ruleset_fini(out);
	FILE *fp = path ? fopen(path, "r") : NULL;
	bool ok;
	if (fp) {
		ok = load_from_fp(out, fp, parse_only);
		fclose(fp);
	} else {
		if (path) {
			fprintf(stderr, "warning: could not open config %s, using built-in defaults\n", path);
		}
		FILE *mem = fmemopen((void *)default_config, strlen(default_config), "r");
		if (!mem) {
			return false;
		}
		ok = load_from_fp(out, mem, parse_only);
		fclose(mem);
	}
	return ok;
}

bool comp_ruleset_dispatch_key(
	struct comp_ruleset *r, struct tinywl_server *server,
	uint32_t mods, xkb_keysym_t sym, bool pressed) {
	if (!pressed) {
		return false;
	}
	for (size_t i = 0; i < r->n_keys; i++) {
		struct comp_keybind *b = &r->keys[i];
		if (b->keysym != sym) {
			continue;
		}
		if ((mods & b->modifiers) != b->modifiers) {
			continue;
		}
		switch (b->action) {
		case COMP_ACTION_QUIT:
			tinywl_server_quit(server);
			return true;
		case COMP_ACTION_CYCLE_FOCUS:
			tinywl_server_cycle_focus(server);
			return true;
		case COMP_ACTION_SET_LAYOUT_FLOAT:
			tinywl_server_set_layout(server, LAYOUT_FLOAT);
			return true;
		case COMP_ACTION_SET_LAYOUT_SCROLLER:
			tinywl_server_set_layout(server, LAYOUT_SCROLLER);
			return true;
		default:
			break;
		}
	}
	return false;
}

bool comp_ruleset_dispatch_axis(
	struct comp_ruleset *r, struct tinywl_server *server,
	uint32_t mods, uint32_t axis, double delta) {
	if (server->layout_mode != LAYOUT_SCROLLER) {
		return false;
	}
	for (size_t i = 0; i < r->n_axes; i++) {
		struct comp_axisbind *b = &r->axes[i];
		if (b->axis != axis) {
			continue;
		}
		if ((mods & b->modifiers) != b->modifiers) {
			continue;
		}
		if (b->action == COMP_ACTION_SCROLL_VIEWPORT) {
			tinywl_server_scroll_viewport(server, delta > 0 ? b->step : -b->step);
			return true;
		}
	}
	return false;
}
