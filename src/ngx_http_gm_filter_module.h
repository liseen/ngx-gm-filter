#ifndef NGX_HTTP_GM_FILTER_MODULE_H
#define NGX_HTTP_GM_FILTER_MODULE_H

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
#define NGX_HTTP_GM_CROP_OPTION   3

/* comamnd type */
#define NGX_HTTP_GM_NONE_CMD      0
#define NGX_HTTP_GM_CONVERT_CMD   1
#define NGX_HTTP_GM_COMPOSITE_CMD 2

#define ngx_gm_null_command  { ngx_null_string, NULL, NULL, NULL}

#if !defined(MaxTextExtent)
#  define MaxTextExtent  2053
#endif

typedef struct {
    ngx_uint_t                   type;

    ngx_str_t                    resize_geometry;
    ngx_http_complex_value_t    *resize_geometry_cv;
    ngx_str_t                    rotate_degrees;
    ngx_http_complex_value_t    *rotate_degrees_cv;
    ngx_str_t                    crop_geometry;
    ngx_http_complex_value_t    *crop_geometry_cv;
} ngx_http_gm_convert_option_t;

/* gm command handler */
typedef ngx_int_t (*ngx_http_gm_command_pt)(ngx_http_request_t *r, void *option, Image **image);

/* gm command option parse handler */
typedef ngx_int_t (*ngx_http_gm_parse_pt)(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);

/* gm out buf */
typedef ngx_buf_t *(*ngx_http_gm_out_pt)(ngx_http_request_t *r, Image *image);

/* gm command */
typedef struct {
    ngx_str_t                   name;
    ngx_http_gm_command_pt      handler;
    ngx_http_gm_parse_pt        option_parse_handler;
    ngx_http_gm_out_pt          out_handler;
} ngx_http_gm_command_t;

/* gm command info */
typedef struct {
    ngx_http_gm_command_t       *command;
    void                        *option;
}ngx_http_gm_command_info_t;

/* gm geometry */
typedef struct {
    ngx_str_t                   geometry;
    ngx_http_complex_value_t    *geometry_cv;
}ngx_http_gm_geometry_t;

/* gm conf */
typedef struct {
    ngx_array_t                 *cmds;          /* gm command info list */

    ngx_flag_t                  filter;         /* gm_filter */
    ngx_int_t                   *filter_statuses;

    size_t                       buffer_size;   /* gm_buffer */

    ngx_http_complex_value_t    *style_cv;      /* gm_style */
      
    ngx_http_complex_value_t    *qcv;           /* gm_quality */
    ngx_uint_t                   image_quality;
} ngx_http_gm_conf_t;

/* gm context */
typedef struct {
    u_char                      *image_blob;
    u_char                      *last;

    size_t                       length;

    ngx_uint_t                   phase;
    ngx_uint_t                   type;
} ngx_http_gm_ctx_t;

extern ngx_module_t ngx_http_gm_module;

ngx_int_t ngx_http_gm_get_geometry_value(ngx_conf_t *conf, ngx_str_t *value, ngx_http_gm_geometry_t *geo);

u_char* ngx_http_gm_get_str_value(ngx_http_request_t *r, ngx_http_complex_value_t *cv, ngx_str_t *val);

extern MagickExport GravityType StringToGravityType(const char *option);

#endif /* NGX_HTTP_GM_FILTER_MODULE_H */
