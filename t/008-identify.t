# vim:set ft= ts=4 sw=4 et fdm=marker:

use FindBin;
use Test::Nginx::Socket;

use lib "$FindBin::Bin/lib";

log_level('debug'); # to ensure any log-level can be outputed

repeat_each(1);

plan tests => repeat_each() * (blocks() * 4);

run_tests();


__DATA__

=== TEST 1: identify jpg
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/1024_768.jpg
--- response_headers
Content-Type: text/plain
Content-Length: 62
--- response_body_like eval: '{ "img" : { "width": 1024, "height": 768, "type": "JPEG" } }'



=== TEST 2: identify gif
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/paged.gif
--- response_headers
Content-Type: text/plain
Content-Length: 58
--- response_body_like eval: '{ "img" : { "width": 46, "height": 46, "type": "GIF" } }'



=== TEST 3: identify png
--- config eval
<<EOF;
    location /identify {
         alias $FindBin::Bin/data;
         gm identify;
    }
EOF
--- request
GET /identify/wm.png
--- response_headers
Content-Type: text/plain
Content-Length: 60
--- response_body_like eval: '{ "img" : { "width": 800, "height": 600, "type": "PNG" } }'
