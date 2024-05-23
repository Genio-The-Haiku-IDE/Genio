#!/bin/sh
script=$1
rc resource.rdef
settype -t application/x-vnd.Be-elfexecutable  $script
resattr -o  $script resource.rsrc