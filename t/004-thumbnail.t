# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no thumbnail
--- config eval
<<EOF;
    location /thumbnail {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /thumbnail/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2:  200x100
--- config eval
<<EOF;
    location /thumbnail {
         alias $FindBin::Bin/data;
         gm thumbnail 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /thumbnail/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5152



=== TEST 3: thumbnail 100x100>
--- config eval
<<EOF;
    location /thumbnail {
         alias $FindBin::Bin/data;
         gm thumbnail 100x100>;
         gm_image_quality 85;
    }
EOF
--- request
GET /thumbnail/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3273



=== TEST 4: thumbnail with variable
--- config eval
<<EOF;
    location /thumbnail {
         set \$thumbnail "100x100>";
         alias $FindBin::Bin/data;
         gm thumbnail \$thumbnail;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /thumbnail/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3273



