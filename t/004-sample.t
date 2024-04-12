# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no sample
--- config eval
<<EOF;
    location /sample {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /sample/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2:  200x100
--- config eval
<<EOF;
    location /sample {
         alias $FindBin::Bin/data;
         gm sample 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /sample/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5975



=== TEST 3: sample 100x100>
--- config eval
<<EOF;
    location /sample {
         alias $FindBin::Bin/data;
         gm sample 100x100>;
         gm_image_quality 85;
    }
EOF
--- request
GET /sample/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3870



=== TEST 4: sample with variable
--- config eval
<<EOF;
    location /sample {
         set \$sample "100x100>";
         alias $FindBin::Bin/data;
         gm sample \$sample;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /sample/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3870



