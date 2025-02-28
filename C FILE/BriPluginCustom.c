#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#define GST_TYPE_BRIGHTNESS_FILTER (gst_brightness_filter_get_type())
G_DECLARE_FINAL_TYPE(GstBrightnessFilter, gst_brightness_filter, GST, BRIGHTNESS_FILTER, GstVideoFilter)

struct _GstBrightnessFilter {
    GstVideoFilter parent;
    gdouble brightness; 
};

G_DEFINE_TYPE(GstBrightnessFilter, gst_brightness_filter, GST_TYPE_VIDEO_FILTER);

static GstFlowReturn gst_brightness_filter_transform_frame(GstVideoFilter *filter, GstVideoFrame *inframe, GstVideoFrame *outframe) {
    GstBrightnessFilter *self = GST_BRIGHTNESS_FILTER(filter);
    guint8 *src = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(inframe, 0);
    guint8 *dest = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(outframe, 0);
    gint size = GST_VIDEO_FRAME_SIZE(inframe);

    for (gint i = 0; i < size; i++) {
        dest[i] = MIN(src[i] + (self->brightness * 255), 255);
    }
    return GST_FLOW_OK;
}

static void gst_brightness_filter_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GstBrightnessFilter *self = GST_BRIGHTNESS_FILTER(object);
    switch (prop_id) {
        case 1:
            self->brightness = g_value_get_double(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void gst_brightness_filter_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GstBrightnessFilter *self = GST_BRIGHTNESS_FILTER(object);
    switch (prop_id) {
        case 1:
            g_value_set_double(value, self->brightness);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void gst_brightness_filter_class_init(GstBrightnessFilterClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstVideoFilterClass *filter_class = GST_VIDEO_FILTER_CLASS(klass);

    gobject_class->set_property = gst_brightness_filter_set_property;
    gobject_class->get_property = gst_brightness_filter_get_property;

    g_object_class_install_property(gobject_class, 1,
        g_param_spec_double("brightness", "Brightness", "Adjust brightness level",
                            0.0, 1.0, 0.2, G_PARAM_READWRITE));

    filter_class->transform_frame = gst_brightness_filter_transform_frame;
}

static void gst_brightness_filter_init(GstBrightnessFilter *self) {
    self->brightness = 0.2; // Giá trị mặc định
}

static gboolean plugin_init(GstPlugin *plugin) {
    return gst_element_register(plugin, "brightness", GST_RANK_NONE, GST_TYPE_BRIGHTNESS_FILTER);
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    brightness,
    "Brightness adjustment filter",
    plugin_init,
    "1.0",
    "LGPL",
    "GStreamer",
    "https://gstreamer.freedesktop.org/",
    "Brightness Plugin")

