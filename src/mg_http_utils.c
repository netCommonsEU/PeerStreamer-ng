#include<mg_http_utils.h>
#include<tokens.h>
#include<path_handlers.h>

const char * http_status(int status_code)
{
  switch (status_code) {
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 206:
      return "Partial Content";
    case 301:
      return "Moved";
    case 302:
      return "Found";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 409:
      return "Conflict";
    case 416:
      return "Requested Range Not Satisfiable";
    case 418:
      return "I'm a teapot";
    case 500:
      return "Internal Server Error";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";
	default:
      return "Unknown";
  }
}

void mg_http_answer(struct mg_connection *nc, uint16_t code, const char * content_type, const char * body)
{
	if (nc && !(nc->flags & CONNECTION_CLOSED) && content_type && body)
	{	
		mg_printf(nc, "%s %d %s\r\nTransfer-Encoding: chunked\r\nContent-type: %s\r\n\r\n", "HTTP/1.1", code, http_status(code), content_type);
		mg_printf_http_chunk(nc, body);
		mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
	}
}

void mg_http_text_answer(struct mg_connection *nc, uint16_t code, const char * body)
{
	mg_http_answer(nc, code, "text/plain", body);
}

void mg_http_json_answer(struct mg_connection *nc, uint16_t code, const char * json)
{
	mg_http_answer(nc, code, "application/json", json);
}

void mg_http_short_answer(struct mg_connection *nc, uint16_t code)
{
	mg_http_text_answer(nc, code, http_status(code));
}

char * mg_uri_field(struct http_message *hm, uint8_t pos)
{
	char * uri;
	char ** tokens;
	uint32_t ntok;

	uri = malloc((hm->uri.len + 1) * sizeof(char));
	strncpy(uri, hm->uri.p, hm->uri.len);
	uri[hm->uri.len] = '\0';

	tokens = tokens_create(uri, '/', &ntok);
	
	strcpy(uri, tokens[pos]);
	tokens_destroy(&tokens, ntok);

	return uri;
}
