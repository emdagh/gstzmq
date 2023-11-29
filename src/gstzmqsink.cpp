/* GStreamer
 * Copyright (C) 2023 FIXME <emiel@FIXME.no>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstzmqsink
 *
 * The zmqsink element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! zmqsink ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstzmqsink.h"
#include <gst/base/gstbasesink.h>
#include <gst/gst.h>
#include <string>
#include <zmq.h>

GST_DEBUG_CATEGORY_STATIC(gst_zmqsink_debug_category);
#define GST_CAT_DEFAULT gst_zmqsink_debug_category

template <typename T> void s_send(void* socket, const char* topic, const T& t)
{
    zmq_send(socket, topic, strlen(topic), ZMQ_SNDMORE);
    zmq_send(socket, &t, sizeof(T), 0);
}

template <> void s_send<std::string>(void* s, const char* topic, const std::string& str)
{
    zmq_send(s, topic, strlen(topic), ZMQ_SNDMORE);
    zmq_send(s, str.data(), str.length(), 0);
}

/* prototypes */

static void gst_zmqsink_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void gst_zmqsink_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);
static void gst_zmqsink_finalize(GObject* object);

static gboolean gst_zmqsink_start(GstBaseSink* sink);
static gboolean gst_zmqsink_stop(GstBaseSink* sink);
static GstFlowReturn gst_zmqsink_render(GstBaseSink* sink, GstBuffer* buffer);

enum
{
    PROP_0,
    PROP_ENDPOINT,
    PROP_BIND,
    PROP_TOPIC
};

/* pad templates */

static GstStaticPadTemplate gst_zmqsink_sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/unknown"));

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    GstZmqsink,
    gst_zmqsink,
    GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT(gst_zmqsink_debug_category, "zmqsink", 0, "debug category for zmqsink element"));

static void gst_zmqsink_class_init(GstZmqsinkClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstBaseSinkClass* base_sink_class = GST_BASE_SINK_CLASS(klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &gst_zmqsink_sink_template);

    gst_element_class_set_static_metadata(
        GST_ELEMENT_CLASS(klass), "FIXME Long name", "Generic", "FIXME Description", "FIXME <emiel@FIXME.no>");

    gobject_class->set_property = gst_zmqsink_set_property;
    gobject_class->get_property = gst_zmqsink_get_property;
    gobject_class->finalize = gst_zmqsink_finalize;
    base_sink_class->start = GST_DEBUG_FUNCPTR(gst_zmqsink_start);
    base_sink_class->stop = GST_DEBUG_FUNCPTR(gst_zmqsink_stop);
    base_sink_class->render = GST_DEBUG_FUNCPTR(gst_zmqsink_render);
}

static void gst_zmqsink_init(GstZmqsink* self)
{
    self->endpoint = g_strdup("tcp://*:5558"); // ZMQ_DEFAULT_ENDPOINT_SERVER);
    self->bind = FALSE;                        // ZMQ_DEFAULT_BIND_SINK;
    self->context = zmq_ctx_new();
}

void gst_zmqsink_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstZmqsink* sink = GST_ZMQSINK(object);

    GST_DEBUG_OBJECT(sink, "set_property");

    switch (property_id)
    {
    case PROP_ENDPOINT:
        if (!g_value_get_string(value))
        {
            g_warning("endpoint property cannot be NULL");
            break;
        }
        if (sink->endpoint)
        {
            g_free(sink->endpoint);
        }
        sink->endpoint = g_strdup(g_value_get_string(value));
        break;
    case PROP_BIND:
        sink->bind = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_zmqsink_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstZmqsink* sink = GST_ZMQSINK(object);

    GST_DEBUG_OBJECT(sink, "get_property");

    switch (prop_id)
    {
    case PROP_ENDPOINT:
        g_value_set_string(value, sink->endpoint);
        break;
    case PROP_BIND:
        g_value_set_boolean(value, sink->bind);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void gst_zmqsink_finalize(GObject* object)
{
    GstZmqsink* self = GST_ZMQSINK(object);

    GST_DEBUG_OBJECT(self, "finalize");

    /* clean up object here */
    zmq_ctx_destroy(self->context);
    G_OBJECT_CLASS(gst_zmqsink_parent_class)->finalize(object);
}

/* get the start and end times for syncing on this buffer */
static void gst_zmqsink_get_times(GstBaseSink* sink, GstBuffer* buffer, GstClockTime* start, GstClockTime* end)
{
    GstZmqsink* zmqsink = GST_ZMQSINK(sink);

    GST_DEBUG_OBJECT(zmqsink, "get_times");
}

/* propose allocation parameters for upstream */
static gboolean gst_zmqsink_propose_allocation(GstBaseSink* sink, GstQuery* query)
{
    GstZmqsink* zmqsink = GST_ZMQSINK(sink);

    GST_DEBUG_OBJECT(zmqsink, "propose_allocation");

    return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean gst_zmqsink_start(GstBaseSink* basesink)
{
    gboolean retval = TRUE;

    GstZmqsink* sink;

    sink = GST_ZMQSINK(basesink);

    int rc;

    GST_DEBUG_OBJECT(sink, "starting");

    sink->socket = zmq_socket(sink->context, ZMQ_PUB);
    if (!sink->socket)
    {
        GST_ERROR_OBJECT(sink, "zmq_socket() failed with error code %d [%s]", errno, zmq_strerror(errno));
    }
    else
    {
        if (sink->bind)
        {
            GST_DEBUG_OBJECT(sink, "binding to endpoint %s", sink->endpoint);
            rc = zmq_bind(sink->socket, sink->endpoint);
            if (rc)
            {
                GST_ERROR_OBJECT(sink,
                                 "zmq_bind() to endpoint \"%s\" failed with error code %d [%s]",
                                 sink->endpoint,
                                 errno,
                                 zmq_strerror(errno));
                retval = FALSE;
            }
        }
        else
        {
            GST_DEBUG_OBJECT(sink, "connecting to endpoint %s", sink->endpoint);
            rc = zmq_connect(sink->socket, sink->endpoint);
            if (rc)
            {
                GST_ERROR_OBJECT(sink,
                                 "zmq_connect() to endpoint \"%s\" failed with error code %d [%s]",
                                 sink->endpoint,
                                 errno,
                                 zmq_strerror(errno));
                retval = FALSE;
            }
        }
    }

    return retval;
}

static gboolean gst_zmqsink_stop(GstBaseSink* basesink)
{
    gboolean retval = TRUE;

    GstZmqsink* sink;

    sink = GST_ZMQSINK(basesink);

    GST_DEBUG_OBJECT(sink, "stopping");

    int rc = zmq_close(sink->socket);

    if (rc)
    {
        GST_WARNING_OBJECT(sink, "zmq_close() failed with error code %d [%s]", errno, strerror(errno));
        retval = FALSE;
    }

    return retval;
}

static GstFlowReturn gst_zmqsink_render(GstBaseSink* basesink, GstBuffer* buffer)
{

    GstFlowReturn retval = GST_FLOW_OK;

    GstZmqsink* sink;
    GstMapInfo map;

    sink = GST_ZMQSINK(basesink);

    gst_buffer_map(buffer, &map, GST_MAP_READ);

    GST_DEBUG_OBJECT(sink, "publishing %" G_GSIZE_FORMAT " bytes", map.size);

    if (map.size > 0 && map.data != NULL)
    {
        zmq_msg_t msg;
        int rc = zmq_msg_init_size(&msg, map.size);
        if (rc)
        {
            GST_ERROR_OBJECT(sink, "zmq_msg_init_size() failed with error code %d [%s]", errno, zmq_strerror(errno));
            retval = GST_FLOW_ERROR;
        }
        else
        {
            memcpy(zmq_msg_data(&msg), map.data, map.size);

            rc = zmq_msg_send(&msg, sink->socket, 0);
            if (rc != map.size)
            {
                GST_ERROR_OBJECT(sink, "zmq_msg_send() failed with error code %d [%s]", errno, zmq_strerror(errno));
                zmq_msg_close(&msg);
                retval = GST_FLOW_ERROR;
            }
        }
    }

    gst_buffer_unmap(buffer, &map);

    return retval;
}
