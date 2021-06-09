#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_popup_interface;
extern const struct wl_interface xdg_positioner_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	&xdg_positioner_interface,
	&xdg_surface_interface,
	&wl_surface_interface,
	&xdg_toplevel_interface,
	&xdg_popup_interface,
	&xdg_surface_interface,
	&xdg_positioner_interface,
	&xdg_toplevel_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&wl_seat_interface,
	NULL,
	&wl_seat_interface,
	NULL,
	NULL,
	&wl_output_interface,
	&wl_seat_interface,
	NULL,
};

static const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy", "", types + 0 },
	{ "create_positioner", "n", types + 4 },
	{ "get_xdg_surface", "no", types + 5 },
	{ "pong", "u", types + 0 },
};

static const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base", 2,
	4, xdg_wm_base_requests,
	1, xdg_wm_base_events,
};

static const struct wl_message xdg_positioner_requests[] = {
	{ "destroy", "", types + 0 },
	{ "set_size", "ii", types + 0 },
	{ "set_anchor_rect", "iiii", types + 0 },
	{ "set_anchor", "u", types + 0 },
	{ "set_gravity", "u", types + 0 },
	{ "set_constraint_adjustment", "u", types + 0 },
	{ "set_offset", "ii", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_positioner_interface = {
	"xdg_positioner", 2,
	7, xdg_positioner_requests,
	0, NULL,
};

static const struct wl_message xdg_surface_requests[] = {
	{ "destroy", "", types + 0 },
	{ "get_toplevel", "n", types + 7 },
	{ "get_popup", "n?oo", types + 8 },
	{ "set_window_geometry", "iiii", types + 0 },
	{ "ack_configure", "u", types + 0 },
};

static const struct wl_message xdg_surface_events[] = {
	{ "configure", "u", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 2,
	5, xdg_surface_requests,
	1, xdg_surface_events,
};

static const struct wl_message xdg_toplevel_requests[] = {
	{ "destroy", "", types + 0 },
	{ "set_parent", "?o", types + 11 },
	{ "set_title", "s", types + 0 },
	{ "set_app_id", "s", types + 0 },
	{ "show_window_menu", "ouii", types + 12 },
	{ "move", "ou", types + 16 },
	{ "resize", "ouu", types + 18 },
	{ "set_max_size", "ii", types + 0 },
	{ "set_min_size", "ii", types + 0 },
	{ "set_maximized", "", types + 0 },
	{ "unset_maximized", "", types + 0 },
	{ "set_fullscreen", "?o", types + 21 },
	{ "unset_fullscreen", "", types + 0 },
	{ "set_minimized", "", types + 0 },
};

static const struct wl_message xdg_toplevel_events[] = {
	{ "configure", "iia", types + 0 },
	{ "close", "", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_toplevel_interface = {
	"xdg_toplevel", 2,
	14, xdg_toplevel_requests,
	2, xdg_toplevel_events,
};

static const struct wl_message xdg_popup_requests[] = {
	{ "destroy", "", types + 0 },
	{ "grab", "ou", types + 22 },
};

static const struct wl_message xdg_popup_events[] = {
	{ "configure", "iiii", types + 0 },
	{ "popup_done", "", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_popup_interface = {
	"xdg_popup", 2,
	2, xdg_popup_requests,
	2, xdg_popup_events,
};
