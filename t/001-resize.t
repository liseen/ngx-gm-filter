# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3);

run_tests();


__DATA__

=== TEST 1: no resize
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2: resize 200x200 and no crop
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm convert -resize 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 5221



=== TEST 3: no resize
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;
         gm convert -resize 100x100>;
         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3319



=== TEST 4: no resize with variable
--- config eval
<<EOF;
    location /resize {
         set \$resize "100x100>";
         alias $FindBin::Bin/data;
         gm convert -resize \$resize;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3319


