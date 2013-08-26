#include "ngx_http_gm_filter_module.h"

/* crop image */
ngx_int_t 
gm_crop_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *crop_image = NULL;
    u_char                            *crop_geometry = NULL;
    
    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting crop");

    crop_geometry = ngx_http_gm_get_str_value(r,
            geo->geometry_cv, &geo->geometry);

    if (crop_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: crop image, get crop geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)crop_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: crop image, get crop geometry %u format error", crop_geometry);
        return  NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: crop image geometry: \"%s\"", crop_geometry);

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
