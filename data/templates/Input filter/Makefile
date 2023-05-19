input_filter.so: input_filter.cpp
	gcc -g -shared -o input_filter.so input_filter.cpp

install: scroll_ifilter.so
	cp input_filter.so /boot/home/config/non-packaged/add-ons/input_server/filters
	/system/servers/input_server -q

all: input_filter.so
