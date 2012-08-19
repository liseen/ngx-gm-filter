# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no op
--- config eval
<<EOF;
    location /rotate {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /rotate/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2: rotate 90
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm convert -rotate 90;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 162704



=== TEST 3: rotate -270
--- config eval
<<EOF;
    location /rotate {
         alias $FindBin::Bin/data;
         gm convert -rotate 90;
         gm_image_quality 85;
    }
EOF
--- request
GET /rotate/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 162704



=== TEST 4: rotate and resize
--- config eval
<<EOF;
    location /resize {
         set \$resize "100x100>";
         set \$rotate "-270";
         alias $FindBin::Bin/data;
         gm convert -resize \$resize -rotate \$rotate;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3111


