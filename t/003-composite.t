# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";


repeat_each(1);

plan tests => repeat_each() * (blocks() * 3-1);

run_tests();


__DATA__

=== TEST 1: no op
--- config eval
<<EOF;
    location /gm {
         alias $FindBin::Bin/data;
    }
EOF
--- request
GET /gm/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 597491



=== TEST 2: composite image
--- config eval
<<EOF;
    location /gm {
         alias $FindBin::Bin/data;

         gm composite -gravity SouthEast -min-width 200 -min-height 200 $FindBin::Bin/data/wm.png;

         gm_image_quality 85;
    }
EOF
--- request
GET /gm/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 164302



=== TEST 3: composite image
--- config eval
<<EOF;
    location /gm {
         alias $FindBin::Bin/data;

         gm composite -gravity SouthEast -min-width 200 -min-height 200 $FindBin::Bin/data/notfund.png;

         gm_image_quality 85;
    }
EOF
--- request
GET /gm/1024_768.jpg
--- response_headers
Content-Type: text/html
--- error_code chomp
415



=== TEST 4: no composite image on width less than min-width
--- config eval
<<EOF;
    location /gm {
         alias $FindBin::Bin/data;

         gm convert -resize 100x100!;
         gm composite -gravity SouthEast -min-width 200 -min-height 200 $FindBin::Bin/data/wm.png;

         gm_image_quality 85;
    }
EOF
--- request
GET /gm/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 4134



=== TEST 5: rotate and resize and composite
--- config eval
<<EOF;
    location /resize {
         alias $FindBin::Bin/data;

         set \$resize "400x400>";
         set \$rotate "-270";

         gm convert -resize \$resize -rotate \$rotate;
         gm composite -gravity SouthEast -min-width 200 -min-height 200 $FindBin::Bin/data/wm.png;

         gm_image_quality 85;
    }
EOF
--- request
GET /resize/1024_768.jpg
--- response_headers
Content-Type: image/jpeg
Content-Length: 31592

