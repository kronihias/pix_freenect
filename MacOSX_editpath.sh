#!/bin/sh
#
# changes path for libfreenect in pix_freenect.pd_darwin and libfreenect.0.0.1.dylib

install_name_tool -change libfreenect.0.0.dylib @loader_path/libfreenect.0.0.1.dylib pix_freenect.pd_darwin 
install_name_tool -change /usr/local/lib/libusb-1.0.0.dylib @loader_path/libusb-1.0.0.dylib libfreenect.0.0.1.dylib


