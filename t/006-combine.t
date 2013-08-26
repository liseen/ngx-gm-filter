# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: resize 450x300, crop 200x100+10+10
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm resize 450x300;
         gm crop 200x100+10+10;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5164



=== TEST 2: crop 450x300+20+10, resize 200x100
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm crop 450x300+20+10;
         gm resize 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 4226

