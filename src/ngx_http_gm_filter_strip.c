#include "ngx_http_gm_filter_module.h"

/* strip image */
ngx_int_t 
gm_strip_image(ngx_http_request_t *r, void *option, Image **image)
{
    dd("starting strip");

    (void) StripImage(*image);

    return NGX_OK;
}

