# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no scale
--- config eval
<<EOF;
    location /scale {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /scale/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2:  200x100
--- config eval
<<EOF;
    location /scale {
         alias $FindBin::Bin/data;
         gm scale 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /scale/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5127



=== TEST 3: scale 100x100>
--- config eval
<<EOF;
    location /scale {
         alias $FindBin::Bin/data;
         gm scale 100x100>;
         gm_image_quality 85;
    }
EOF
--- request
GET /scale/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3258



=== TEST 4: scale with variable
--- config eval
<<EOF;
    location /scale {
         set \$scale "100x100>";
         alias $FindBin::Bin/data;
         gm scale \$scale;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /scale/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3258



