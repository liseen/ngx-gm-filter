/* vim:set ft=c ts=4 sw=4 et fdm=marker: */

#include "ddebug.h"
/* TODO */
#include "ngx_http_gm_filter_module.h"



static ngx_int_t ngx_http_gm_image_send(ngx_http_request_t *r,
    ngx_http_gm_ctx_t *ctx, ngx_chain_t *in);

static ngx_uint_t ngx_http_gm_image_test(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_gm_image_read(ngx_http_request_t *r,
    ngx_chain_t *in);

static ngx_buf_t *ngx_http_gm_image_process(ngx_http_request_t *r);

static void ngx_http_gm_image_length(ngx_http_request_t *r,
    ngx_buf_t *b);

static ngx_int_t ngx_http_gm_image_size(ngx_http_request_t *r,
    ngx_http_gm_ctx_t *ctx);

static ngx_buf_t *
ngx_http_gm_image_json(ngx_http_request_t *r, ngx_http_gm_ctx_t *ctx);


static ngx_uint_t ngx_http_gm_get_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *cv, ngx_uint_t v);
static ngx_uint_t ngx_http_gm_value(ngx_str_t *value);

static void *ngx_http_gm_create_conf(ngx_conf_t *cf);
static char *ngx_http_gm_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);

static char *ngx_http_gm_gm(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static ngx_int_t ngx_http_gm_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_gm_commands[] = {

    { ngx_string("gm"),
      NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_gm_gm,
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
    NULL,                           /* init process */
    NULL,                           /* init thread */
    NULL,                           /* exit thread */
    NULL,                           /* exit process */
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
    ngx_http_gm_ctx_t   *ctx;
    ngx_http_gm_conf_t  *conf;

    if (r->headers_out.status == NGX_HTTP_NOT_MODIFIED) {
        return ngx_http_next_header_filter(r);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);

    if (ctx) {
        ngx_http_set_ctx(r, NULL, ngx_http_gm_module);
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

    if (!conf->cmds) {
        return ngx_http_next_header_filter(r);
    }

    if (r->headers_out.content_type.len
            >= sizeof("multipart/x-mixed-replace") - 1
        && ngx_strncasecmp(r->headers_out.content_type.data,
                           (u_char *) "multipart/x-mixed-replace",
                           sizeof("multipart/x-mixed-replace") - 1)
           == 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "image filter: multipart/x-mixed-replace response");

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
                      "image filter: too big response: %O", len);

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
    ngx_http_gm_conf_t  *conf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "image filter");

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

        conf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

        if (ctx->type == NGX_HTTP_GM_IMAGE_NONE) {
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
                   "image filter: \"%c%c\"", p[0], p[1]);

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

    if (ctx->image == NULL) {
        ctx->image = ngx_palloc(r->pool, ctx->length);
        if (ctx->image == NULL) {
            return NGX_ERROR;
        }

        ctx->last = ctx->image;
    }

    p = ctx->last;

    for (cl = in; cl; cl = cl->next) {

        b = cl->buf;
        size = b->last - b->pos;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "image buf: %uz", size);

        rest = ctx->image + ctx->length - p;
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
    ngx_int_t                      rc;
    ngx_http_gm_ctx_t   *ctx;
    ngx_http_gm_conf_t  *conf;

    r->connection->buffered &= ~NGX_HTTP_IMAGE_BUFFERED;

    ctx = ngx_http_get_module_ctx(r, ngx_http_gm_module);

    rc = ngx_http_gm_image_size(r, ctx);

    conf = ngx_http_get_module_loc_conf(r, ngx_http_gm_module);

    return ngx_http_gm_image_json(r, rc == NGX_OK ? ctx : NULL);
}


static ngx_buf_t *
ngx_http_gm_image_json(ngx_http_request_t *r, ngx_http_gm_ctx_t *ctx)
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
    ngx_str_set(&r->headers_out.content_type, "text/plain");
    r->headers_out.content_type_lowcase = NULL;

    if (ctx == NULL) {
        b->pos = (u_char *) "{}" CRLF;
        b->last = b->pos + sizeof("{}" CRLF) - 1;

        ngx_http_gm_image_length(r, b);

        return b;
    }

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
                          ctx->width, ctx->height,
                          ngx_http_gm_image_types[ctx->type - 1].data + 6);

    ngx_http_gm_image_length(r, b);

    return b;
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


static ngx_int_t
ngx_http_gm_image_size(ngx_http_request_t *r, ngx_http_gm_ctx_t *ctx)
{
    u_char      *p, *last;
    size_t       len, app;
    ngx_uint_t   width, height;

    p = ctx->image;

    switch (ctx->type) {

    case NGX_HTTP_GM_IMAGE_JPEG:

        p += 2;
        last = ctx->image + ctx->length - 10;
        width = 0;
        height = 0;
        app = 0;

        while (p < last) {

            if (p[0] == 0xff && p[1] != 0xff) {

                ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "JPEG: %02xd %02xd", p[0], p[1]);

                p++;

                if ((*p == 0xc0 || *p == 0xc1 || *p == 0xc2 || *p == 0xc3
                     || *p == 0xc9 || *p == 0xca || *p == 0xcb)
                    && (width == 0 || height == 0))
                {
                    width = p[6] * 256 + p[7];
                    height = p[4] * 256 + p[5];
                }

                ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "JPEG: %02xd %02xd", p[1], p[2]);

                len = p[1] * 256 + p[2];

                if (*p >= 0xe1 && *p <= 0xef) {
                    /* application data, e.g., EXIF, Adobe XMP, etc. */
                    app += len;
                }

                p += len;

                continue;
            }

            p++;
        }

        if (width == 0 || height == 0) {
            return NGX_DECLINED;
        }

        if (ctx->length / 20 < app) {
            /* force conversion if application data consume more than 5% */
            ctx->force = 1;
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "app data size: %uz", app);
        }

        break;

    case NGX_HTTP_GM_IMAGE_GIF:

        if (ctx->length < 10) {
            return NGX_DECLINED;
        }

        width = p[7] * 256 + p[6];
        height = p[9] * 256 + p[8];

        break;

    case NGX_HTTP_GM_IMAGE_PNG:

        if (ctx->length < 24) {
            return NGX_DECLINED;
        }

        width = p[18] * 256 + p[19];
        height = p[22] * 256 + p[23];

        break;

    default:

        return NGX_DECLINED;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "image size: %d x %d", width, height);

    ctx->width = width;
    ctx->height = height;

    return NGX_OK;
}

static ngx_uint_t
ngx_http_gm_get_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *cv, ngx_uint_t v)
{
    ngx_str_t  val;

    if (cv == NULL) {
        return v;
    }

    if (ngx_http_complex_value(r, cv, &val) != NGX_OK) {
        return 0;
    }

    return ngx_http_gm_value(&val);
}


static ngx_uint_t
ngx_http_gm_value(ngx_str_t *value)
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


static void *
ngx_http_gm_create_conf(ngx_conf_t *cf)
{
    ngx_http_gm_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_gm_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->buffer_size = NGX_CONF_UNSET_SIZE;

    return conf;
}


static char *
ngx_http_gm_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_gm_conf_t *prev = parent;
    ngx_http_gm_conf_t *conf = child;

    if (!conf->cmds && prev->cmds) {
        conf->cmds = prev->cmds;
    }

    ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size,
                              4 * 1024 * 1024);
    return NGX_CONF_OK;
}


static char *
ngx_http_gm_gm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_gm_conf_t *gmcf = conf;

    ngx_str_t                         *value;
    ngx_http_gm_cmd                   *gm_cmd;
    ngx_int_t                          n;
    ngx_uint_t                         i;

    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    ngx_array_t                     *args;

    args  = cf->args;
    value = cf->args->elts;

    i = 1;

    if (args->nelts < 2) {
        return NGX_CONF_ERROR;
    }

    if (!gmcf->cmds) {
        gmcf->cmds = ngx_array_create(cf->pool, 1, sizeof(ngx_http_gm_cmd));
        if (gmcf->cmds == NULL) {
            goto failed;
        }
        gm_cmd = gmcf->cmds->elts;
    } else {
        gm_cmd = ngx_array_push(gmcf->cmds);
        if (gm_cmd == NULL) {
            goto failed;
        }
    }

    if (ngx_strcmp(value[i].data, "convert") == 0) {
        gm_cmd->type = NGX_HTTP_GM_CONVERT_CMD;
    } else if (ngx_strcmp(value[i].data, "composite") == 0) {
        gm_cmd->type = NGX_HTTP_GM_COMPOSITE_CMD;
    } else {
        goto failed;
    }

    for (i = 2; i < args->nelts; ++i) {
    }

    return NGX_CONF_OK;

failed:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"",
                       &value[i]);

    return NGX_CONF_ERROR;
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
