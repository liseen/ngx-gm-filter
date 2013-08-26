#include "ngx_http_gm_filter_module.h"

#define GM_ROTATE_FLAG_NONE ' '
#define GM_ROTATE_FLAG_WIDTH '>'
#define GM_ROTATE_FLAG_HEIGHT '<'

/* gm rotate param */
typedef struct {
    u_char                      flag;
    double                      degrees;
    ngx_http_complex_value_t    *degrees_cv;
}rotate_option_t;

/* parse degrees */
static ngx_int_t
parse_degress(u_char *data, size_t len, rotate_option_t *deg)
{
    if (ngx_strchr(data, GM_ROTATE_FLAG_WIDTH) != (char *) NULL) {
        deg->flag = GM_ROTATE_FLAG_WIDTH;
    } else if (ngx_strchr(data, GM_ROTATE_FLAG_HEIGHT) != (char *) NULL) {
        deg->flag = GM_ROTATE_FLAG_HEIGHT;
    } else {
        deg->flag = GM_ROTATE_FLAG_NONE;
    }

    deg->degrees = strtod((char *)data, (char **)NULL);
    return NGX_OK;
}

/* rotate option parse */
ngx_int_t 
gm_parse_rotate_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, void **option)
{
    rotate_option_t                   *deg;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    ngx_str_t                         *value;

    dd("entering");

    if (args->nelts - start <= 0) {
        return NGX_ERROR;
    }

    value = args->elts;
    value += start + 1;

    deg = ngx_palloc(cf->pool, sizeof(rotate_option_t));
    if (deg == NULL) {
        return NGX_ERROR;
    }
    ngx_memzero(deg, sizeof(rotate_option_t));

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = value;
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_ERROR;
    }

    if (cv.lengths == NULL) {
        parse_degress(value->data, value->len, deg);
    } else {
        deg->degrees_cv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (deg->degrees_cv == NULL) {
            return NGX_ERROR;
        }

        *deg->degrees_cv = cv;
    }

    *option = deg;
    return NGX_OK;
}

/* rotate image */
ngx_int_t 
gm_rotate_image(ngx_http_request_t *r, void *option, Image **image)
{
    ExceptionInfo                      exception;

    Image                             *rotate_image = NULL;
    u_char                            *rotate_degrees = NULL;

    rotate_option_t                 *deg = (rotate_option_t *)option;

    dd("starting rotate");

    rotate_degrees = ngx_http_gm_get_str_value(r, deg->degrees_cv, NULL);

    if (rotate_degrees != NULL) {
       if (parse_degress(rotate_degrees, ngx_strlen(rotate_degrees), deg) != NGX_OK) {
           ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                   "gm filter: rotate image, get rotate degrees failed");
           return NGX_ERROR;
       }
    }

    if (deg->degrees == 0) {
        return NGX_OK;
    }

    if (deg->flag == GM_ROTATE_FLAG_WIDTH) {
        if ((*image)->columns <= (*image)->rows) {
            return NGX_OK;
        }
    }

    if (deg->flag == GM_ROTATE_FLAG_HEIGHT) {
        if ((*image)->columns >= (*image)->rows) {
            return NGX_OK;
        }
    }
    
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: rotate image degrees \"%f\"", deg->degrees);

    GetExceptionInfo(&exception);


    rotate_image=RotateImage(*image, deg->degrees, &exception);
    if (rotate_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: rotate image failed, "
                "degrees: \"%f\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                deg->degrees, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = rotate_image;

    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
