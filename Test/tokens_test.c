/*
 * This is SSSim: the Simple & Stupid Simulator
 *
 *  Copyright (c) 2015 Luca Baldesi
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <tokens.h>

void tokens_create_test()
{
	char ** toks;
	uint32_t n;
	
	toks = tokens_create(NULL, ' ', NULL);
	assert(toks == NULL);

	toks = tokens_create(NULL, ' ', &n);
	assert(toks == NULL);

	toks = tokens_create("ciao mondo", ' ', NULL);
	assert(toks == NULL);

	toks = tokens_create("ciao mondo", ' ', &n);
	assert(toks);
	assert(n == 2);
	assert(strcmp(toks[0], "ciao") == 0);
	assert(strcmp(toks[1], "mondo") == 0);
	tokens_destroy(&toks, n);

	toks = tokens_create("ciao,mondo", ',', &n);
	assert(toks);
	assert(n == 2);
	assert(strcmp(toks[0], "ciao") == 0);
	assert(strcmp(toks[1], "mondo") == 0);
	tokens_destroy(&toks, n);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void tokens_destroy_test()
{
	char ** toks;
	uint32_t n;

	tokens_destroy(NULL, 3);

	toks = tokens_create("ciao,mondo", ',', &n);
	tokens_destroy(&toks, n);
	assert(toks == NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void tokens_check_test()
{
	char ** toks;
	uint32_t n;
	
	toks = tokens_create("ciao mondo", ' ', &n);
	assert(tokens_check(toks, n, "ciao") == 0);
	assert(tokens_check(toks, n, "mondo") == 1);
	assert(tokens_check(toks, n, "zio") < 0);
	tokens_destroy(&toks, n);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void tokens_url_test()
{
	char ** tokens;
	uint32_t ntok;
	char * uri1 = "/channels/cool";
	char * uri2 = "/channels/cool/";
	char * uri3 = "/channels//cool/";

	tokens = tokens_create(uri1, '/', &ntok);
	assert(ntok==2);
	assert(tokens_check(tokens, ntok, "channels") == 0);
	assert(tokens_check(tokens, ntok, "cool") == 1);
	tokens_destroy(&tokens, ntok);

	tokens = tokens_create(uri2, '/', &ntok);
	assert(ntok==2);
	assert(tokens_check(tokens, ntok, "channels") == 0);
	assert(tokens_check(tokens, ntok, "cool") == 1);
	tokens_destroy(&tokens, ntok);

	tokens = tokens_create(uri3, '/', &ntok);
	assert(ntok==2);
	assert(tokens_check(tokens, ntok, "channels") == 0);
	assert(tokens_check(tokens, ntok, "cool") == 1);
	tokens_destroy(&tokens, ntok);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(int argv, char ** argc)
{
	tokens_create_test();
	tokens_destroy_test();
	tokens_check_test();
	tokens_url_test();
	return 0;
}
