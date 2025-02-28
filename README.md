gcc -shared -fPIC -o BriPluginCustom.so BriPluginCustom.c $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0)
