#include "ngx_http_gm_filter_module.h"

/* thumbnail image for low cost*/
ngx_int_t 
gm_thumbnail_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *thumbnail_image = NULL;
    u_char                            *thumbnail_geometry = NULL;

    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting thumbnail");

    thumbnail_geometry = ngx_http_gm_get_str_value(r, geo->geometry_cv, &geo->geometry);

    if (thumbnail_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: thumbnail image, get thumbnail geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)thumbnail_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: thumbnail image, get thumbnail geometry %u format error", thumbnail_geometry);
        return  NGX_ERROR;
    }

    (void) GetImageGeometry(*image, (char *)thumbnail_geometry, 1,
            &geometry);

    if ((geometry.width == (*image)->columns) &&
            (geometry.height == (*image)->rows)) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: thumbnail image geometry: \"%s\"", thumbnail_geometry);

    GetExceptionInfo(&exception);
    thumbnail_image = ThumbnailImage(*image, geometry.width, geometry.height, &exception);

    if (thumbnail_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: thumbnail image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                thumbnail_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = thumbnail_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
