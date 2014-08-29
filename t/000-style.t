# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: style with resize
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm_style "gm resize 100x100>;";
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3319



=== TEST 2: style with resize and crop
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm_style "gm resize 450x300;gm crop 200x100+10+10;";
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5164



=== TEST 3: style variable with resize and crop
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         set \$style "gm resize 450x300;gm crop 200x100+10+10;";
         gm_style \$style;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5164



