#include "ngx_http_gm_filter_module.h"

/* resize image */
ngx_int_t 
gm_resize_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *resize_image = NULL;
    u_char                            *resize_geometry = NULL;

    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting resize");

    resize_geometry = ngx_http_gm_get_str_value(r, geo->geometry_cv, &geo->geometry);

    if (resize_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: resize image, get resize geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)resize_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: resize image, get resize geometry %u format error", resize_geometry);
        return  NGX_ERROR;
    }

    (void) GetImageGeometry(*image, (char *)resize_geometry, 1,
            &geometry);

    if ((geometry.width == (*image)->columns) &&
            (geometry.height == (*image)->rows)) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: resize image geometry: \"%s\"", resize_geometry);

    GetExceptionInfo(&exception);
    resize_image = ResizeImage(*image, geometry.width, geometry.height,
            (*image)->filter,(*image)->blur, &exception);

    if (resize_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: resize image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                resize_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = resize_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
