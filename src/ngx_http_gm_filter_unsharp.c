#include "ngx_http_gm_filter_module.h"

/* unsharp image */
ngx_int_t 
gm_unsharp_image(ngx_http_request_t *r, void *option, Image **image)
{
    ExceptionInfo                      exception;

    Image                             *unsharp_image = NULL;
    u_char                            *unsharp_geometry = NULL;
    double                            amount, radius, sigma, threshold;

    ngx_http_gm_geometry_t            *geo = (ngx_http_gm_geometry_t *)option;

    dd("starting unsharp");

    unsharp_geometry = ngx_http_gm_get_str_value(r, geo->geometry_cv, &geo->geometry);

    if (unsharp_geometry == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: unsharp image, get unsharp geometry failed");
        return  NGX_ERROR;
    }

    if (!IsGeometry((const char *)unsharp_geometry)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: unsharp image, get unsharp geometry %u format error", unsharp_geometry);
        return  NGX_ERROR;
    }
    
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "gm filter: unsharp image geometry: \"%s\"", unsharp_geometry);

    GetExceptionInfo(&exception);

    /*
      Gaussian unsharpen image.
    */
    amount=1.0;
    radius=0.0;
    sigma=1.0;
    threshold=0.05;
    (void) GetMagickDimension((const char *)unsharp_geometry,&radius,&sigma,&amount,&threshold);

    unsharp_image=UnsharpMaskImage(*image,radius,sigma,amount,threshold,
              &(*image)->exception);
    
    if (unsharp_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: unsharp image failed, "
                "arg: \"%s\" severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                unsharp_geometry, exception.severity,
                exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = unsharp_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}

