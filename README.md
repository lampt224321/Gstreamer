gcc -Wall -fPIC -shared -o libgstbrightness.so gstbrightness.c $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-base-1.0)

export GST_PLUGIN_PATH=$PWD:$GST_PLUGIN_PATH
