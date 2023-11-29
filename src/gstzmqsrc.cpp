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
 * SECTION:element-gstzmqsrc
 *
 * The zmqsrc element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! zmqsrc ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstzmqsrc.h"
#include <chrono>
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <zmq.h>

using namespace std::literals;

#define ZMQ_CHECKED(x)                                                                                                 \
    {                                                                                                                  \
        auto rc = x;                                                                                                   \
        if (rc)                                                                                                        \
        {                                                                                                              \
            GST_ERROR_OBJECT(src, #x " failed %d [%s]", errno, zmq_strerror(errno));                                   \
            return FALSE;                                                                                              \
        }                                                                                                              \
    }

GST_DEBUG_CATEGORY_STATIC(gst_zmqsrc_debug_category);
#define GST_CAT_DEFAULT gst_zmqsrc_debug_category

/* prototypes */

static void gst_zmqsrc_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void gst_zmqsrc_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);
static void gst_zmqsrc_dispose(GObject* object);
static void gst_zmqsrc_finalize(GObject* object);

static GstCaps* gst_zmqsrc_get_caps(GstBaseSrc* src, GstCaps* filter);
static gboolean gst_zmqsrc_negotiate(GstBaseSrc* src);
static GstCaps* gst_zmqsrc_fixate(GstBaseSrc* src, GstCaps* caps);
static gboolean gst_zmqsrc_set_caps(GstBaseSrc* src, GstCaps* caps);
static gboolean gst_zmqsrc_decide_allocation(GstBaseSrc* src, GstQuery* query);
static gboolean gst_zmqsrc_start(GstBaseSrc* src);
static gboolean gst_zmqsrc_stop(GstBaseSrc* src);
static void gst_zmqsrc_get_times(GstBaseSrc* src, GstBuffer* buffer, GstClockTime* start, GstClockTime* end);
static gboolean gst_zmqsrc_get_size(GstBaseSrc* src, guint64* size);
static gboolean gst_zmqsrc_is_seekable(GstBaseSrc* src);
static gboolean gst_zmqsrc_prepare_seek_segment(GstBaseSrc* src, GstEvent* seek, GstSegment* segment);
static gboolean gst_zmqsrc_do_seek(GstBaseSrc* src, GstSegment* segment);
static gboolean gst_zmqsrc_unlock(GstBaseSrc* src);
static gboolean gst_zmqsrc_unlock_stop(GstBaseSrc* src);
static gboolean gst_zmqsrc_query(GstBaseSrc* src, GstQuery* query);
static gboolean gst_zmqsrc_event(GstBaseSrc* src, GstEvent* event);
static GstFlowReturn gst_zmqsrc_create(GstBaseSrc* src, guint64 offset, guint size, GstBuffer** buf);
static GstFlowReturn gst_zmqsrc_alloc(GstBaseSrc* src, guint64 offset, guint size, GstBuffer** buf);
static GstFlowReturn gst_zmqsrc_fill(GstBaseSrc* src, guint64 offset, guint size, GstBuffer* buf);
static GstStateChangeReturn gst_zmqsrc_change_state(GstElement* element, GstStateChange transition);

enum
{
    PROP_0,
    PROP_ENDPOINT,
    PROP_BIND,
    PROP_IS_LIVE,
    PROP_TOPIC
};

/* pad templates */

static GstStaticPadTemplate gst_zmqsrc_src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    GstZmqsrc,
    gst_zmqsrc,
    GST_TYPE_BASE_SRC,
    GST_DEBUG_CATEGORY_INIT(gst_zmqsrc_debug_category, "zmqsrc", 0, "debug category for zmqsrc element"));

static void gst_zmqsrc_class_init(GstZmqsrcClass* klass)
{
    GObjectClass* gobject_class     = G_OBJECT_CLASS(klass);
    GstBaseSrcClass* base_src_class = GST_BASE_SRC_CLASS(klass);
    GstElementClass* element_class  = GST_ELEMENT_CLASS(klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &gst_zmqsrc_src_template);

    gst_element_class_set_static_metadata(
        GST_ELEMENT_CLASS(klass), "FIXME Long name", "Generic", "FIXME Description", "FIXME <emiel@FIXME.no>");

    element_class->change_state = GST_DEBUG_FUNCPTR(gst_zmqsrc_change_state);
    gobject_class->set_property = gst_zmqsrc_set_property;
    gobject_class->get_property = gst_zmqsrc_get_property;
    gobject_class->dispose      = gst_zmqsrc_dispose;
    gobject_class->finalize     = gst_zmqsrc_finalize;
    base_src_class->start       = GST_DEBUG_FUNCPTR(gst_zmqsrc_start);
    base_src_class->stop        = GST_DEBUG_FUNCPTR(gst_zmqsrc_stop);
    base_src_class->create      = GST_DEBUG_FUNCPTR(gst_zmqsrc_create);

    g_object_class_install_property(
        gobject_class,
        PROP_ENDPOINT,
        g_param_spec_string("endpoint",
                            "Endpoint",
                            "ZeroMQ endpoint from which to receive buffers",
                            ZMQ_DEFAULT_ENDPOINT_CLIENT,
                            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(
        gobject_class,
        PROP_TOPIC,
        g_param_spec_string("topic",
                            "Topic",
                            "ZeroMQ topic on which to receive buffers",
                            "",
                            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(
        gobject_class,
        PROP_BIND,
        g_param_spec_boolean("bind",
                             "Bind",
                             "If true, bind to the endpoint (be the \"server\")",
                             ZMQ_DEFAULT_BIND_SRC,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(
        gobject_class,
        PROP_IS_LIVE,
        g_param_spec_boolean("is-live",
                             "Is this a live source",
                             "True if the element cannot produce data in PAUSED",
                             TRUE,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void gst_zmqsrc_init(GstZmqsrc* self)
{
    self->endpoint = g_strdup(ZMQ_DEFAULT_ENDPOINT_CLIENT);
    self->bind     = ZMQ_DEFAULT_BIND_SRC;
    self->context  = zmq_ctx_new();
    self->topic    = g_strdup("");
}

void gst_zmqsrc_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(object);

    GST_DEBUG_OBJECT(zmqsrc, "set_property");

    switch (property_id)
    {
    case PROP_ENDPOINT:
        if (!g_value_get_string(value))
        {
            g_warning("endpoint property cannot be NULL");
            break;
        }
        g_free(zmqsrc->endpoint);
        zmqsrc->endpoint = g_strdup(g_value_get_string(value));
        break;
    case PROP_TOPIC:
        if (!g_value_get_string(value))
        {
            g_warning("topic property cannot be NULL");
            break;
        }
        g_free(zmqsrc->topic);
        zmqsrc->topic = g_strdup(g_value_get_string(value));
        break;
    case PROP_BIND:
        zmqsrc->bind = g_value_get_boolean(value);
        break;
    case PROP_IS_LIVE:
        gst_base_src_set_live(GST_BASE_SRC(object), g_value_get_boolean(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_zmqsrc_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(object);

    GST_DEBUG_OBJECT(zmqsrc, "get_property");

    switch (property_id)
    {
    case PROP_ENDPOINT:
        g_value_set_string(value, zmqsrc->endpoint);
        break;
    case PROP_TOPIC:
        g_value_set_string(value, zmqsrc->topic);
        break;
    case PROP_BIND:
        g_value_set_boolean(value, zmqsrc->bind);
        break;
    case PROP_IS_LIVE:
        g_value_set_boolean(value, gst_base_src_is_live(GST_BASE_SRC(object)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_zmqsrc_dispose(GObject* object)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(object);

    GST_DEBUG_OBJECT(zmqsrc, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_zmqsrc_parent_class)->dispose(object);
}

void gst_zmqsrc_finalize(GObject* object)
{
    GstZmqsrc* self = GST_ZMQSRC(object);

    GST_DEBUG_OBJECT(self, "finalize");

    /* clean up object here */
    zmq_ctx_destroy(self->context);
    // G_OBJECT_CLASS (parent_class)->finalize (gobject);

    G_OBJECT_CLASS(gst_zmqsrc_parent_class)->finalize(object);
}

/* get caps from subclass */
static GstCaps* gst_zmqsrc_get_caps(GstBaseSrc* src, GstCaps* filter)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(src);

    GST_DEBUG_OBJECT(zmqsrc, "get_caps");

    return NULL;
}
/* start and stop processing, ideal for opening/closing the resource */
static gboolean gst_zmqsrc_start(GstBaseSrc* src)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(src);

    GST_DEBUG_OBJECT(zmqsrc, "start");

    return TRUE;
}

static gboolean gst_zmqsrc_stop(GstBaseSrc* src)
{
    GstZmqsrc* zmqsrc = GST_ZMQSRC(src);

    GST_DEBUG_OBJECT(zmqsrc, "stop");

    return TRUE;
}

template <typename T>
int s_setsockopt(void* socket, int opt, const T& t)
{
    // ZMQ_CHECKED(zmq_setsockopt(socket, opt, &t, sizeof(T)));
    return TRUE;
}
#include <ostream>

void s_recv_stream(void* s, std::ostream& os)
{
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    int size     = zmq_msg_recv(&msg, s, 0);
    int numparts = 0;
    os.write((const char*)&numparts, sizeof(numparts));

    zmq_msg_close(&msg);
}

template <typename OutputIt>
void s_recv_impl(void* socket, OutputIt o_first)
{
    static const int BUFFER_LEN = 255;
    char buffer[BUFFER_LEN]     = {0};
    zmq_msg_t msg;
    zmq_msg_init(&msg);

    int size = zmq_msg_recv(&msg, socket, 0);
    if (size < 0)
    {
        throw std::runtime_error(zmq_strerror(errno));
    }
    if (size > BUFFER_LEN)
        size = BUFFER_LEN;

    *o_first++ = std::move(msg);
    // char* data= (char*)zmq_msg_data(&msg);
    // size_t len= zmq_msg_size(&msg);
    // std::copy(data, data + len, o_first);
    // zmq_msg_close(&msg);
};

template <typename OutputIt>
void s_recv(void* socket, OutputIt o_first)
{
    int more;
    size_t prop_len = sizeof(more);
    do
    {
        s_recv_impl(socket, o_first);
        zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &prop_len);
    } while (more);
};
/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn gst_zmqsrc_create(GstBaseSrc* psrc, guint64 offset, guint size, GstBuffer** outbuf)
{
    GstZmqsrc* src;
    GstFlowReturn retval = GST_FLOW_OK;
    GstMapInfo map;

    src = GST_ZMQSRC(psrc);
    std::vector<zmq_msg_t> parts;
    int err = 0;
    try
    {
        std::stringstream ss;
        s_recv(src->socket, std::back_inserter(parts));
        auto size = parts.size();
        ss.write((const char*)&size, sizeof(size));
        for (auto& part : parts)
        {
            auto size        = zmq_msg_size(&part);
            const char* data = static_cast<const char*>(zmq_msg_data(&part));
            ss.write((const char*)&size, sizeof(size));
            ss.write(data, size);
            zmq_msg_close(&part); // ownership has been moved to the `parts` list
        }
        std::string sdata = ss.str();
        size_t msg_size   = sdata.size();
        *outbuf           = gst_buffer_new_and_alloc(msg_size);

        gst_buffer_map(*outbuf, &map, GST_MAP_WRITE);
        memcpy(map.data, &sdata[0], msg_size);
        gst_buffer_unmap(*outbuf, &map);

        GST_LOG_OBJECT(src, "delivered a buffer of size %" G_GSIZE_FORMAT " bytes", sdata.size());
    }
    catch (std::runtime_error& err)
    {
        if (ENOTSOCK == errno)
        {
            GST_WARNING_OBJECT(src, "Connection closed");
            return GST_FLOW_EOS;
        }
        else
        {
            GST_ERROR_OBJECT(src, "receive failed with error code %d [%s]", errno, zmq_strerror(errno));
            return GST_FLOW_ERROR;
        }
    }
    return GST_FLOW_OK;
}

static gboolean gst_zmqsrc_open(GstZmqsrc* src)
{

    gboolean retval = TRUE;

    int rc;

    src->socket = zmq_socket(src->context, ZMQ_SUB);
    if (!src->socket)
    {
        GST_ERROR_OBJECT(src, "zmq_socket() failed with error code %d [%s]", errno, zmq_strerror(errno));
        retval = FALSE;
    }

    if (retval)
    {
        if (src->bind)
        {
            GST_DEBUG("binding to endpoint %s", src->endpoint);
            rc = zmq_bind(src->socket, src->endpoint);
            if (rc)
            {
                GST_ERROR_OBJECT(src,
                                 "zmq_bind() to endpoint \"%s\" failed with error code %d [%s]",
                                 src->endpoint,
                                 errno,
                                 zmq_strerror(errno));
                retval = FALSE;
            }
        }
        else
        {
            GST_DEBUG("connecting to endpoint %s", src->endpoint);
            rc = zmq_connect(src->socket, src->endpoint);
            if (rc)
            {
                GST_ERROR_OBJECT(src,
                                 "zmq_connect() to endpoint \"%s\" failed with error code %d [%s]",
                                 src->endpoint,
                                 errno,
                                 zmq_strerror(errno));
                retval = FALSE;
            }
        }
    }

    if (retval)
    {
        int watermark         = 1000;
        int rcvbuf            = 65535;
        int tcpkeepalive      = 1;
        int tcpkeepalive_idle = 60;

        GST_DEBUG("setting socket options...");

        // s_setsockopt(src->socket,)
        zmq_setsockopt(src->socket, ZMQ_SUBSCRIBE, src->topic, strlen(src->topic));
        s_setsockopt(src->socket,
                     ZMQ_RCVHWM,
                     watermark); //, sizeof(watermark))); // Set high water mark for inbound messages
        s_setsockopt(src->socket, ZMQ_RCVBUF, rcvbuf); //, sizeof(rcvbuf))); // Set kernel receive buffer size [bytes]
        s_setsockopt(src->socket, ZMQ_TCP_KEEPALIVE, tcpkeepalive); //, sizeof(tcpkeepalive))); // Enable TCP keep-alive
        s_setsockopt(
            src->socket,
            ZMQ_TCP_KEEPALIVE_IDLE,
            tcpkeepalive_idle); //, sizeof(tcpkeepalive_idle))); // Timeout until first keep-alive packet is sent [s]
        GST_DEBUG("done");
    }

    /*if (retval) {
      int timeout_ms = 1000;
      rc = zmq_setsockopt (src->socket, ZMQ_RCVTIMEO, &timeout_ms, sizeof (timeout_ms));
      if (rc) {
        GST_ERROR_OBJECT(src, "zmq_setsockopt() failed with error code %d [%s]", errno, zmq_strerror (errno));
        retval = FALSE;
      }
    }*/
    return retval;
}

static gboolean gst_zmqsrc_close(GstZmqsrc* src)
{

    gboolean retval = TRUE;

    int rc = zmq_close(src->socket);

    if (rc)
    {
        GST_WARNING_OBJECT(src, "zmq_close() failed with error code %d [%s]", errno, strerror(errno));
        return FALSE;
    }

    return TRUE;
}

static GstStateChangeReturn gst_zmqsrc_change_state(GstElement* element, GstStateChange transition)
{
    GstZmqsrc* src;
    GstStateChangeReturn result;

    src = GST_ZMQSRC(element);

    switch (transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!gst_zmqsrc_open(src))
        {
            GST_DEBUG_OBJECT(src, "failed to open socket");
            return GST_STATE_CHANGE_FAILURE;
        }
        break;
    default:
        break;
    }
    if ((result = GST_ELEMENT_CLASS(gst_zmqsrc_parent_class)->change_state(element, transition)) ==
        GST_STATE_CHANGE_FAILURE)
    {
        GST_DEBUG_OBJECT(src, "parent failed state change");
        return result;
    }

    switch (transition)
    {
    case GST_STATE_CHANGE_READY_TO_NULL:
        gst_zmqsrc_close(src);
        break;
    default:
        break;
    }
    return result;
}