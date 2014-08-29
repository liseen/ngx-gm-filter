#include "ngx_http_gm_filter_util.h"
#include "ngx_http_gm_filter_module.h"

static const static_magic
StaticMagic[] =
{
#define MAGIC(name,offset,magic) {name,(unsigned char *)magic,sizeof(magic)-1,offset}
  MAGIC("AVI", 0, "RIFF"),
  MAGIC("8BIMWTEXT", 0, "8\000B\000I\000M\000#"),
  MAGIC("8BIMTEXT", 0, "8BIM#"),
  MAGIC("8BIM", 0, "8BIM"),
  MAGIC("BMP", 0, "BA"),
  MAGIC("BMP", 0, "BM"),
  MAGIC("BMP", 0, "CI"),
  MAGIC("BMP", 0, "CP"),
  MAGIC("BMP", 0, "IC"),
  MAGIC("BMP", 0, "PI"),
  MAGIC("CALS", 21, "version: MIL-STD-1840"),
  MAGIC("CALS", 0, "srcdocid:"),
  MAGIC("CALS", 9, "srcdocid:"),
  MAGIC("CALS", 8, "rorient:"),
  MAGIC("CGM", 0, "BEGMF"),
  MAGIC("CIN", 0, "\200\052\137\327"),
  MAGIC("DCM", 128, "DICM"),
  MAGIC("DCX", 0, "\261\150\336\72"),
  MAGIC("DIB", 0, "\050\000"),
  MAGIC("DOT", 0, "digraph"),
  MAGIC("DPX", 0, "SDPX"),
  MAGIC("DPX", 0, "XPDS"),
  MAGIC("EMF", 40, "\040\105\115\106\000\000\001\000"),
  MAGIC("EPT", 0, "\305\320\323\306"),
  MAGIC("FAX", 0, "DFAX"),
  MAGIC("FIG", 0, "#FIG"),
  MAGIC("FITS", 0, "IT0"),
  MAGIC("FITS", 0, "SIMPLE"),
  MAGIC("FPX", 0, "\320\317\021\340"),
  MAGIC("GIF", 0, "GIF8"),
  MAGIC("GPLT", 0, "#!/usr/local/bin/gnuplot"),
  MAGIC("HDF", 1, "HDF"),
  MAGIC("HPGL", 0, "IN;"),
  MAGIC("HPGL", 0, "\033E\033"),
  MAGIC("HTML", 1, "HTML"),
  MAGIC("HTML", 1, "html"),
  MAGIC("ILBM", 8, "ILBM"),
  MAGIC("IPTCWTEXT", 0, "\062\000#\000\060\000=\000\042\000&\000#\000\060\000;\000&\000#\000\062\000;\000\042\000"),
  MAGIC("IPTCTEXT", 0, "2#0=\042&#0;&#2;\042"),
  MAGIC("IPTC", 0, "\034\002"),
  MAGIC("JNG", 0, "\213JNG\r\n\032\n"),
  MAGIC("JPEG", 0, "\377\330\377"),
  MAGIC("JPC", 0, "\377\117"),
  MAGIC("JP2", 4, "\152\120\040\040\015"),
  MAGIC("MAT", 0, "MATLAB 5.0 MAT-file,"),
  MAGIC("MIFF", 0, "Id=ImageMagick"),
  MAGIC("MIFF", 0, "id=ImageMagick"),
  MAGIC("MNG", 0, "\212MNG\r\n\032\n"),
  MAGIC("MPC", 0, "id=MagickCache"),
  MAGIC("MPEG", 0, "\000\000\001\263"),
  MAGIC("PCD", 2048, "PCD_"),
  MAGIC("PCL", 0, "\033E\033"),
  MAGIC("PCX", 0, "\012\002"),
  MAGIC("PCX", 0, "\012\005"),
  MAGIC("PDB", 60, "vIMGView"),
  MAGIC("PDF", 0, "%PDF-"),
  MAGIC("PFA", 0, "%!PS-AdobeFont-1.0"),
  MAGIC("PFB", 6, "%!PS-AdobeFont-1.0"),
  MAGIC("PGX", 0, "\050\107\020\115\046"),
  MAGIC("PICT", 522, "\000\021\002\377\014\000"),
  MAGIC("PNG", 0, "\211PNG\r\n\032\n"),
  MAGIC("PBM", 0, "P1"),
  MAGIC("PGM", 0, "P2"),
  MAGIC("PPM", 0, "P3"),
  MAGIC("PBM", 0, "P4"),
  MAGIC("PGM", 0, "P5"),
  MAGIC("PPM", 0, "P6"),
  MAGIC("P7", 0, "P7 332"), /* XV Thumbnail */
  MAGIC("PAM", 0, "P7"), /* Should be listed after "P7 332" */
  MAGIC("PS", 0, "%!"),
  MAGIC("PS", 0, "\004%!"),
  MAGIC("PS", 0, "\305\320\323\306"),
  MAGIC("PSD", 0, "8BPS"),
  MAGIC("PWP", 0, "SFW95"),
  MAGIC("RAD", 0, "#?RADIANCE"),
  MAGIC("RAD", 0, "VIEW= "),
  MAGIC("RLE", 0, "\122\314"),
  MAGIC("SCT", 0, "CT"),
  MAGIC("SFW", 0, "SFW94"),
  MAGIC("SGI", 0, "\001\332"),
  MAGIC("SUN", 0, "\131\246\152\225"),
  MAGIC("SVG", 1, "?XML"),
  MAGIC("SVG", 1, "?xml"),
  MAGIC("TIFF", 0, "\115\115\000\052"),
  MAGIC("TIFF", 0, "\111\111\052\000"),
  MAGIC("BIGTIFF", 0, "\115\115\000\053\000\010\000\000"),
  MAGIC("BIGTIFF", 0, "\111\111\053\000\010\000\000\000"),
  MAGIC("VICAR", 0, "LBLSIZE"),
  MAGIC("VICAR", 0, "NJPL1I"),
  MAGIC("VIFF", 0, "\253\001"),
  MAGIC("WMF", 0, "\327\315\306\232"),
  MAGIC("WMF", 0, "\001\000\011\000"),
  MAGIC("WPG", 0, "\377WPC"),
  MAGIC("XBM", 0, "#define"),
  MAGIC("XCF", 0, "gimp xcf"),
  MAGIC("XPM", 1, "* XPM *"),
  MAGIC("XWD", 4, "\007\000\000"),
  MAGIC("XWD", 5, "\000\000\007")
};

ngx_int_t
get_image_format(const unsigned char *header, const size_t header_length, const static_magic **magic)
{
    unsigned int i;
    ngx_int_t status;
    const static_magic *m;

    status = NGX_ERROR;

    if(!((header == NULL) || (header_length == 0)))
    {
        /* Search for requested magic. */
        for (i=0; i < sizeof(StaticMagic)/sizeof(StaticMagic[0]); i++)
        {
            m = &StaticMagic[i];
            if ((header[m->offset] == m->magic[0]) &&
                    (memcmp(header+m->offset,m->magic, ngx_max(m->length,header_length)) == 0))
            {
                if (header_length >= m->length) {
                    *magic = m;
                    status = NGX_OK;
                    break;
                } else {
                    /* continue to check */
                    status = NGX_AGAIN;
                }
            }
        }
    }
    return status;
}

ngx_str_t
desc_image(Image *image)
{
    ngx_str_t desc = ngx_null_string;
    return desc;
}


