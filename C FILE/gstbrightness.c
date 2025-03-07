#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#define GST_TYPE_BRIGHTNESS_FILTER (gst_brightness_filter_get_type())
G_DECLARE_FINAL_TYPE(GstBrightnessFilter, gst_brightness_filter, GST, BRIGHTNESS_FILTER, GstBaseTransform)

struct _GstBrightnessFilter {
  GstBaseTransform parent;
  
  gdouble brightness;
  GstVideoInfo info;
};

enum {
  PROP_0,
  PROP_BRIGHTNESS,
  PROP_LAST
};

#define DEFAULT_BRIGHTNESS 0.0 /* Mặc định không thay đổi độ sáng */

G_DEFINE_TYPE(GstBrightnessFilter, gst_brightness_filter, GST_TYPE_BASE_TRANSFORM)

static void
gst_brightness_filter_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
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
gst_brightness_filter_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
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

static gboolean
gst_brightness_filter_set_info(GstBaseTransform * btrans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstBrightnessFilter *filter = GST_BRIGHTNESS_FILTER(btrans);
  
  if (!gst_video_info_from_caps(&filter->info, incaps))
    return FALSE;
    
  return TRUE;
}

static GstFlowReturn
gst_brightness_filter_transform_frame(GstBaseTransform * btrans,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstBrightnessFilter *filter = GST_BRIGHTNESS_FILTER(btrans);
  GstVideoFrame in_frame, out_frame;
  guint y, x;
  gint pixel_stride, row_stride;
  guint8 *in_data, *out_data;
  gint width, height;
  
  if (!gst_video_frame_map(&in_frame, &filter->info, inbuf, GST_MAP_READ))
    goto map_failed;
    
  if (!gst_video_frame_map(&out_frame, &filter->info, outbuf, GST_MAP_WRITE)) {
    gst_video_frame_unmap(&in_frame);
    goto map_failed;
  }
  
  width = GST_VIDEO_FRAME_WIDTH(&in_frame);
  height = GST_VIDEO_FRAME_HEIGHT(&in_frame);

  /* Chỉ xử lý kênh Y cho định dạng YUV */
  in_data = GST_VIDEO_FRAME_PLANE_DATA(&in_frame, 0);
  out_data = GST_VIDEO_FRAME_PLANE_DATA(&out_frame, 0);
  
  row_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&in_frame, 0);
  
  /* Điều chỉnh độ sáng cho từng pixel */
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      /* Đối với Y của YUV, giới hạn giá trị từ 16-235 */
      gint new_val = in_data[x] + (filter->brightness * 255.0);
      
      /* Đảm bảo giá trị nằm trong khoảng hợp lệ */
      if (new_val < 16)
        new_val = 16;
      else if (new_val > 235)
        new_val = 235;
        
      out_data[x] = new_val;
    }
    
    /* Di chuyển đến dòng tiếp theo */
    in_data += row_stride;
    out_data += row_stride;
  }
  
  /* Sao chép các kênh màu U và V (không thay đổi) */
  for (guint p = 1; p < GST_VIDEO_FRAME_N_PLANES(&in_frame); p++) {
    in_data = GST_VIDEO_FRAME_PLANE_DATA(&in_frame, p);
    out_data = GST_VIDEO_FRAME_PLANE_DATA(&out_frame, p);
    row_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&in_frame, p);
    pixel_stride = 1;
    width = GST_VIDEO_FRAME_COMP_WIDTH(&in_frame, p);
    height = GST_VIDEO_FRAME_COMP_HEIGHT(&in_frame, p);
    
    for (y = 0; y < height; y++) {
      memcpy(out_data, in_data, width * pixel_stride);
      in_data += row_stride;
      out_data += row_stride;
    }
  }
  
  gst_video_frame_unmap(&out_frame);
  gst_video_frame_unmap(&in_frame);
  
  return GST_FLOW_OK;
  
map_failed:
  GST_ELEMENT_ERROR(filter, CORE, FAILED, ("Failed to map video frame"), (NULL));
  return GST_FLOW_ERROR;
}

static void
gst_brightness_filter_class_init(GstBrightnessFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  
  gobject_class->set_property = gst_brightness_filter_set_property;
  gobject_class->get_property = gst_brightness_filter_get_property;
  
  g_object_class_install_property(gobject_class, PROP_BRIGHTNESS,
      g_param_spec_double("brightness", "Brightness", "Brightness adjustment factor",
          -1.0, 1.0, DEFAULT_BRIGHTNESS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  base_transform_class->transform_frame = gst_brightness_filter_transform_frame;
  base_transform_class->set_caps = gst_brightness_filter_set_info;
  
  gst_element_class_set_static_metadata(element_class,
      "Brightness filter", "Filter/Effect/Video",
      "Adjusts the brightness of video frames",
      "Your Name <your.email@example.com>");
  
  gst_element_class_add_pad_template(element_class,
      gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string("video/x-raw, format={ I420, NV12, NV21, YUY2, UYVY }")));
  
  gst_element_class_add_pad_template(element_class,
      gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string("video/x-raw, format={ I420, NV12, NV21, YUY2, UYVY }")));
}

static void
gst_brightness_filter_init(GstBrightnessFilter * filter)
{
  filter->brightness = DEFAULT_BRIGHTNESS;
}

static gboolean
plugin_init(GstPlugin * plugin)
{
  return gst_element_register(plugin, "brightness", GST_RANK_NONE,
      GST_TYPE_BRIGHTNESS_FILTER);
}

#define VERSION "1.0"
#define PACKAGE "brightness_plugin_package"

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    brightness,
    "Brightness adjustment plugin for GStreamer",
    plugin_init,
    VERSION,
    "LGPL",
    PACKAGE,
    "https://gstreamer.freedesktop.org/")
