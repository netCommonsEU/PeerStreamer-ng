/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2016 Luca Baldesi <luca.baldesi@unitn.it>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************/

#include<regex.h>

#include "router.h"

#define HTTP_MAX_METHOD_LENGTH 10

struct route {
	char method[HTTP_MAX_METHOD_LENGTH];
	regex_t path;
	http_handle_t handler;
};

uint8_t route_init(struct route * r, const char * method, const char * regex, http_handle_t handler)
{
	uint8_t res = 0; 

	if(r && method && regex &&  handler)
	{
		 if (regcomp(&r->path, regex, REG_EXTENDED) == 0)
		{
			strncpy(r->method, method, HTTP_MAX_METHOD_LENGTH);
			r->method[HTTP_MAX_METHOD_LENGTH - 1] = '\0';
			r->handler = handler;
		}
	} else
		res = 1;
	return res;
}

void route_deinit(struct route * r)
{
	regfree(&r->path);
}

struct router {
	struct route * routes;
	uint32_t route_num;
	uint32_t route_max;
};

void router_destroy(struct router ** r)
{
	uint32_t i;

	if(r && *r)
	{
		if((*r)->routes)
		{
			for(i = 0; i < (*r)->route_num; i++)
				route_deinit(&((*r)->routes[i]));
			free((*r)->routes);
		}
		free(*r);
		*r = NULL;
	}
}

struct router * router_create(uint32_t size)
{
	struct router * r;
	r = malloc(sizeof(struct router));
	if (r)
	{
		r->route_num = 0;
		r->route_max = size;
		r->routes = malloc(sizeof(struct route) * r->route_max);
		if (!r->routes)
			router_destroy(&r);
	}
	return r;
}

uint8_t router_add_route(struct router *r, const char * method, const char * regex, http_handle_t handler)
{
	uint8_t res = 0;

	if (r && r->route_num >= r->route_max)
	{
		r->route_max = r->route_max ? r->route_max * 2 : 2;
		r->routes = realloc(r->routes, sizeof(struct route) * r->route_max);
	}

	if (r && r->routes)
	{
		res = route_init(&(r->routes[r->route_num]), method, regex, handler);
		if(res == 0)
			r->route_num++;
	}
	else
		res = 1;

	return res;
}

char * mg_str2str(const struct mg_str * s)
{
	char * p;

	p = malloc(s->len + 1);
	strncpy(p, s->p, s->len);
	p[s->len] = '\0';
	return p;
}

uint8_t router_handle(const struct router * r, struct mg_connection *nc, struct http_message *hm)
{
	uint32_t i = 0, res = 1;
	char * str;

	if(hm && r)
	{
		while (res && i < r->route_num)
		{
			if(mg_vcmp(&hm->method, r->routes[i].method) == 0)
			{
				str = mg_str2str(&hm->uri);
				if(regexec(&(r->routes[i].path), str, 0, NULL, 0) == 0)
				{
					res = 0;
					r->routes[i].handler(nc, hm);
				}
				free(str);
			}
			i++;
		}
	}
	return res;
}
