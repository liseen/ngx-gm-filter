#ifndef NGX_HTTP_GM_FILTER_UTIL_H
#define NGX_HTTP_GM_FILTER_UTIL_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <magick/api.h>

typedef struct
{
  char *name;
  unsigned char *magic;
  unsigned int length, offset;
}static_magic;

ngx_int_t get_image_format(const unsigned char *header, const size_t header_length, const static_magic **magic);

#endif /* NGX_HTTP_GM_FILTER_UTIL_H */
