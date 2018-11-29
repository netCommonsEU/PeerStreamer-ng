#ifndef __MG_HTTP_UTILS__
#define  __MG_HTTP_UTILS__

#include<mongoose.h>
#include<stdint.h>

void mg_http_answer(struct mg_connection *nc, uint16_t code, const char * content_type, const char * body);

void mg_http_text_answer(struct mg_connection *nc, uint16_t code, const char * body);

void mg_http_json_answer(struct mg_connection *nc, uint16_t code, const char * json);

char * mg_uri_field(struct http_message *hm, uint8_t pos);

void mg_http_short_answer(struct mg_connection *nc, uint16_t code);

#endif
