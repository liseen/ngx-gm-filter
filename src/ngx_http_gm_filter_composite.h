#ifndef NGX_HTTP_GM_FILTER_COMPOSITE_H
#define NGX_HTTP_GM_FILTER_COMPOSITE_H

#include <magick/api.h>

ngx_int_t parse_composite_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, composite_options_t *option_info);
ngx_int_t composite_image(ngx_http_request_t *r, composite_options_t *option_info, Image **image);

#endif /* NGX_HTTP_GM_FILTER_COMPOSITE_H */
