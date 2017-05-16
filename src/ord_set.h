/*
 *
 *  copyright (c) 2017 luca baldesi
 *
 */

#ifndef __ORD_SET__
#define __ORD_SET__

#define ORD_SET_INC_SIZE 10

#include<stdint.h>

typedef uint32_t ord_set_size;

typedef int8_t (*cmp_func_t)(const void *, const void *);

struct ord_set;

struct ord_set * ord_set_new(ord_set_size size, cmp_func_t cmp);

void ord_set_destroy(struct ord_set ** os, uint8_t free_elements);

/* ord_set_insert
 * if element el is present it is overridden if override = 1
 * it returns the element in the set for the given equivalence class
 */
void * ord_set_insert(struct ord_set * os, void * el, uint8_t override);

const void * ord_set_iter(const struct ord_set * os, const void * iter);

ord_set_size ord_set_length(const struct ord_set * os);

const void * ord_set_find(const struct ord_set * os, const void * el);

uint8_t ord_set_remove(struct ord_set * os, const void * el, uint8_t free_element);

#endif
