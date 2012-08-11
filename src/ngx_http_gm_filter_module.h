#ifndef NGX_HTTP_GM_FILTER_MODULE_H
#define NGX_HTTP_GM_FILTER_MODULE_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_GM_START     0
#define NGX_HTTP_GM_IMAGE_READ      1
#define NGX_HTTP_GM_IMAGE_PROCESS   2
#define NGX_HTTP_GM_IMAGE_PASS      3
#define NGX_HTTP_GM_IMAGE_DONE      4

#define NGX_HTTP_GM_IMAGE_NONE      0
#define NGX_HTTP_GM_IMAGE_JPEG      1
#define NGX_HTTP_GM_IMAGE_GIF       2
#define NGX_HTTP_GM_IMAGE_PNG       3

#define NGX_HTTP_IMAGE_BUFFERED  0x08

/* option type */
#define NGX_HTTP_GM_NONE_OPTION   0
#define NGX_HTTP_GM_RESIZE_OPTION 1

/* comamnd type */
#define NGX_HTTP_GM_NONE_CMD      0
#define NGX_HTTP_GM_CONVERT_CMD   1
#define NGX_HTTP_GM_COMPOSITE_CMD 2

typedef struct {
    ngx_uint_t                   type;
    ngx_array_t                 *args;
} ngx_http_gm_option;


typedef struct {
    ngx_uint_t                  type;
    ngx_array_t                *options;
} ngx_http_gm_cmd;

typedef struct {
    ngx_array_t                 *cmds;
    size_t                       buffer_size;
} ngx_http_gm_conf_t;

typedef struct {
    u_char                      *image;
    u_char                      *last;

    size_t                       length;

    ngx_uint_t                   width;
    ngx_uint_t                   height;

    ngx_uint_t                   phase;
    ngx_uint_t                   type;
    ngx_uint_t                   force;
} ngx_http_gm_ctx_t;


extern ngx_module_t ngx_http_gm_module;

#endif /* NGX_HTTP_GM_FILTER_MODULE_H */
