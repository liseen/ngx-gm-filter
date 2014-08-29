# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";

log_level('debug'); # to ensure any log-level can be outputed

repeat_each(1);

plan tests => repeat_each() * (blocks() * 4);

run_tests();


__DATA__

=== TEST 1: exif jpg
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm exif;
    }
EOF
--- request
GET /identify/ginfo.jpg

--- response_headers
Content-Type: application/json
Content-Length: 556
--- response_body_like eval: '{ "Width": 905, "Height": 678, "FileSize": 303425, "Format": "JPEG", "Make": "Apple","Model": "iPhone 4S","Orientation": 1,"DateTimeOriginal": "2013:08:10 11:40:05","FNumber": "12/5","FocalLength": "107/25","ExposureTime": "1/986","ISOSpeedRatings": 50,"FocalLengthIn35mmFilm": 35,"GPSInfo": "594","GPSLatitudeRef": "N","GPSLatitude": "42/1,3164/100,0/1","GPSLongitudeRef": "E","GPSLongitude": "117/1,1380/100,0/1","GPSAltitudeRef": ".","GPSAltitude": "1535/1","GPSTimeStamp": "3/1,40/1,437/100","GPSImgDirectionRef": "T","GPSImgDirection": "18089/613" }'



=== TEST 2: exif jpg none
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm exif;
    }
EOF
--- request
GET /identify/1024_768.jpg

--- response_headers
Content-Type: application/json
Content-Length: 72
--- response_body_like eval: '{ "Width": 1024, "Height": 768, "FileSize": 597491, "Format": "JPEG" }'



=== TEST 3: identify jpg
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/1024_768.jpg
--- response_headers
Content-Type: application/json
Content-Length: 90
--- response_body_like eval: '{ "Width": 1024, "Height": 768, "FileSize": 597491, "Orientation": 0, "Format": "JPEG" }'



=== TEST 4: identify gif
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/paged.gif
--- response_headers
Content-Type: application/json
Content-Length: 84
--- response_body_like eval: '{ "Width": 46, "Height": 46, "FileSize": 3094, "Orientation": 0, "Format": "GIF" }'



=== TEST 5: identify png
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/wm.png
--- response_headers
Content-Type: application/json
Content-Length: 87
--- response_body_like eval: '{ "Width": 800, "Height": 600, "FileSize": 13380, "Orientation": 0, "Format": "PNG" }'



