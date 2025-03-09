$ make
$ export GST_PLUGIN_PATH=$PWD:$GST_PLUGIN_PATH
$ gst-launch-1.0 v4l2src ! videoconvert ! brightnessfilter brightness=0.2 ! videoconvert ! autovideosink

# Adjust brightness from 0 to 1
