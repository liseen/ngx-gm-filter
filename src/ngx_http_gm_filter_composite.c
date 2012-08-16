#include "ngx_http_gm_filter_module.h"
#include "ngx_http_gm_filter_composite.h"

ngx_int_t parse_composite_options(ngx_conf_t *cf, ngx_array_t *args, ngx_uint_t start, composite_options_t *option_info)
{

    ngx_str_t                         *value;
    ngx_uint_t                         i;
    ngx_uint_t                         end;
    ngx_str_t                          file;

    value = args->elts;
    end = args->nelts;

    /* init composite options */

    ngx_memzero(option_info, sizeof(composite_options_t));

    option_info->dissolve = 0.0;
    option_info->stegano = 0;
    option_info->stereo = 0;
    option_info->tile = 0;

    option_info->compose = OverCompositeOp;

    option_info->gravity = ForgetGravity;

    if (end < 3) {
        return NGX_ERROR;
    }

    for (i = 2; i < end - 1; ++i) {
        if (ngx_strcmp(value[i].data, "-gravity") == 0) {
            GravityType gravity = ForgetGravity;
            i++;
            if (i == end)
                return NGX_ERROR;

            gravity = StringToGravityType(value[i].data);
            if (gravity == ForgetGravity)
                return NGX_ERROR;

            option_info->gravity = gravity;
        } else if (ngx_strcmp(value[i].data, "-geometry") == 0) {
            i++;
            if (i == end)
                return NGX_ERROR;

            if (value[i].len > MaxTextExtent - 1)
                return NGX_ERROR;

            ngx_memcpy(option_info->geometry, value[i].data, value[i].len);
            option_info->geometry[value[i].len] = '\0';

            if (!IsGeometry(option_info->geometry)) {
                return NGX_ERROR;
            }
        } else if (ngx_strcmp(value[i].data, "-min-width") == 0) {
            i++;

            if (i == end)
                return NGX_ERROR;

            option_info->min_width = ngx_atoi(value[i].data, value[i].len);
        } else if (ngx_strcmp(value[i].data, "-min-height") == 0) {
            i++;

            if (i == end)
                return NGX_ERROR;

            option_info->min_height = ngx_atoi(value[i].data, value[i].len);

        } else {
            return NGX_ERROR;
        }
    }

    file = value[end - 1];
    if (ngx_conf_full_name(cf->cycle, &file, 1) != NGX_OK) {
        return NGX_ERROR;
    }

    option_info->composite_image_file = file;


    return NGX_OK;
}

ngx_int_t composite_image(ngx_http_request_t *r, composite_options_t *option_info, Image **image)
{
    char             composite_geometry[MaxTextExtent];
    MagickPassFail   status;
    RectangleInfo    geometry;
    ExceptionInfo    exception;

    ImageInfo       *image_info;
    Image            *composite_image;

    dd("entering");

    status = MagickPass;

    geometry.x=0;
    geometry.y=0;

    composite_image = option_info->composite_image;

    if (composite_image == NULL) {
        image_info = CloneImageInfo((ImageInfo *) NULL);

        ngx_memcpy(image_info->filename, option_info->composite_image_file.data,
                option_info->composite_image_file.len);

        image_info->filename[option_info->composite_image_file.len] = '\0';

        dd("composite_image filename:\"%s\"", image_info->filename);

        GetExceptionInfo(&exception);
        composite_image = ReadImage(image_info, &exception);
        if (composite_image == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: read composite image failed, severity: %O reason: %s, description: %s",
                        exception.severity, exception.reason, exception.description);
        }

        option_info->composite_image = composite_image;

        DestroyImageInfo(image_info);
        DestroyExceptionInfo(&exception);
    }

    if (composite_image == NULL) {
        return NGX_ERROR;
    }


    (void) GetGeometry(option_info->geometry, &geometry.x, &geometry.y,
        &geometry.width, &geometry.height);

    FormatString(composite_geometry, "%lux%lu%+ld%+ld",
        composite_image->columns, composite_image->rows,geometry.x,
        geometry.y);

    (*image)->gravity=option_info->gravity;
    (void) GetImageGeometry(*image, composite_geometry, 0, &geometry);

    if ((*image)->columns >= option_info->min_width &&
            (*image)->rows >= option_info->min_height) {

        GetExceptionInfo(&exception);

        status = CompositeImage(*image, option_info->compose, composite_image,
            geometry.x, geometry.y);

        if (status == MagickFail) {
            GetImageException(*image, &exception);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: composite image failed, severity: %O reason: %s, description: %s",
                        exception.severity, exception.reason, exception.description);
            DestroyExceptionInfo(&exception);

            return NGX_ERROR;
        }

        DestroyExceptionInfo(&exception);
    }

    return NGX_OK;
}
