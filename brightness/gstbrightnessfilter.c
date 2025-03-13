#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#define GST_TYPE_BRIGHTNESS_FILTER (gst_brightness_filter_get_type())
#define GST_BRIGHTNESS_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BRIGHTNESS_FILTER, GstBrightnessFilter))
#define GST_BRIGHTNESS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BRIGHTNESS_FILTER, GstBrightnessFilterClass))
#define GST_IS_BRIGHTNESS_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BRIGHTNESS_FILTER))
#define GST_IS_BRIGHTNESS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BRIGHTNESS_FILTER))

typedef struct _GstBrightnessFilter GstBrightnessFilter;
typedef struct _GstBrightnessFilterClass GstBrightnessFilterClass;

struct _GstBrightnessFilter
{
  GstVideoFilter videofilter;
  
  /* Độ sáng, phạm vi từ -1.0 đến 1.0 */
  gdouble brightness;
};

struct _GstBrightnessFilterClass
{
  GstVideoFilterClass parent_class;
};

GType gst_brightness_filter_get_type(void);

enum
{
  PROP_0,
  PROP_BRIGHTNESS
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ RGB, BGR, RGBA, BGRA, YUV }")));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ RGB, BGR, RGBA, BGRA, YUV }")));

G_DEFINE_TYPE(GstBrightnessFilter, gst_brightness_filter, GST_TYPE_VIDEO_FILTER);

static void gst_brightness_filter_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);
static void gst_brightness_filter_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);

static GstFlowReturn gst_brightness_filter_transform_frame_ip(GstVideoFilter *filter,
    GstVideoFrame *frame);

static void
gst_brightness_filter_class_init(GstBrightnessFilterClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;
  GstElementClass *element_class = (GstElementClass *)klass;
  GstVideoFilterClass *videofilter_class = (GstVideoFilterClass *)klass;

  gobject_class->set_property = gst_brightness_filter_set_property;
  gobject_class->get_property = gst_brightness_filter_get_property;

  g_object_class_install_property(gobject_class, PROP_BRIGHTNESS,
      g_param_spec_double("brightness", "Brightness",
          "Độ sáng của video, phạm vi từ -1.0 (tối) đến 1.0 (sáng)",
          -1.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata(element_class,
      "Brightness adjustment filter",
      "Filter/Effect/Video",
      "Điều chỉnh độ sáng của video",
      "Your Name <your.email@example.com>");

  gst_element_class_add_pad_template(element_class,
      gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(element_class,
      gst_static_pad_template_get(&sink_factory));

  videofilter_class->transform_frame_ip = gst_brightness_filter_transform_frame_ip;
}

static void
gst_brightness_filter_init(GstBrightnessFilter *filter)
{
  filter->brightness = 0.0;
}

static void
gst_brightness_filter_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
  GstBrightnessFilter *filter = GST_BRIGHTNESS_FILTER(object);

  switch (prop_id) {
    case PROP_BRIGHTNESS:
      filter->brightness = g_value_get_double(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_brightness_filter_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
  GstBrightnessFilter *filter = GST_BRIGHTNESS_FILTER(object);

  switch (prop_id) {
    case PROP_BRIGHTNESS:
      g_value_set_double(value, filter->brightness);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

/* Áp dụng hiệu ứng độ sáng cho video frame */
static GstFlowReturn
gst_brightness_filter_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame)
{
  GstBrightnessFilter *brightness_filter = GST_BRIGHTNESS_FILTER(filter);
  gint x, y;
  guint8 *data;
  gint stride, width, height;
  gint pixel_stride;
  gint brightness_offset;
  
  /* Không làm gì nếu độ sáng là 0 */
  if (brightness_filter->brightness == 0.0)
    return GST_FLOW_OK;
  
  /* Chuyển đổi giá trị độ sáng từ -1.0...1.0 thành -255...255 */
  brightness_offset = (gint)(brightness_filter->brightness * 255);
  
  /* Lấy thông tin từ frame */
  data = GST_VIDEO_FRAME_PLANE_DATA(frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);
  width = GST_VIDEO_FRAME_WIDTH(frame);
  height = GST_VIDEO_FRAME_HEIGHT(frame);
  
  /* Xác định số bytes cho mỗi pixel */
  switch (GST_VIDEO_FRAME_FORMAT(frame)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
      pixel_stride = 3;
      break;
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
      pixel_stride = 4;
      break;
    default:
      /* Chỉ hỗ trợ RGB/BGR và RGBA/BGRA */
      return GST_FLOW_ERROR;
  }
  
  /* Điều chỉnh độ sáng cho từng pixel */
  for (y = 0; y < height; y++) {
    guint8 *line = data + y * stride;
    for (x = 0; x < width; x++) {
      gint i;
      for (i = 0; i < pixel_stride; i++) {
        /* Bỏ qua kênh alpha */
        if (pixel_stride == 4 && i == 3)
          continue;
        
        /* Áp dụng điều chỉnh độ sáng */
        gint new_value = line[x * pixel_stride + i] + brightness_offset;
        
        /* Đảm bảo giá trị nằm trong phạm vi 0-255 */
        if (new_value > 255)
          new_value = 255;
        else if (new_value < 0)
          new_value = 0;
        
        line[x * pixel_stride + i] = new_value;
      }
    }
  }
  
  return GST_FLOW_OK;
}

/* Plugin initialization */
static gboolean
plugin_init(GstPlugin *plugin)
{
  return gst_element_register(plugin, "brightnessfilter",
      GST_RANK_NONE, GST_TYPE_BRIGHTNESS_FILTER);
}

#define PACKAGE "mybrightnessfilter"
#define VERSION "1.0"

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    brightnessfilter,
    "Brightness adjustment filter",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "https://github.com/lampt224321/Gstreamer"
)
