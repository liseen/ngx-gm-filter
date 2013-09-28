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

    len = sizeof("{ \"img\" : "
                 "{ \"width\": , \"height\": , \"type\": \"jpeg\" } }" CRLF) - 1
          + 2 * NGX_SIZE_T_LEN;

    b->pos = ngx_pnalloc(r->pool, len);
    if (b->pos == NULL) {
        return NULL;
    }

    b->last = ngx_sprintf(b->pos,
                          "{ \"img\" : "
                                       "{ \"width\": %uz,"
                                        " \"height\": %uz,"
                                        " \"type\": \"%s\" } }" CRLF,
                          image->columns,image->rows,
                          image->magick);

    return b;
}

/* image exif info */
typedef struct {
    ngx_str_t   name;
    ngx_str_t   value;
}exif_attri;

/* image exif attribute key list */
static ngx_str_t exif_keys[] = {
    ngx_string("Make"),
    ngx_string("Model"),
    ngx_string("Orientation"),
    ngx_string("DateTimeOriginal"),
    ngx_string("FNumber"),
    ngx_string("FocalLength"),
    ngx_string("ExposureTime"),
    ngx_string("ExposureBiasValue"),
    ngx_string("ISOSpeedRatings"),
    ngx_string("FocalLengthIn35mmFilm"),
    ngx_string("GPSInfo"),
    ngx_string("GPSLatitudeRef"),
    ngx_string("GPSLatitude"),
    ngx_string("GPSLongitudeRef"),
    ngx_string("GPSLongitude"),
    ngx_string("GPSAltitudeRef"),
    ngx_string("GPSAltitude"),
    ngx_string("GPSTimeStamp"),
    ngx_string("GPSImgDirectionRef"),
    ngx_string("GPSImgDirection"),
    ngx_null_string
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

    ngx_str_t               *attri_key;
    ngx_array_t             *attris;
    exif_attri              *attri;

    size_t      len;
    ngx_buf_t  *b;

    attris = ngx_array_create(r->pool, sizeof(exif_attri), 12);
    if (attris == NULL) {

        return NULL;
    }
    
    /* get exif attribute */
    profile_iterator=AllocateImageProfileIterator(image);
    while(NextImageProfile(profile_iterator,&profile_name,&profile_info,
                &profile_length) != MagickFail)
    {
        if (profile_length == 0)
            continue;

        if (ngx_strcmp(profile_name,"EXIF") == 0)
        {
            attri_key = exif_keys;

            for(/* void */; attri_key->len; attri_key++) {
                access_key = ngx_palloc(r->pool, attri_key->len + 5);

                ngx_sprintf(access_key, "EXIF:%s", attri_key->data);

                attribute=GetImageAttribute(image,(char *)access_key);
                if (attribute != (const ImageAttribute *) NULL)
                {
                    //log

                    attri = ngx_array_push(attris);
                    attri->name.len = attri_key->len;
                    attri->name.data = attri_key->data;

                    attri->value.data = (u_char *)attribute->value;
                    attri->value.len = attribute->length;

                    len += attri_key->len;
                    len += attribute->length;
                }
            }
        }
    }
    DeallocateImageProfileIterator(profile_iterator);

    /* output json */
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

    len = sizeof("{ \"exif\" : "
                 "{ \"width\": , \"height\": , \"type\": \"jpeg\" } }" CRLF) - 1
          + 2 * NGX_SIZE_T_LEN;

    b->pos = ngx_pnalloc(r->pool, len);
    if (b->pos == NULL) {
        
        return NULL;
    }

    b->last = ngx_sprintf(b->pos, "{ \"exif\" : {");

    ngx_uint_t      i;
    for (i = 0,  attri = attris->elts; i < attris->nelts; ++i) {
        b->last = ngx_sprintf(b->last, "{ \"%V\": %uV,", attri->name,attri->value);
    }
    b->last = ngx_sprintf(b->last, "%s", " }" CRLF);

    return b; 
}
