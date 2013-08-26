#!/bin/bash
script_dir=$(dirname $0)
root=$(readlink -f $script_dir/..)
testfile=${1:-$root/t/*.t}
cd $root
$script_dir/reindex $testfile
export PATH=/opt/nginx/sbin:$PATH
#export TEST_NGINX_USE_VALGRIND=1
#export TEST_NGINX_SLEEP=5
killall nginx

echo "start test.........."
prove -v $testfile

