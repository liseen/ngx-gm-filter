#include "ngx_http_gm_filter_module.h"

/* scale image */
ngx_int_t 
gm_scale_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *scale_image = NULL;
    u_char                            *scale_geometry = NULL;

    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting scale");

    scale_geometry = ngx_http_gm_get_str_value(r, geo->geometry_cv, &geo->geometry);

    if (scale_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: scale image, get scale geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)scale_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: scale image, get scale geometry %u format error", scale_geometry);
        return  NGX_ERROR;
    }

    (void) GetImageGeometry(*image, (char *)scale_geometry, 1,
            &geometry);

    if ((geometry.width == (*image)->columns) &&
            (geometry.height == (*image)->rows)) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: scale image geometry: \"%s\"", scale_geometry);

    GetExceptionInfo(&exception);
    scale_image = ScaleImage(*image, geometry.width, geometry.height, &exception);

    if (scale_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: scale image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                scale_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = scale_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
