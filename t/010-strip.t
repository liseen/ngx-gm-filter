# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: strip
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm strip;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 162428



=== TEST 2: auto-orient and strip
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm auto-orient;
         gm strip;
         gm_buffer 16m;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/outer_3.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 2446457



=== TEST 3: auto-orient and strip and resize 800x600
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm auto-orient;
         gm strip;
         gm resize 800x600;
         gm_image_quality 85;
         gm_buffer 16m;
    }
EOF
--- request
GET /resize/outer_3.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 200406


