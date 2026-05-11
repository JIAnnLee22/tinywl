#include "layout.h"

#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>

#include <math.h>
#include <wayland-server-core.h>

#include "server.h"

#define SCROLL_GAP 4

void layout_arrange(struct tinywl_server *server) {
	if (server->layout_mode != LAYOUT_SCROLLER) {
		return;
	}

	struct wlr_box usable = {0};
	struct tinywl_output *out;
	wl_list_for_each(out, &server->outputs, link) {
		usable.width = out->wlr_output->width;
		usable.height = out->wlr_output->height;
		break;
	}
	if (usable.width <= 0 || usable.height <= 0) {
		return;
	}

	double y = -server->scroll_viewport_offset;
	struct tinywl_toplevel *toplevel;
	wl_list_for_each(toplevel, &server->toplevels, link) {
		if (!toplevel->xdg_toplevel->base->surface->mapped) {
			continue;
		}
		struct wlr_box *geo = &toplevel->xdg_toplevel->base->geometry;
		int width = usable.width;
		int height = geo->height;
		if (width < 1 || height < 1) {
			continue;
		}
		wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, width, height);
		wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, (int)round(y));
		y += height + SCROLL_GAP;
	}
}
