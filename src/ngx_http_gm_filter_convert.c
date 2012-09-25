#include "ngx_http_gm_filter_convert.h"

static u_char * ngx_http_gm_get_str_value(ngx_http_request_t *r,
    ngx_http_complex_value_t *cv, ngx_str_t *val);

ngx_int_t
parse_convert_options(ngx_conf_t *cf, ngx_array_t *args,
    ngx_uint_t start, convert_options_t *option_info)
{
    ngx_http_gm_convert_option_t      *gm_option;

    ngx_uint_t                         i;
    ngx_uint_t                         end;
    ngx_str_t                         *value;
    ngx_array_t                       *options;

    ngx_http_complex_value_t           resize_cv;
    ngx_http_compile_complex_value_t   resize_ccv;

    ngx_http_complex_value_t           crop_cv;
    ngx_http_compile_complex_value_t   crop_ccv;

    ngx_http_complex_value_t           rotate_cv;
    ngx_http_compile_complex_value_t   rotate_ccv;

    dd("entering");

    value = args->elts;

    option_info->options = ngx_array_create(cf->pool, 1,
                                        sizeof(ngx_http_gm_convert_option_t));

    if (option_info->options == NULL) {
        return NGX_ERROR;
    }

    options = option_info->options;

    gm_option = NULL;
    end = args->nelts;

    for (i = 2; i < end; ++i) {
        if (ngx_strncmp(value[i].data, "-resize", value[i].len) == 0) {
            gm_option = ngx_array_push(options);
            if (gm_option == NULL) {
                return NGX_ERROR;
            }

            gm_option->type = NGX_HTTP_GM_RESIZE_OPTION;
            gm_option->resize_geometry_cv = NULL;

            i++;
            if (i == end) {
                return NGX_ERROR;
            }

            ngx_memzero(&resize_ccv, sizeof(ngx_http_compile_complex_value_t));

            resize_ccv.cf = cf;
            resize_ccv.value = &value[i];
            resize_ccv.complex_value = &resize_cv;

            if (ngx_http_compile_complex_value(&resize_ccv) != NGX_OK) {
                return NGX_ERROR;
            }

            if (resize_cv.lengths == NULL) {

                if (value[i].len > MaxTextExtent - 1) {
                    return NGX_ERROR;
                }

                gm_option->resize_geometry = value[i];

            } else {
                gm_option->resize_geometry_cv = ngx_palloc(cf->pool,
                        sizeof(ngx_http_complex_value_t));

                if (gm_option->resize_geometry_cv == NULL) {
                    return NGX_ERROR;
                }

                *gm_option->resize_geometry_cv = resize_cv;
            }

        } else if (ngx_strncmp(value[i].data, "-rotate", value[i].len) == 0) {
            gm_option = ngx_array_push(options);
            if (gm_option == NULL) {
                return NGX_ERROR;
            }

            gm_option->type = NGX_HTTP_GM_ROTATE_OPTION;
            gm_option->rotate_degrees_cv = NULL;

            i++;
            if (i == end) {
                return NGX_ERROR;
            }

            ngx_memzero(&rotate_ccv, sizeof(ngx_http_compile_complex_value_t));

            rotate_ccv.cf = cf;
            rotate_ccv.value = &value[i];
            rotate_ccv.complex_value = &rotate_cv;

            if (ngx_http_compile_complex_value(&rotate_ccv) != NGX_OK) {
                return NGX_ERROR;
            }

            if (rotate_cv.lengths == NULL) {

                if (value[i].len > MaxTextExtent - 1) {
                    return NGX_ERROR;
                }

                gm_option->rotate_degrees = value[i];
            } else {
                gm_option->rotate_degrees_cv = ngx_palloc(cf->pool,
                        sizeof(ngx_http_complex_value_t));

                if (gm_option->rotate_degrees_cv == NULL) {
                    return NGX_ERROR;
                }

                *gm_option->rotate_degrees_cv = rotate_cv;
            }

        } else if (ngx_strncmp(value[i].data, "-crop", value[i].len) == 0) {
            gm_option = ngx_array_push(options);
            if (gm_option == NULL) {
                return NGX_ERROR;
            }

            gm_option->type = NGX_HTTP_GM_CROP_OPTION;
            gm_option->crop_geometry_cv = NULL;

            i++;
            if (i == end) {
                return NGX_ERROR;
            }

            ngx_memzero(&crop_ccv, sizeof(ngx_http_compile_complex_value_t));

            crop_ccv.cf = cf;
            crop_ccv.value = &value[i];
            crop_ccv.complex_value = &crop_cv;

            if (ngx_http_compile_complex_value(&crop_ccv) != NGX_OK) {
                return NGX_ERROR;
            }

            if (crop_cv.lengths == NULL) {

                if (value[i].len > MaxTextExtent - 1) {
                    return NGX_ERROR;
                }

                gm_option->crop_geometry = value[i];

            } else {
                gm_option->crop_geometry_cv = ngx_palloc(cf->pool,
                        sizeof(ngx_http_complex_value_t));

                if (gm_option->crop_geometry_cv == NULL) {
                    return NGX_ERROR;
                }

                *gm_option->crop_geometry_cv = crop_cv;
            }


        } else {

        }
    }

    return NGX_OK;
}


ngx_int_t
convert_image(ngx_http_request_t *r, convert_options_t *option_info,
    Image **image)
{
    ngx_uint_t                          i;

    ngx_http_gm_convert_option_t      *options;
    ngx_http_gm_convert_option_t      *option;

    RectangleInfo                      geometry;
    RectangleInfo                      geometry_info;
    ExceptionInfo                      exception;

    Image                             *resize_image = NULL;
    u_char                            *resize_geometry = NULL;
    u_char                            *need_crop_resize = NULL;

    int                                crop_flags;
    Image                             *crop_image = NULL;
    u_char                            *crop_geometry = NULL;

    Image                             *rotate_image = NULL;
    u_char                            *rotate_degrees = NULL;
    ngx_int_t                          degrees;
    ngx_int_t                          degrees_suffix_len = 0;

    dd("entering");

    options = option_info->options->elts;

    for (i = 0; i < option_info->options->nelts; ++i) {
        option = &options[i];

        if (option->type == NGX_HTTP_GM_RESIZE_OPTION) {
            dd("starting resize");

            resize_geometry = ngx_http_gm_get_str_value(r,
                    option->resize_geometry_cv, &option->resize_geometry);

            need_crop_resize = (u_char *)ngx_strchr(resize_geometry, 'c');
            if (need_crop_resize != NULL) {
                *need_crop_resize = '^';
            }

            if (resize_geometry == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: resize image, get resize geometry failed");
                return  NGX_ERROR;
            }

            if ((ngx_strlen(resize_geometry) == 0) ||
                    (ngx_strncmp(resize_geometry, "no", 2)) == 0) {
                continue;
            }


            (void) GetImageGeometry(*image, (char *)resize_geometry, 1,
                                    &geometry);

            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
                continue;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "resize image geometry: \"%s\"", resize_geometry);

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

            if (need_crop_resize != NULL) {
                GetGeometry((char *)resize_geometry,
                            &geometry_info.x, &geometry_info.y,
                            (unsigned long *)(&geometry_info.width),
                            (unsigned long *)(&geometry_info.height));

                if (geometry_info.width > (*image)->columns ||
                        geometry_info.height > (*image)->rows) {
                    continue;
                }

                geometry_info.x = ((*image)->columns - geometry_info.width) / 2;
                geometry_info.y = ((*image)->rows - geometry_info.height) / 2;

                GetExceptionInfo(&exception);
                resize_image = CropImage(*image, &geometry_info, &exception);


                if (resize_image == (Image *) NULL) {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                "gm filter: resize croping image failed, "
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
            }

        } else if (option->type == NGX_HTTP_GM_ROTATE_OPTION) {
            dd("starting rotate");

            rotate_degrees = ngx_http_gm_get_str_value(r,
                    option->rotate_degrees_cv, &option->rotate_degrees);

            if (rotate_degrees == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: rotate image, get rotate degrees failed");
                return  NGX_ERROR;
            }

            if (ngx_strchr(rotate_degrees,'>') != (char *) NULL) {
                if ((*image)->columns <= (*image)->rows)
                    continue;
                degrees_suffix_len = 1;
            }

            if (ngx_strchr(rotate_degrees,'<') != (char *) NULL) {
                if ((*image)->columns >= (*image)->rows)
                    continue;
                degrees_suffix_len = 1;
            }

            degrees = 0;
            degrees = ngx_atoi(rotate_degrees,
                               ngx_strlen(rotate_degrees) - degrees_suffix_len);

            if (degrees == 0) {
                continue;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rotate image degrees: \"%O\"", degrees);

            GetExceptionInfo(&exception);


            rotate_image=RotateImage(*image, degrees, &exception);
            if (rotate_image == (Image *) NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "gm filter: rotate image failed, "
                              "degrees: \"%s\" severity: \"%O\" "
                              "reason: \"%s\", description: \"%s\"",
                              option->rotate_degrees, exception.severity,
                              exception.reason, exception.description);

                DestroyExceptionInfo(&exception);

                return NGX_ERROR;
            }

            DestroyImage(*image);
            *image = rotate_image;

            DestroyExceptionInfo(&exception);

        } else if (option->type == NGX_HTTP_GM_CROP_OPTION) {
            dd("starting crop");
            crop_geometry = ngx_http_gm_get_str_value(r,
                    option->crop_geometry_cv, &option->crop_geometry);

            if (crop_geometry == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "gm filter: crop image, get crop geometry failed");
                return  NGX_ERROR;
            }

            if ((ngx_strlen(crop_geometry) == 0) ||
                    (ngx_strncmp(crop_geometry, "no", 2)) == 0) {
                continue;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "crop image geometry: \"%s\"", crop_geometry);

            crop_flags = GetImageGeometry(*image, (char *)crop_geometry, 0,
                                    &geometry);

            if ((geometry.width == 0) || (geometry.height == 0) ||
                ((crop_flags & XValue) != 0) || ((crop_flags & YValue) != 0) ||
                (crop_flags & PercentValue)) {

                GetExceptionInfo(&exception);
	            crop_image = CropImage(*image, &geometry, &exception);

                if (crop_image == (Image *) NULL) {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                "gm filter: crop image failed, "
                                "arg: \"%s\" severity: \"%O\" "
                                "reason: \"%s\", description: \"%s\"",
                                crop_geometry, exception.severity,
                                exception.reason, exception.description);

                    DestroyExceptionInfo(&exception);

                    return NGX_ERROR;
                }

                DestroyImage(*image);
                *image = crop_image;
                DestroyExceptionInfo(&exception);
            } else {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "gm filter:  command, unkonwn option");
            }
        } else {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "gm filter: invalid crop geometry: \"%s\"", crop_geometry);
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static u_char *
ngx_http_gm_get_str_value(ngx_http_request_t *r, ngx_http_complex_value_t *cv,
    ngx_str_t *val)
{
    u_char      *buf;
    ngx_str_t  str;

    if (cv == NULL) {
        buf = ngx_pcalloc(r->pool, val->len + 1);
        if (buf == NULL) {
            return NULL;
        }

        ngx_memcpy(buf, val->data, val->len);
        buf[val->len] = '\0';

        return buf;
    } else {
        if (ngx_http_complex_value(r, cv, &str) != NGX_OK) {
            return NULL;
        }

        buf = ngx_pcalloc(r->pool, str.len + 1);
        if (buf == NULL) {
            return NULL;
        }

        ngx_memcpy(buf, str.data, str.len);
        buf[str.len] = '\0';

        return buf;
    }
}
