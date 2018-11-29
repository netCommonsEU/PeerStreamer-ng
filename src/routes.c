#include<path_handlers.h>


uint8_t load_path_handlers(struct router *r)
{
	uint8_t res = 0;

	res |= router_add_route(r, "GET", "^[/]+channels$", channel_index);
	res |= router_add_route(r, "POST", "^[/]+channels[/]+[a-zA-Z0-9]+$", streamer_create);
	res |= router_add_route(r, "UPDATE", "^[/]+channels[/]+[a-zA-Z0-9]+$", streamer_update);

	res |= router_add_route(r, "GET", "^[/]+sources$", source_index);
	res |= router_add_route(r, "POST", "^[/]+sources[/]+[a-zA-Z0-9]+$", source_streamer_create);
	res |= router_add_route(r, "UPDATE", "^[/]+sources[/]+[a-zA-Z0-9]+$", source_streamer_update);

	return res;
}
