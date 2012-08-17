#ifndef NGX_HTTP_GM_FILTER_MODULE_H
#define NGX_HTTP_GM_FILTER_MODULE_H

#define DDEBUG 1
#include "ddebug.h"

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <magick/api.h>

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
#define NGX_HTTP_GM_ROTATE_OPTION 2

/* comamnd type */
#define NGX_HTTP_GM_NONE_CMD      0
#define NGX_HTTP_GM_CONVERT_CMD   1
#define NGX_HTTP_GM_COMPOSITE_CMD 2

#if !defined(MaxTextExtent)
#  define MaxTextExtent  2053
#endif

typedef struct _CompositeOptions {
    char geometry[MaxTextExtent];
    CompositeOperator compose;
    GravityType gravity;

    ngx_uint_t min_width;
    ngx_uint_t min_height;

    ngx_str_t composite_image_file;

    Image *composite_image;
} composite_options_t;


typedef struct {
    ngx_uint_t                   type;

    ngx_str_t                    resize_geometry;
    ngx_http_complex_value_t    *resize_geometry_cv;
    ngx_str_t                    rotate_degrees;
    ngx_http_complex_value_t    *rotate_degrees_cv;
} ngx_http_gm_convert_option_t;

typedef struct _ConvertOptions
{
    ngx_array_t *options;
} convert_options_t;

typedef struct {
    ngx_uint_t                  type;

    char                       *cmd;
    composite_options_t         composite_options;
    convert_options_t           convert_options;
} ngx_http_gm_command_t;

typedef struct {
    ngx_array_t                 *cmds;
    size_t                       buffer_size;

    size_t                       image_quality;
} ngx_http_gm_conf_t;

typedef struct {
    u_char                      *image_blob;
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
