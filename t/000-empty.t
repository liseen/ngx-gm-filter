# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no empty
--- config eval
<<EOF;
    location /empty {
         alias $FindBin::Bin/data;
         set \$quality 85;
         gm_image_quality \$quality;
    }
EOF
--- request
GET /empty/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 162428



=== TEST 2: empty with quality 85
--- config eval
<<EOF;
    location /empty {
         alias $FindBin::Bin/data;
         gm_image_quality 85;
    }
EOF
--- request
GET /empty/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 162428



