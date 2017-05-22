/*
 * This is SSSim: the Simple & Stupid Simulator
 *
 *  Copyright (c) 2015 Luca Baldesi
 *
 *  This is free software; see gpl-3.0.txt
 */

#include "tokens.h"
#include <malloc.h>
#include <string.h>

char * substring_trim(const char * s,uint32_t start,uint32_t len)
{
	uint32_t leading=0,trailing=0;

	while(leading < len && (s[start+leading] == '\n' || s[start+leading] == ' '))
		leading++;
	while(trailing < len && (s[start+len-trailing-1] == '\n' || s[start+len-trailing-1] == ' '))
		trailing++;

	if(len - leading -trailing > 0)
		return strndup(s+start+leading,len-trailing);
	else
		return NULL;
}

void tokens_destroy(char *** sv,uint32_t n)
{
	uint32_t i;

	if (sv && *sv && **sv)
	{
		for( i = 0; i < n; i++)
			free((*sv)[i]);
		free(*sv);
		*sv = NULL;
	}
}

char ** tokens_create(char * s,const char delim,uint32_t * n)
{
	char * np =s, *nnp;
	char ** sv = NULL;
	uint32_t i;


	if(s && n && strcmp(s,""))
	{
		*n = 0;
		while((nnp = strchr(np,delim)))
		{
			if ((nnp - np) > 1)
				(*n)++;
			np = nnp + 1;
		}
		if (strlen(np) > 0)
			(*n)++;
		sv = (char **) malloc(sizeof(char *) * (*n));

		np = nnp = s;
		i = 0;
		while((nnp = strchr(np,delim)))
		{
			if ((nnp - np) > 1)
			{
				sv[i++] = substring_trim(np,0,nnp-np);
			}
			np = nnp+1;
		}
		if (strlen(np) > 0)
			sv[i] = substring_trim(np,0,strlen(np));
	}
	return sv;
}

int32_t tokens_check(char ** tokens, uint32_t n, char * target)
{
	int32_t pos = -1;
	uint32_t i;

	if(tokens && *tokens && target)
	{
		i = 0;
		while (pos < 0 && i < n)
		{
			if (strcmp(tokens[i], target) == 0)
				pos = i;
			i++;
		}
	}
	return pos;
}
