#include "layout.h"

#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>

#include <math.h>
#include <wayland-server-core.h>

#include "server.h"

#define SCROLL_GAP 4
#define FLOAT_CASCADE_STEP 24

void layout_arrange(struct tinywl_server *server) {
	if (server->layout_mode == LAYOUT_FLOAT) {
		struct wlr_box layout_box;
		wlr_output_layout_get_box(server->output_layout, NULL, &layout_box);
		if (layout_box.width <= 0 || layout_box.height <= 0) {
			return;
		}

		struct tinywl_toplevel *toplevel;
		wl_list_for_each(toplevel, &server->toplevels, link) {
			if (!toplevel->xdg_toplevel->base->surface->mapped) {
				continue;
			}
			if (toplevel->float_user_positioned) {
				continue;
			}
			struct wlr_box *geo = &toplevel->xdg_toplevel->base->geometry;
			int outer_w = geo->width + 2 * TINYWL_BORDER_THICKNESS;
			int outer_h = geo->height + 2 * TINYWL_BORDER_THICKNESS;
			if (outer_w < 1 || outer_h < 1) {
				continue;
			}

			int before = 0;
			struct tinywl_toplevel *other;
			wl_list_for_each(other, &server->toplevels, link) {
				if (!other->xdg_toplevel->base->surface->mapped) {
					continue;
				}
				if (other->float_user_positioned) {
					continue;
				}
				if (other->float_place_serial < toplevel->float_place_serial) {
					before++;
				}
			}

			double cx = layout_box.x + (layout_box.width - outer_w) * 0.5 +
				TINYWL_BORDER_THICKNESS;
			double cy = layout_box.y + (layout_box.height - outer_h) * 0.5 +
				TINYWL_BORDER_THICKNESS;
			cx += before * FLOAT_CASCADE_STEP;
			cy += before * FLOAT_CASCADE_STEP;

			double min_x = layout_box.x + TINYWL_BORDER_THICKNESS;
			double min_y = layout_box.y + TINYWL_BORDER_THICKNESS;
			double max_x = layout_box.x + layout_box.width - outer_w +
				TINYWL_BORDER_THICKNESS;
			double max_y = layout_box.y + layout_box.height - outer_h +
				TINYWL_BORDER_THICKNESS;
			if (max_x >= min_x) {
				if (cx < min_x) {
					cx = min_x;
				}
				if (cx > max_x) {
					cx = max_x;
				}
			} else {
				cx = min_x;
			}
			if (max_y >= min_y) {
				if (cy < min_y) {
					cy = min_y;
				}
				if (cy > max_y) {
					cy = max_y;
				}
			} else {
				cy = min_y;
			}

			wlr_scene_node_set_position(&toplevel->scene_tree->node,
					(int)lround(cx), (int)lround(cy));
		}
		return;
	}

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
