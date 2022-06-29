#!/bin/sh

path=$1

#create link
ln -s $path/bin/rtl_tcp /usr/bin/rtl_tcp
ln -s $path /var/www
