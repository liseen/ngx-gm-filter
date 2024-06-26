#include "ngx_http_gm_filter_module.h"
typedef struct _CropOptions {
    ngx_http_gm_geometry_t      geo;
    GravityType                 gravity;
    ngx_http_complex_value_t    *gravity_cv;
} crop_options_t;

ngx_int_t
parse_crop_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start,
    void **option)
{
    crop_options_t                    *option_info;
    ngx_str_t                         *value;
    ngx_uint_t                         i, rc;
    ngx_uint_t                         end;

    GravityType                        gravity;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    value = args->elts;
    end = args->nelts;

    dd("entering");

    if (end < 3) {
        return NGX_ERROR;
    }

    /* init crop options */
    option_info = ngx_palloc(cf->pool, sizeof(crop_options_t));
    if (option_info == NULL) {
        return NGX_ERROR;
    }

    *option = option_info;

    ngx_memzero(option_info, sizeof(crop_options_t));

    option_info->gravity = ForgetGravity;


    for (i = 2; i <= end - 1; ++i) {
        if (ngx_strcmp(value[i].data, "-gravity") == 0) {
            gravity = ForgetGravity;
            i++;
            if (i == end) {
                return NGX_ERROR;
            }
            ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));
            ccv.cf = cf;
            ccv.value = &value[i];
            ccv.complex_value = &cv;
            if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
                return NGX_ERROR;
            }

            if (cv.lengths == NULL) {

                gravity = StringToGravityType((char*)value[i].data);
                if (gravity == ForgetGravity) {
                    return NGX_ERROR;
                }

                option_info->gravity = gravity;
            } else {
                option_info->gravity_cv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
                if (option_info->gravity_cv == NULL) {
                    return NGX_ERROR;
                }

                *option_info->gravity_cv = cv;

            }
        } else {
            rc = ngx_http_gm_get_geometry_value(cf, &value[i], &option_info->geo);

            if (rc != NGX_OK) {
                return NGX_ERROR;
            }
        }
    }

    return NGX_OK;
}

/* crop image */
ngx_int_t 
gm_crop_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *crop_image = NULL;
    u_char                            *crop_geometry = NULL;
    u_char                            *crop_gravity = NULL;
    
    crop_options_t                    *crop_option = (crop_options_t *)option;

    dd("starting crop");

    //geometry
    crop_geometry = ngx_http_gm_get_str_value(r,
            crop_option->geo.geometry_cv, &crop_option->geo.geometry);

    if (crop_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: crop image, get crop geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)crop_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: crop image, get crop geometry %u format failed", crop_geometry);
        return  NGX_ERROR;
    }

    //gravity
    crop_gravity = ngx_http_gm_get_str_value(r,
            crop_option->gravity_cv, NULL);

    if (crop_gravity != NULL && ngx_strlen(crop_gravity)) {
        crop_option->gravity = StringToGravityType((char*)crop_gravity);
        if (crop_option->gravity == ForgetGravity) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "gm filter: crop image, get crop gravity failed");
            return NGX_ERROR;
        }
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: crop image geometry: \"%s\"", crop_geometry);

    (*image)->gravity=crop_option->gravity;

    (void) GetImageGeometry(*image, (char *)crop_geometry, 0, &geometry);

    GetExceptionInfo(&exception);
    crop_image = CropImage(*image, &geometry, &exception);

    if (crop_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: crop image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                crop_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = crop_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
