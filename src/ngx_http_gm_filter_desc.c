#include "ngx_http_gm_filter_module.h"

/* image base info */
ngx_buf_t *
ngx_http_gm_image_json(ngx_http_request_t *r,  Image *image)
{
    size_t      len;
    ngx_buf_t  *b;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NULL;
    }

    b->memory = 1;
    b->last_buf = 1;

    ngx_http_clean_header(r);

    r->headers_out.status = NGX_HTTP_OK;
    ngx_str_set(&r->headers_out.content_type, "application/json");
    r->headers_out.content_type_lowcase = NULL;

    len = sizeof("{ \"Width\": , \"Height\": , \"FileSize\": , \"Orientation\": , \"Format\": \"jpeg\" } }" CRLF) - 1
          + 4 * NGX_SIZE_T_LEN;

    b->pos = ngx_pnalloc(r->pool, len);
    if (b->pos == NULL) {
        return NULL;
    }

    b->last = ngx_sprintf(b->pos,
                          "{ \"Width\": %uz,"
                          " \"Height\": %uz,"
                          " \"FileSize\": %uz,"
                          " \"Orientation\": %uz,"
                          " \"Format\": \"%s\" }" CRLF,
                          image->columns,image->rows,
                          GetBlobSize(image), image->orientation,
                          image->magick);

    return b;
}

/* image exif info */

#ifndef EXIF_ATTRI_KEY_TYPE
    #define EXIF_KEY_STRING "STRING"
    #define EXIF_KEY_INTEGER "INTEGER"
#endif

typedef struct {
    ngx_str_t        name;
    const char       *type;
}exif_attri_key;

typedef struct {
    exif_attri_key   *key;
    ngx_str_t        value;
}exif_attri;

/* image exif attribute key list */
static exif_attri_key exif_keys[] = {
    { ngx_string("Make"), EXIF_KEY_STRING }, 
    { ngx_string("Model"), EXIF_KEY_STRING }, 
    { ngx_string("Orientation"), EXIF_KEY_INTEGER },
    { ngx_string("DateTimeOriginal"), EXIF_KEY_STRING },
    { ngx_string("FNumber"), EXIF_KEY_STRING }, 
    { ngx_string("FocalLength"), EXIF_KEY_STRING }, 
    { ngx_string("ExposureTime"), EXIF_KEY_STRING }, 
    { ngx_string("ExposureBiasValue"), EXIF_KEY_STRING },
    { ngx_string("ISOSpeedRatings"), EXIF_KEY_INTEGER },
    { ngx_string("FocalLengthIn35mmFilm"), EXIF_KEY_INTEGER },
    { ngx_string("GPSInfo"), EXIF_KEY_STRING },
    { ngx_string("GPSLatitudeRef"), EXIF_KEY_STRING },
    { ngx_string("GPSLatitude"), EXIF_KEY_STRING },
    { ngx_string("GPSLongitudeRef"), EXIF_KEY_STRING },
    { ngx_string("GPSLongitude"), EXIF_KEY_STRING },
    { ngx_string("GPSAltitudeRef"), EXIF_KEY_STRING },
    { ngx_string("GPSAltitude"), EXIF_KEY_STRING },
    { ngx_string("GPSTimeStamp"), EXIF_KEY_STRING },
    { ngx_string("GPSImgDirectionRef"), EXIF_KEY_STRING },
    { ngx_string("GPSImgDirection"), EXIF_KEY_STRING },
    { ngx_null_string, NULL }
};

ngx_buf_t *
ngx_http_gm_image_exif_json(ngx_http_request_t *r,  Image *image)
{
    const ImageAttribute    *attribute;
    const char              *profile_name;
    size_t                  profile_length;
    const unsigned char     *profile_info;
    ImageProfileIterator    profile_iterator;

    u_char                  *access_key;

    exif_attri_key          *attri_key;
    ngx_array_t             *attris;
    exif_attri              *attri;

    size_t                  len = 0;
    ngx_buf_t               *b;
    ngx_uint_t              i;

    attris = ngx_array_create(r->pool, 36, sizeof(exif_attri));
    if (attris == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: alloc exif info failed.");
        return NULL;
    }
    
    profile_iterator=AllocateImageProfileIterator(image);
    while(NextImageProfile(profile_iterator,&profile_name,&profile_info,
                &profile_length) != MagickFail) {
        if (profile_length == 0)
            continue;

        if (ngx_strcmp(profile_name,"EXIF") == 0) {
            attri_key = exif_keys;

            for(/* void */; attri_key->name.len; attri_key++) {
                access_key = ngx_pcalloc(r->pool, attri_key->name.len + 6);

                ngx_sprintf(access_key, "EXIF:%s", attri_key->name.data);
                attribute=GetImageAttribute(image,(char *)access_key);

                if ((attribute != (const ImageAttribute *) NULL) && ngx_strcmp(attribute->value, "unknown")) {
                    attri = ngx_array_push(attris);

                    attri->key = attri_key;
                    attri->value.data = (u_char *)attribute->value;
                    attri->value.len = attribute->length;

                    len += attri_key->name.len;
                    len += attribute->length;
                    /* add "": "",*/
                    len += 7;

                    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                            "gm filter: url %V exif info %V=%V", 
                            &r->uri, &attri->key->name, &attri->value);
                }
            }
        }
    }
    DeallocateImageProfileIterator(profile_iterator);

    /* output json */
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: alloc exif buf failed.");
        return NULL;
    }

    b->memory = 1;
    b->last_buf = 1;

    ngx_http_clean_header(r);

    r->headers_out.status = NGX_HTTP_OK;
    ngx_str_set(&r->headers_out.content_type, "application/json");
    r->headers_out.content_type_lowcase = NULL;

    len += 4 + 2 * NGX_SIZE_T_LEN;

    b->pos = ngx_pnalloc(r->pool, len);
    if (b->pos == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "gm filter: alloc exif output buffer failed.");
        return NULL;
    }

    /* to json */
    b->last = ngx_sprintf(b->pos, "{ ");
    for (i = 0,  attri = attris->elts; attris->nelts > 0 && (i < attris->nelts - 1); ++i, attri++) {
        if (ngx_strcmp(attri->key->type, EXIF_KEY_STRING) == 0) {
            b->last = ngx_sprintf(b->last, "\"%V\": \"%V\",", &attri->key->name, &attri->value);
        }
        if (ngx_strcmp(attri->key->type, EXIF_KEY_INTEGER) == 0) {
            b->last = ngx_sprintf(b->last, "\"%V\": %V,", &attri->key->name, &attri->value);
        }
    }

    /* last attri item */
    if (attri->key != NULL) {
        if (ngx_strcmp(attri->key->type, EXIF_KEY_STRING) == 0) {
            b->last = ngx_sprintf(b->last, "\"%V\": \"%V\"", &attri->key->name, &attri->value);
        }
    }
    if (attri->key != NULL) {
        if (ngx_strcmp(attri->key->type, EXIF_KEY_INTEGER) == 0) {
            b->last = ngx_sprintf(b->last, "\"%V\": %V", &attri->key->name, &attri->value);
        }
    }

    b->last = ngx_sprintf(b->last, "%s",  " }" CRLF);

    return b; 
}
