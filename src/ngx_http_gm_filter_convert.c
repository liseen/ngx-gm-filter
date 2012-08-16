#include "ngx_http_gm_filter_convert.h"

ngx_int_t parse_convert_options(ngx_pool_t *p, ngx_array_t *args, ngx_uint_t start, convert_options_t *option_info)
{
    ngx_http_gm_command_t             *gm_cmd;
    ngx_http_gm_convert_option_t      *gm_option;
    ngx_str_t                         *gm_arg;

    ngx_uint_t                         i;
    ngx_uint_t                         end;
    ngx_str_t                         *value;
    ngx_array_t                       *options;

    dd("entering");

    value = args->elts;

    option_info->options = ngx_array_create(p, 1, sizeof(ngx_http_gm_convert_option_t));
    if (option_info->options == NULL) {
        return NGX_ERROR;
    }

    options = option_info->options;

    gm_option == NULL;
    end = args->nelts;

    for (i = 2; i < end; ++i) {
        if (ngx_strncmp(value[i].data, "-resize", 1) == 0) {
            gm_option = ngx_array_push(options);
            if (gm_option == NULL) {
                return NGX_ERROR;
            }

            gm_option->type = NGX_HTTP_GM_RESIZE_OPTION;

            i++;
            if (i == end) {
                return NGX_ERROR;
            }

            ngx_memcpy(gm_option->resize_geometry, value[i].data, value[i].len);
            gm_option->resize_geometry[value[i].len] = '\0';

        } else {
        }
    }

    return NGX_OK;
}


ngx_int_t convert_image(ngx_http_request_t *r, convert_options_t *option_info, Image **image)
{
    ngx_int_t i;
    ngx_http_gm_convert_option_t *options;
    ngx_http_gm_convert_option_t *option;
    RectangleInfo geometry;
    ExceptionInfo exception;
    Image *resize_image = NULL;

    dd("entering");

    options = option_info->options->elts;

    for (i = 0; i < option_info->options->nelts; ++i) {
        option = &options[i];

        if (option->type == NGX_HTTP_GM_RESIZE_OPTION) {
            dd("starting resize");

            (void) GetImageGeometry(*image, option->resize_geometry, 1, &geometry);

            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
                continue;

            GetExceptionInfo(&exception);
            resize_image=ResizeImage(*image, geometry.width, geometry.height,
                (*image)->filter,(*image)->blur, &exception);

            if (resize_image == (Image *) NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: resize image failed, severity: %O reason: %s, description: %s",
                        exception.severity, exception.reason, exception.description);

                DestroyExceptionInfo(&exception);
                return NGX_ERROR;
            }

            DestroyImage(*image);

            *image = resize_image;

            DestroyExceptionInfo(&exception);
            continue;
        } else {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: convert command, unkonwn option");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}
