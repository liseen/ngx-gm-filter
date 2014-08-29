#include "ngx_http_gm_filter_module.h"

/* auto orient image */
ngx_int_t 
gm_auto_orient_image(ngx_http_request_t *r, void *option, Image **image)
{
    ExceptionInfo                      exception;

    Image                             *orient_image = NULL;

    dd("starting auto orient");

    GetExceptionInfo(&exception);
    orient_image=AutoOrientImage(*image,(*image)->orientation, &exception);

    if (orient_image == (Image *) NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: auto orient image failed, "
                "severity: \"%O\" "
                "reason: \"%s\", description: \"%s\"",
                exception.severity, exception.reason, exception.description);

        DestroyExceptionInfo(&exception);

        return NGX_ERROR;
    }

    DestroyImage(*image);
    *image = orient_image;
    DestroyExceptionInfo(&exception);

    return NGX_OK;
}

