#include "gstzmqsink.h"
#include "gstzmqsrc.h"
#include <gst/gst.h>

static gboolean plugin_init(GstPlugin* plugin)
{

    /* FIXME Remember to set the rank if it's an element that is meant
       to be autoplugged by decodebin. */

    if (!gst_element_register(plugin, "zmqsrc", GST_RANK_NONE, GST_TYPE_ZMQSRC))
      return FALSE;
    if (!gst_element_register(plugin, "zmqsink", GST_RANK_NONE,
                              GST_TYPE_ZMQSINK))
      return FALSE;

    return TRUE;
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.2"
#endif
#ifndef PACKAGE
#define PACKAGE "zmq"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME PACKAGE
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://oh.no/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, zmq,
                  "ZeroMQ source and sink", plugin_init, VERSION, "GPL",
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
