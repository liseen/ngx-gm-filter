# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3 - 2);

run_tests();


__DATA__

=== TEST 1: no crop
--- config eval
<<EOF;
    location /crop {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /crop/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2: crop 200x100
--- config eval
<<EOF;
    location /crop {
         alias $FindBin::Bin/data;
         gm crop 200x100;
         gm_image_quality 85;
    }
EOF
--- request
GET /crop/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3646



=== TEST 3: crop 200x100+50+30
--- config eval
<<EOF;
    location /crop {
         alias $FindBin::Bin/data;
         gm crop 200x100+50+30;
         gm_image_quality 85;
    }
EOF
--- request
GET /crop/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 3058



=== TEST 4: crop with variable
--- config eval
<<EOF;
    location /crop {
         set \$crop "100x100+20+30";
         alias $FindBin::Bin/data;
         gm crop \$crop;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /crop/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 2511



=== TEST 5: crop with variable, geometry error
--- config eval
<<EOF;
    location /crop {
         set \$crop "100x100+a+30";
         alias $FindBin::Bin/data;
         gm crop \$crop;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /crop/1024_768.jpg
--- error_code: 415
--- error_log
format error



=== TEST 6: crop with geometry not contain image
--- config eval
<<EOF;
    location /crop {
         set \$crop "100x100+50+50";
         alias $FindBin::Bin/data;
         gm crop \$crop;
         gm_image_quality 85;
         gm_buffer 16M;
    }
EOF
--- request
GET /crop/paged.gif
--- error_code: 415
--- error_log
crop image failed
