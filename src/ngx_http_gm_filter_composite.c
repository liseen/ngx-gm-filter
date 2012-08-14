#include "ngx_http_gm_filter_module.h"
#include "ngx_http_gm_filter_composite.h"

ngx_uint_t parse_composite_options(ngx_pool_t * p, ngx_array_t *args, ngx_uint_t start, composite_options_t *option_info)
{

    ngx_str_t                         *value;
    ngx_uint_t                         i;

    value = args->elts;

    /* init composite options */

    ngx_memzero(option_info, sizeof(composite_options_t));

    option_info->dissolve = 0.0;
    option_info->stegano = 0;
    option_info->stereo = 0;
    option_info->tile = 0;

    option_info->compose=OverCompositeOp;

    option_info->gravity = ForgetGravity;

    for (i = 2; i < args->nelts; ++i) {
        if (ngx_strcmp(value[i].data, "-gravity") == 0) {
            GravityType gravity = ForgetGravity;
            i++;
            if (i == args->nelts)
                return 1;

            gravity = StringToGravityType(value[i].data);
            if (gravity == ForgetGravity)
                return 1;

            option_info->gravity = gravity;
        } else if (ngx_strcmp(value[i].data, "-geometry") == 0) {
            i++;
            if (i == args->nelts)
                return 1;

            if (value[i].len > MaxTextExtent - 1)
                return 1;

            ngx_memcpy(option_info->geometry, value[i].data, value[i].len);
            option_info->geometry[value[i].len] = '\0';

            if (!IsGeometry(option_info->geometry)) {
                return 1;
            }
        } else {
            return 1;
        }
    }

    return 0;
}

ngx_uint_t composite_image(composite_options_t *option_info, Image *image)
{
    char  composite_geometry[MaxTextExtent];
    MagickPassFail status;


    RectangleInfo geometry;
    Image  *composite_image;
    ExceptionInfo exception;

    status = MagickPass;

    geometry.x=0;
    geometry.y=0;
    composite_image = option_info->composite_image;

    (void) GetGeometry(option_info->geometry, &geometry.x, &geometry.y,
        &geometry.width, &geometry.height);

    FormatString(composite_geometry, "%lux%lu%+ld%+ld",
        composite_image->columns, composite_image->rows,geometry.x,
        geometry.y);

    image->gravity=option_info->gravity;
    (void) GetImageGeometry(image, composite_geometry, 0, &geometry);

    status = CompositeImage(image, option_info->compose, composite_image,
        geometry.x, geometry.y);

    if (status == MagickFail) {
        GetImageException(image, &exception);
    }

    return NGX_OK;
}
