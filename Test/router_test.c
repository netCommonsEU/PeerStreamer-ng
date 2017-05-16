#include <assert.h>
#include <stdio.h>

#include <router.h>


char users_path[] = "/users/ciao";
char posts_path[] = "/users/ciao/posts/5";


void handler_posts(struct mg_connection *nc, struct http_message *hm)
{
	assert(mg_vcmp(&hm->uri, posts_path) == 0);
}

void handler_users(struct mg_connection *nc, struct http_message *hm)
{
	assert(mg_vcmp(&hm->uri, users_path) == 0);
}

void router_destroy_test()
{
	struct router * r = NULL;

	router_destroy(NULL);
	router_destroy(&r);
	r = router_create(0);
	router_destroy(&r);
	r = router_create(100);
	router_destroy(&r);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void router_add_route_test()
{
	struct router * r;
	uint8_t res;

	r = router_create(0);

	res = router_add_route(NULL, NULL, NULL, NULL);
	assert(res);
	res = router_add_route(r, NULL, NULL, NULL);
	assert(res);
	res = router_add_route(r, "GET", NULL, NULL);
	assert(res);
	res = router_add_route(r, "GET", "/users/[a-z]+", NULL);
	assert(res);
	res = router_add_route(NULL, "GET", "/users/[a-z]+", handler_users);
	assert(res);
	res = router_add_route(r, NULL, "/users/[a-z]+", handler_users);
	assert(res);
	res = router_add_route(r, "GET", NULL, handler_users);
	assert(res);
	res = router_add_route(r, "GET", "^/users/[a-z]+$", handler_users);
	assert(res == 0);
	res = router_add_route(r, "GET", "^/users/[a-z]+/posts/[0-9]+$", handler_posts);
	assert(res == 0);

	router_destroy(&r);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void router_handle_post_test()
{
	struct router * r = NULL;
	struct http_message hm;
	char method[] = "POST";
	uint8_t res;

	r = router_create(0);
	res = router_add_route(r, "POST", "^/users/[a-z]+$", handler_users);
	assert(res == 0);

	hm.method.p = method;
	hm.method.len = strlen(method);
	hm.uri.p = users_path;
	hm.uri.len = strlen(users_path);

	res = router_handle(r, NULL, &hm);
	assert(res == 0);

	router_destroy(&r);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void router_handle_test()
{
	struct router * r = NULL;
	struct http_message hm;
	char method[] = "GET";
	uint8_t res;

	r = router_create(0);
	res = router_add_route(r, "GET", "^/users/[a-z]+$", handler_users);
	assert(res == 0);
	res = router_add_route(r, "GET", "^/users/[a-z]+/posts/[0-9]+$", handler_posts);
	assert(res == 0);
	hm.method.p = method;
	hm.method.len = strlen(method);

	res = router_handle(NULL, NULL, NULL);
	assert(res);
	res = router_handle(r, NULL, NULL);
	assert(res);
	res = router_handle(NULL, NULL, &hm);
	assert(res);

	hm.uri.p = users_path;
	hm.uri.len = strlen(users_path);
	res = router_handle(r, NULL, &hm);
	assert(res == 0);

	hm.uri.p = posts_path;
	hm.uri.len = strlen(posts_path);
	res = router_handle(r, NULL, &hm);
	assert(res == 0);

	hm.uri.p = "foo";
	hm.uri.len = 3;
	res = router_handle(r, NULL, &hm);
	assert(res);

	router_destroy(&r);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	router_destroy_test();
	router_add_route_test();
	router_handle_test();
	router_handle_post_test();
	return 0;
}

