#pragma once

struct tinywl_server;

enum layout_mode {
	LAYOUT_FLOAT = 0,
	LAYOUT_SCROLLER,
};

void layout_arrange(struct tinywl_server *server);
