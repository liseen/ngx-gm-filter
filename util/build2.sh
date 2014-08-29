#!/bin/sh
pwd=`pwd`

cd /data/Source/nginx/
CFLAGS="-g -DDDEBUG=1 -DNGX_HAVE_VARIADIC_MACROS" ./configure --prefix=/opt/nginx \
    --add-module=/data/Source/ngx_devel_kit \
    --add-module=/data/Source/set-misc-nginx-module \
    --add-module=/data/Source/srcache-nginx-module \
    --add-module=/data/Source/lua-nginx-module \
    --add-module=/data/Source/nginx-tfs \
    --add-module=${pwd} \
    --add-module=/data/Source/nginx_upstream_hash \
    --with-pcre=/data/Source/pcre-8.33 \
    --with-debug 
#    --with-cc-opt=""

make
make install

cd $pwd
