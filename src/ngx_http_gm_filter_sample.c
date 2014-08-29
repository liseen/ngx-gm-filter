#include "ngx_http_gm_filter_module.h"

/* sample image */
ngx_int_t 
gm_sample_image(ngx_http_request_t *r, void *option, Image **image)
{
    RectangleInfo                      geometry;
    ExceptionInfo                      exception;

    Image                             *sample_image = NULL;
    u_char                            *sample_geometry = NULL;

    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting sample");

    sample_geometry = ngx_http_gm_get_str_value(r, geo->geometry_cv, &geo->geometry);

    if (sample_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: sample image, get sample geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)sample_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: sample image, get sample geometry %u format error", sample_geometry);
        return  NGX_ERROR;
    }

    (void) GetImageGeometry(*image, (char *)sample_geometry, 1,
            &geometry);

    if ((geometry.width == (*image)->columns) &&
            (geometry.height == (*image)->rows)) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: sample image geometry: \"%s\"", sample_geometry);

    GetExceptionInfo(&exception);
    sample_image = SampleImage(*image, geometry.width, geometry.height, &exception);

    if (sample_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: sample image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                sample_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = sample_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}
