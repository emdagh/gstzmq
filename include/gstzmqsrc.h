/* GStreamer
 * Copyright (C) 2023 FIXME <miel.van.dam@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_ZMQSRC_H_
#define _GST_ZMQSRC_H_

#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define ZMQ_DEFAULT_BIND_SRC FALSE
#define ZMQ_DEFAULT_BIND_SINK TRUE

#define ZMQ_DEFAULT_ENDPOINT_SERVER "tcp://*:5556"
#define ZMQ_DEFAULT_ENDPOINT_CLIENT "tcp://localhost:5556"

#define GST_TYPE_ZMQSRC (gst_zmqsrc_get_type())
#define GST_ZMQSRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ZMQSRC, GstZmqsrc))
#define GST_ZMQSRC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ZMQSRC, GstZmqsrcClass))
#define GST_IS_ZMQSRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_ZMQSRC))
#define GST_IS_ZMQSRC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_ZMQSRC))

typedef struct _GstZmqsrc GstZmqsrc;
typedef struct _GstZmqsrcClass GstZmqsrcClass;

struct _GstZmqsrc
{
    GstPushSrc base_zmqsrc;
    gchar* endpoint;
    gchar* topic;
    gboolean bind;
    void* context;
    void* socket;
};

struct _GstZmqsrcClass
{
    GstPushSrcClass base_zmqsrc_class;
};

GType gst_zmqsrc_get_type(void);

G_END_DECLS

#endif
