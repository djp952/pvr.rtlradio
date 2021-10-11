#!/bin/sh

path=$1
rm -rf /usr/bin/rtl_tcp 2> /dev/null
rm -rf /var/www/rtl_tcp > /dev/null
rm -rf $path
