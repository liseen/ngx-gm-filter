/* vim:set ft=c ts=4 sw=4 et fdm=marker: */

#include "ngx_http_gm_filter_module.h"

static ngx_int_t ngx_http_gm_image_send(ngx_http_request_t *r,
    ngx_http_gm_ctx_t *ctx, ngx_chain_t *in);

static ngx_uint_t ngx_http_gm_image_test(ngx_http_request_t *r,
    ngx_chain_t *in);

static ngx_int_t ngx_http_gm_image_read(ngx_http_request_t *r,
    ngx_chain_t *in);

static ngx_buf_t *ngx_http_gm_image_process(ngx_http_request_t *r);

static void ngx_http_gm_image_cleanup(void *data);
static ngx_uint_t ngx_http_gm_filter_value(ngx_str_t *value);

static void ngx_http_gm_image_length(ngx_http_request_t *r,
    ngx_buf_t *b);
static ngx_buf_t *ngx_http_gm_image_json(ngx_http_request_t *r,  Image *image);

static ngx_buf_t * ngx_http_gm_image_run_commands(ngx_http_request_t *r,
    ngx_http_gm_ctx_t *ctx);

static void *ngx_http_gm_create_conf(ngx_conf_t *cf);
static char *ngx_http_gm_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);

static char *ngx_http_gm_quality(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static char *
ngx_http_gm_statuses(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *ngx_http_gm_gm(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static char *ngx_http_gm_style(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static ngx_int_t ngx_http_gm_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_gm_init_worker(ngx_cycle_t *cycle);
static void ngx_http_gm_exit_worker(ngx_cycle_t *cycle);

static ngx_int_t ngx_http_gm_parse_style(ngx_conf_t *cf, ngx_http_request_t *r, ngx_str_t *value, void *conf);

/* parse geometry option */
static ngx_int_t ngx_http_gm_parse_geometry(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);

/* resize */
ngx_int_t gm_resize_image(ngx_http_request_t *r, void *option, Image **image);

/* thumbnail */
ngx_int_t gm_thumbnail_image(ngx_http_request_t *r, void *option, Image **image);

/* sample */
ngx_int_t gm_sample_image(ngx_http_request_t *r, void *option, Image **image);

/* scale */
ngx_int_t gm_scale_image(ngx_http_request_t *r, void *option, Image **image);

/* crop */
ngx_int_t parse_crop_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);
ngx_int_t gm_crop_image(ngx_http_request_t *r, void *option, Image **image);

/* rotate */
ngx_int_t gm_parse_rotate_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);
ngx_int_t gm_rotate_image(ngx_http_request_t *r, void *option, Image **image);

/* convert */
ngx_int_t parse_convert_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);
ngx_int_t convert_image(ngx_http_request_t *r, void *option, Image **image);

/* composite */
ngx_int_t parse_composite_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option);
ngx_int_t composite_image(ngx_http_request_t *r, void *option, Image **image);

/* gm command */
static ngx_http_gm_command_t ngx_gm_commands[] = {
    { ngx_string("empty"),
      NULL,
      NULL,
      NULL},

    { ngx_string("identify"),
      NULL,
      NULL,
      ngx_http_gm_image_json},

    { ngx_string("convert"),
      convert_image,
      parse_convert_options,
      NULL },

    { ngx_string("resize"),
      gm_resize_image,
      ngx_http_gm_parse_geometry,
      NULL },

    { ngx_string("thumbnail"),
      gm_thumbnail_image,
      ngx_http_gm_parse_geometry,
      NULL },

    { ngx_string("sample"),
      gm_sample_image,
      ngx_http_gm_parse_geometry,
      NULL },

    { ngx_string("scale"),
      gm_scale_image,
      ngx_http_gm_parse_geometry,
      NULL },

    { ngx_string("crop"),
      gm_crop_image,
      parse_crop_options, 
      NULL },

    { ngx_string("rotate"),
      gm_rotate_image,
      gm_parse_rotate_options, 
      NULL },

    { ngx_string("composite"),
      composite_image,
      parse_composite_options, 
      NULL },

    ngx_gm_null_command
};

static ngx_command_t  ngx_http_gm_commands[] = {
    { ngx_string("gm_filter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gm_conf_t, filter),
      NULL },

    { ngx_string("gm_filter_statuses"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_gm_statuses,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("gm"),
      NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_gm_gm,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /* use for custom combined command */
    { ngx_string("gm_style"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_gm_style,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },


    { ngx_string("gm_image_quality"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_gm_quality,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },


    { ngx_string("gm_buffer"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gm_conf_t, buffer_size),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_gm_module_ctx = {
    NULL,                        /* preconfiguration */
    ngx_http_gm_init,            /* postconfiguration */

    NULL,                        /* create main configuration */
    NULL,                        /* init main configuration */

    NULL,                        /* create server configuration */
    NULL,                        /* merge server configuration */

    ngx_http_gm_create_conf,     /* create location configuration */
    ngx_http_gm_merge_conf       /* merge location configuration */
};


ngx_module_t  ngx_http_gm_module = {
    NGX_MODULE_V1,
    &ngx_http_gm_module_ctx,        /* module context */
    ngx_http_gm_commands,           /* module directives */
    NGX_HTTP_MODULE,                /* module type */
    NULL,                           /* init master */
    NULL,                           /* init module */
    ngx_http_gm_init_worker,        /* init process */
    NULL,                           /* init thread */
    NULL,                           /* exit thread */
    ngx_http_gm_exit_worker,        /* exit process */
    NULL,                           /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_str_t  ngx_http_gm_image_types[] = {
    ngx_string("image/jpeg"),
    ngx_string("image/gif"),
    ngx_string("image/png")
};


static ngx_int_t
ngx_http_gm_header_filter(ngx_http_request_t *r)
{
    off_t                          len;
    ngx_uint_t                     *sp; /* status pointer */
    ngx_http_gm_ctx_t   *ctx;
    ngx_http_gm_conf_t  *conf;

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);

    if (ctx) {
        dd("gm filter: image filter bypass because of ctx exist, %.*s", (int) r->uri.len, r->uri.data);
        ngx_http_set_ctx(r, NULL, ngx_http_gm_module);
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

    if (!conf->filter) {
        return ngx_http_next_header_filter(r);
    }

    if (conf->filter_statuses != NULL) {

        sp = (ngx_uint_t *) conf->filter_statuses;

        while (r->headers_out.status < *sp) {
            sp++;
        }

        if (*sp == 0 || r->headers_out.status > *sp) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "gm filter: image filter bypassed because of unmatched "
                    "status code %ui with gm_filter_statuses",
                    r->headers_out.status);

            return ngx_http_next_header_filter(r);
        }

    } else {

        if (r->headers_out.status != NGX_HTTP_OK) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "gm filter: image filter bypassed because of unmatched status "
                    "code %i (only 200 are accepted by "
                    "default)", r->headers_out.status);

            return ngx_http_next_header_filter(r);
        }
    }

    if (r->headers_out.content_type.len
            >= sizeof("multipart/x-mixed-replace") - 1
        && ngx_strncasecmp(r->headers_out.content_type.data,
                           (u_char *) "multipart/x-mixed-replace",
                           sizeof("multipart/x-mixed-replace") - 1)
           == 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "gm filter: multipart/x-mixed-replace response");

        return NGX_ERROR;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_gm_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, ngx_http_gm_module);

    len = r->headers_out.content_length_n;

    if (len != -1 && len > (off_t) conf->buffer_size) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "gm filter: too big response: %O", len);

        return NGX_HTTP_UNSUPPORTED_MEDIA_TYPE;
    }

    if (len == -1) {
        ctx->length = conf->buffer_size;

    } else {
        ctx->length = (size_t) len;
    }

    if (r->headers_out.refresh) {
        r->headers_out.refresh->hash = 0;
    }

    r->main_filter_need_in_memory = 1;
    r->allow_ranges = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_gm_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_int_t                      rc;
    ngx_str_t                     *ct;
    ngx_chain_t                    out;
    ngx_http_gm_ctx_t   *ctx;


    if (in == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);

    if (ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    switch (ctx->phase) {

    case NGX_HTTP_GM_START:

        ctx->type = ngx_http_gm_image_test(r, in);


        if (ctx->type == NGX_HTTP_GM_IMAGE_NONE) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "gm filter: image test return none");
            return ngx_http_filter_finalize_request(r,
                                              &ngx_http_gm_module,
                                              NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
        }

        /* override content type */

        ct = &ngx_http_gm_image_types[ctx->type - 1];
        r->headers_out.content_type_len = ct->len;
        r->headers_out.content_type = *ct;
        r->headers_out.content_type_lowcase = NULL;

        ctx->phase = NGX_HTTP_GM_IMAGE_READ;

        /* fall through */

    case NGX_HTTP_GM_IMAGE_READ:

        rc = ngx_http_gm_image_read(r, in);

        if (rc == NGX_AGAIN) {
            return NGX_OK;
        }

        if (rc == NGX_ERROR) {
            return ngx_http_filter_finalize_request(r,
                                              &ngx_http_gm_module,
                                              NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
        }

        /* fall through */

    case NGX_HTTP_GM_IMAGE_PROCESS:

        out.buf = ngx_http_gm_image_process(r);

        if (out.buf == NULL) {
            return ngx_http_filter_finalize_request(r,
                                              &ngx_http_gm_module,
                                              NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
        }

        out.next = NULL;
        ctx->phase = NGX_HTTP_GM_IMAGE_PASS;

        return ngx_http_gm_image_send(r, ctx, &out);

    case NGX_HTTP_GM_IMAGE_PASS:

        return ngx_http_next_body_filter(r, in);

    default: /* NGX_HTTP_GM_IMAGE_DONE */

        rc = ngx_http_next_body_filter(r, NULL);

        /* NGX_ERROR resets any pending data */
        return (rc == NGX_OK) ? NGX_ERROR : rc;
    }
}


static ngx_int_t
ngx_http_gm_image_send(ngx_http_request_t *r, ngx_http_gm_ctx_t *ctx,
    ngx_chain_t *in)
{
    ngx_int_t  rc;

    rc = ngx_http_next_header_filter(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return NGX_ERROR;
    }

    rc = ngx_http_next_body_filter(r, in);

    if (ctx->phase == NGX_HTTP_GM_IMAGE_DONE) {
        /* NGX_ERROR resets any pending data */
        return (rc == NGX_OK) ? NGX_ERROR : rc;
    }

    return rc;
}


static ngx_uint_t
ngx_http_gm_image_test(ngx_http_request_t *r, ngx_chain_t *in)
{
    u_char  *p;

    p = in->buf->pos;

    if (in->buf->last - p < 16) {
        return NGX_HTTP_GM_IMAGE_NONE;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "gm filter: \"%c%c\"", p[0], p[1]);

    if (p[0] == 0xff && p[1] == 0xd8) {

        /* JPEG */

        return NGX_HTTP_GM_IMAGE_JPEG;

    } else if (p[0] == 'G' && p[1] == 'I' && p[2] == 'F' && p[3] == '8'
               && p[5] == 'a')
    {
        if (p[4] == '9' || p[4] == '7') {
            /* GIF */
            return NGX_HTTP_GM_IMAGE_GIF;
        }

    } else if (p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G'
               && p[4] == 0x0d && p[5] == 0x0a && p[6] == 0x1a && p[7] == 0x0a)
    {
        /* PNG */

        return NGX_HTTP_GM_IMAGE_PNG;
    }

    return NGX_HTTP_GM_IMAGE_NONE;
}


static ngx_int_t
ngx_http_gm_image_read(ngx_http_request_t *r, ngx_chain_t *in)
{
    u_char                       *p;
    size_t                        size, rest;
    ngx_buf_t                    *b;
    ngx_chain_t                  *cl;
    ngx_http_gm_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);

    if (ctx->image_blob == NULL) {
        ctx->image_blob = ngx_palloc(r->pool, ctx->length);
        if (ctx->image_blob == NULL) {
            return NGX_ERROR;
        }

        ctx->last = ctx->image_blob;
    }

    p = ctx->last;

    for (cl = in; cl; cl = cl->next) {

        b = cl->buf;
        size = b->last - b->pos;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "gm filter: image buf size %uz", size);

        rest = ctx->image_blob + ctx->length - p;
        size = (rest < size) ? rest : size;

        p = ngx_cpymem(p, b->pos, size);
        b->pos += size;

        if (b->last_buf) {
            ctx->last = p;
            return NGX_OK;
        }
    }

    ctx->last = p;
    r->connection->buffered |= NGX_HTTP_IMAGE_BUFFERED;

    return NGX_AGAIN;
}


static ngx_buf_t *
ngx_http_gm_image_process(ngx_http_request_t *r)
{
    ngx_http_gm_ctx_t   *ctx;
    ngx_http_gm_conf_t  *gmcf;
    ngx_str_t            str;

    r->connection->buffered &= ~NGX_HTTP_IMAGE_BUFFERED;

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);
    gmcf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

    /* quality */
    if (gmcf->qcv != NULL) {
        if (ngx_http_complex_value(r, gmcf->qcv, &str) != NGX_OK) {
            return NULL;
        }
        gmcf->image_quality = ngx_http_gm_filter_value(&str);
    }

    /* style */
    if (gmcf->style_cv != NULL) {
        if (ngx_http_complex_value(r, gmcf->style_cv, &str) != NGX_OK) {
            return NULL;
        }

        /* parse style */
        if (ngx_http_gm_parse_style(NULL, r, &str, gmcf) != NGX_OK) {
            return NULL;
        }
    }

    return ngx_http_gm_image_run_commands(r, ctx);
}


static ngx_buf_t *
ngx_http_gm_image_run_commands(ngx_http_request_t *r, ngx_http_gm_ctx_t *ctx)
{
    ngx_buf_t           *b;
    ngx_int_t            rc;
    ngx_http_gm_conf_t  *gmcf;

    u_char         *image_blob;

    ImageInfo      *image_info;
    Image          *image;
    ExceptionInfo   exception;

    ngx_uint_t      i;
    ngx_http_gm_command_t *gm_cmd;
    ngx_http_gm_command_info_t *cmd_info;
    u_char         *out_blob;
    ngx_uint_t      out_len;

    ngx_pool_cleanup_t            *cln;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "gm filter: entering gm image run commands");

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "gm filter: url %V", &r->uri);

    gmcf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

    GetExceptionInfo(&exception);

    image_blob = ctx->image_blob;

    image_info = CloneImageInfo((ImageInfo *) NULL);

    /* blob to image */
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "gm filter: blob to image");

    image = BlobToImage(image_info, image_blob, ctx->length, &exception);
    if (image == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "gm filter: blob to image failed, "
                      "severity: %O reason: %s, description: %s",
                      exception.severity, exception.reason,
                      exception.description);

        goto failed1;
    }


    /* run commands */
    rc = NGX_OK;
    if (gmcf->cmds != NULL) {
        for (i = 0, cmd_info = gmcf->cmds->elts; i < gmcf->cmds->nelts; ++i, cmd_info++) {
            gm_cmd = cmd_info->command;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "gm filter: run command: \"%V\"",
                    &gm_cmd->name);

            if (gm_cmd->handler != NULL) {
                rc = gm_cmd->handler(r, cmd_info->option, &image);
            }

            if (rc != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: run command failed, command: \"%V\"",
                        &gm_cmd->name);

                goto failed2;
            }

            if (gm_cmd->out_handler != NULL) {
                b = gm_cmd->out_handler(r, image);
                goto out;
            }
        }
    }

    /* image to blob */
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: image to blob, quality %d", gmcf->image_quality);

    image_info->quality = gmcf->image_quality;

    out_blob = ImageToBlob(image_info, image,  &out_len, &exception);
    if (out_blob == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: image to blob failed, "
                "severity: %O reason: %s, description: %s",
                exception.severity, exception.reason,
                exception.description);
        goto failed2;
    }

    /* image out to buf */
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: blob to buf");

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: alloc buf_t failed");
        goto failed3;
    }

    b->pos = out_blob;
    b->last = out_blob + out_len;
    b->memory = 1;
    b->last_buf = 1;

    /* register cleanup */
    cln = ngx_pool_cleanup_add(r->pool, 0);
    if (cln == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: register cleanup failed");
        goto failed3;
    }

    cln->handler = ngx_http_gm_image_cleanup;
    cln->data = out_blob;

out:
    ngx_http_gm_image_length(r, b);


    /* destory imput blob */
    ngx_pfree(r->pool, ctx->image_blob);

    /* destory iamge */
    DestroyImage(image);
    /* destory image info */
    DestroyImageInfo(image_info);
    DestroyExceptionInfo(&exception);

    return b;

failed3:
    /* clean out blob */
    MagickFree(out_blob);

failed2:
    /* destory iamge */
    DestroyImage(image);

failed1:
    /* destory image info */
    DestroyImageInfo(image_info);
    DestroyExceptionInfo(&exception);

    return NULL;
}

static ngx_buf_t *
ngx_http_gm_image_json(ngx_http_request_t *r,  Image *image)
{
    size_t      len;
    ngx_buf_t  *b;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NULL;
    }

    b->memory = 1;
    b->last_buf = 1;

    ngx_http_clean_header(r);

    r->headers_out.status = NGX_HTTP_OK;
    ngx_str_set(&r->headers_out.content_type, "application/json");
    r->headers_out.content_type_lowcase = NULL;

    len = sizeof("{ \"img\" : "
                 "{ \"width\": , \"height\": , \"type\": \"jpeg\" } }" CRLF) - 1
          + 2 * NGX_SIZE_T_LEN;

    b->pos = ngx_pnalloc(r->pool, len);
    if (b->pos == NULL) {
        return NULL;
    }

    b->last = ngx_sprintf(b->pos,
                          "{ \"img\" : "
                                       "{ \"width\": %uz,"
                                        " \"height\": %uz,"
                                        " \"type\": \"%s\" } }" CRLF,
                          image->columns,image->rows,
                          image->magick);

    return b;
}

static void
ngx_http_gm_image_cleanup(void *out_blob)
{
    dd("cleanup iamge out_blob");
    MagickFree(out_blob);
}


static void
ngx_http_gm_image_length(ngx_http_request_t *r, ngx_buf_t *b)
{
    r->headers_out.content_length_n = b->last - b->pos;

    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
    }

    r->headers_out.content_length = NULL;
}


static void *
ngx_http_gm_create_conf(ngx_conf_t *cf)
{
    ngx_http_gm_conf_t  *gmcf;

    gmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_gm_conf_t));
    if (gmcf == NULL) {
        return NULL;
    }

    gmcf->filter = NGX_CONF_UNSET;
    gmcf->buffer_size = NGX_CONF_UNSET_SIZE;
    gmcf->image_quality = NGX_CONF_UNSET_UINT;

    return gmcf;
}


static char *
ngx_http_gm_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_gm_conf_t *prev = parent;
    ngx_http_gm_conf_t *conf = child;

    if (conf->cmds == NULL && prev->cmds != NULL) {
        conf->cmds = prev->cmds;
    }

    ngx_conf_merge_value(conf->filter, prev->filter, 0);
    ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size,
                              4 * 1024 * 1024);

    if (conf->image_quality == NGX_CONF_UNSET_UINT) {

        ngx_conf_merge_uint_value(conf->image_quality, prev->image_quality, 75);

        if (conf->qcv == NULL) {
            conf->qcv = prev->qcv;
        }
    }

    if (conf->filter_statuses == NULL) {
        conf->filter_statuses = prev->filter_statuses;
    }
    
    
    return NGX_CONF_OK;
}

/* geometry option parse */
ngx_int_t 
ngx_http_gm_parse_geometry(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option)
{
    ngx_http_gm_geometry_t            *geo;

    ngx_uint_t                         rc;
    ngx_str_t                         *value;

    dd("entering");

    if (args->nelts - start <= 0) {
        return NGX_ERROR;
    }

    value = args->elts;
    value += start + 1;

    geo = ngx_palloc(cf->pool, sizeof(ngx_http_gm_geometry_t));
    if (geo == NULL) {
        return NGX_ERROR;
    }
    ngx_memzero(geo, sizeof(ngx_http_gm_geometry_t));

    rc = ngx_http_gm_get_geometry_value(cf, value, geo);

    if (rc == NGX_OK) {
        *option = geo;
    }

    return rc;
}

/* get geometry value, support variable */
ngx_int_t 
ngx_http_gm_get_geometry_value(ngx_conf_t *conf, ngx_str_t *value, ngx_http_gm_geometry_t *geo)
{
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = conf;
    ccv.value = value;
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_ERROR;
    }

    if (cv.lengths == NULL) {
        if (IsGeometry((const char *)value->data)) {
            geo->geometry = *value;
            return NGX_OK;
        } else {
            return NGX_ERROR;
        }

    } else {
        geo->geometry_cv = ngx_palloc(conf->pool, sizeof(ngx_http_complex_value_t));
        if (geo->geometry_cv == NULL) {
            return NGX_ERROR;
        }

        *geo->geometry_cv = cv;
        return NGX_OK;
    }
}


/* get value */
u_char *
ngx_http_gm_get_str_value(ngx_http_request_t *r, ngx_http_complex_value_t *cv,
    ngx_str_t *val)
{
    u_char      *buf;
    ngx_str_t  str;

    if (cv == NULL) {
        if (val == NULL) {
            return NULL;
        }

        buf = ngx_pcalloc(r->pool, val->len + 1);
        if (buf == NULL) {
            return NULL;
        }

        ngx_memcpy(buf, val->data, val->len);
        buf[val->len] = '\0';

        return buf;
    } else {
        if (ngx_http_complex_value(r, cv, &str) != NGX_OK) {
            return NULL;
        }

        buf = ngx_pcalloc(r->pool, str.len + 1);
        if (buf == NULL) {
            return NULL;
        }

        ngx_memcpy(buf, str.data, str.len);
        buf[str.len] = '\0';

        return buf;
    }
}

static char *
ngx_http_gm_gm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_gm_conf_t                *gmcf = conf;

    ngx_str_t                         *value;

    ngx_http_gm_command_t             *gm_cmd;
    ngx_http_gm_command_info_t        *cmd_info;

    ngx_int_t                          rc;
    ngx_uint_t                         i, j;

    ngx_array_t                       *args;

    dd("entering");

    args  = cf->args;
    value = cf->args->elts;
    gmcf->filter = 1;

    i = 1;

    if (args->nelts < 2) {
        return NGX_CONF_ERROR;
    }

    if (gmcf->cmds == NULL) {
        gmcf->cmds = ngx_array_create(cf->pool, 4, sizeof(ngx_http_gm_command_info_t));
        if (gmcf->cmds == NULL) {
            goto failed;
        }
    }
    
    gm_cmd = ngx_gm_commands;
    rc = NGX_ERROR;

    for(/* void */; gm_cmd->name.len; gm_cmd++) {

        if (ngx_strcmp(value[i].data, gm_cmd->name.data) != 0) {
            continue;
        }

        cmd_info = ngx_array_push(gmcf->cmds);
        if (cmd_info == NULL) {
            goto alloc_failed;
        }

        cmd_info->command = gm_cmd;

        if (gm_cmd->option_parse_handler) {
            rc = gm_cmd->option_parse_handler(cf, args, i, &cmd_info->option);
            if (rc != NGX_OK) {
                goto failed;
            }
        }
        rc = NGX_OK;

        break;
    }

    /* command extensive */
    if (rc == NGX_OK) {

        for (j = 0, cmd_info = gmcf->cmds->elts; j < gmcf->cmds->nelts; ++j, cmd_info++) {
            if (gm_cmd->out_handler != NULL && gmcf->cmds->nelts > 1) {
                goto failed;
            }
        }
    }

    if (rc != NGX_OK) {
        goto failed;
    }
        
    dd("parse config okay");

    return NGX_CONF_OK;

alloc_failed:
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "alloc failed \"%V\"",
                       &value[i]);

    return NGX_CONF_ERROR;

failed:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter for command, \"%V\"",
                       &value[i]);

    return NGX_CONF_ERROR;
}

static ngx_int_t
ngx_http_gm_parse_style(ngx_conf_t *cf, ngx_http_request_t *r, ngx_str_t *value, void *conf)
{
    ngx_conf_t                         style;
    ngx_conf_file_t                    conf_file;
    ngx_buf_t                          b;
    
    if (value->len == 0) {
        return NGX_OK;
    }

    ngx_memzero(&conf_file, sizeof(ngx_conf_file_t));

    ngx_memzero(&b, sizeof(ngx_buf_t));

    b.start = value->data;
    b.pos = value->data;
    b.last = value->data + value->len;
    b.end = b.last;
    b.temporary = 1;

    conf_file.file.fd = NGX_INVALID_FILE;
    conf_file.file.name.data = NULL;
    conf_file.line = 0;

    if (cf != NULL) {
        style = *cf;
    } else {
        style.args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
        if (style.args == NULL) {
            return NGX_ERROR;
        }
        style.pool = r->pool;
    }
    style.conf_file = &conf_file;
    style.conf_file->buffer = &b;

    style.handler = ngx_http_gm_gm;
    style.handler_conf = conf;

    if (ngx_conf_parse(&style, NULL) != NGX_CONF_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


static char *
ngx_http_gm_style(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_gm_conf_t                *gmcf = conf;

    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;
    
    ngx_str_t                         *value;

    dd("entering");

    /* compile style variable */
    gmcf->filter = 1;
    value = (ngx_str_t *)cf->args->elts + 1;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = value;
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cv.lengths != NULL) {
        gmcf->style_cv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (gmcf->style_cv == NULL) {
            return NGX_CONF_ERROR;
        }

        *gmcf->style_cv = cv;
        return NGX_CONF_OK;
    }
    
    /* parse config */
    if (ngx_http_gm_parse_style(cf, NULL, value, conf) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "gm filter: style \"%V\" parse error",
                value);
        return NGX_CONF_ERROR;
    }
    return NGX_CONF_OK;
}

static ngx_uint_t
ngx_http_gm_filter_value(ngx_str_t *value)
{
    ngx_int_t  n;

    if (value->len == 1 && value->data[0] == '-') {
        return (ngx_uint_t) -1;
    }

    n = ngx_atoi(value->data, value->len);

    if (n > 0) {
        return (ngx_uint_t) n;
    }

    return 0;
}

static char *
ngx_http_gm_quality(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    ngx_http_gm_conf_t                *gmcf = conf;

    ngx_str_t                         *value;
    ngx_int_t                          n;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    gmcf->filter = 1;
    value = cf->args->elts;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cv.lengths == NULL) {
        n = ngx_http_gm_filter_value(&value[1]);

        if (n <= 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid value \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

        gmcf->image_quality = (ngx_uint_t) n;

    } else {
        gmcf->qcv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (gmcf->qcv == NULL) {
            return NGX_CONF_ERROR;
        }

        *gmcf->qcv = cv;
    }

    return NGX_CONF_OK;
}

ngx_int_t
ngx_http_gm_cmp_int(const void *one, const void *two)
{
    const ngx_int_t           *a = one;
    const ngx_int_t           *b = two;

    return (*a < *b);
}

static char *
ngx_http_gm_statuses(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_gm_conf_t                *gmcf = conf;

    ngx_uint_t       i, n;
    ngx_int_t        status;
    ngx_str_t       *value;

    value = cf->args->elts;

    if (gmcf->filter_statuses) {
        return "is duplicate";
    }

    n = cf->args->nelts - 1;

    gmcf->filter_statuses = ngx_pnalloc(cf->pool, (n + 1) * sizeof(ngx_int_t));
    if (gmcf->filter_statuses == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 1; i <= n; i++) {
        status = ngx_atoi(value[i].data, value[i].len);
        if (status == NGX_ERROR) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                    "status code \"%V\" is an invalid number",
                    &value[i]);

            return NGX_CONF_ERROR;
        }

        if (status < 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                    "status code \"%V\" is not a positive number",
                    &value[i]);

            return NGX_CONF_ERROR;
        }

        gmcf->filter_statuses[i - 1] = status;
    }

    gmcf->filter_statuses[i - 1] = 0;

    ngx_sort(gmcf->filter_statuses, n, sizeof(ngx_int_t),
            ngx_http_gm_cmp_int);

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_gm_init_worker(ngx_cycle_t *cycle)
{
    InitializeMagick("logs");

    return NGX_OK;
}


static void
ngx_http_gm_exit_worker(ngx_cycle_t *cycle)
{
    DestroyMagick();
}


static ngx_int_t
ngx_http_gm_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_gm_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_gm_body_filter;


    return NGX_OK;
}

