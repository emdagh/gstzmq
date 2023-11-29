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

#ifndef _GST_ZMQSINK_H_
#define _GST_ZMQSINK_H_

#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_ZMQSINK (gst_zmqsink_get_type())
#define GST_ZMQSINK(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ZMQSINK, GstZmqsink))
#define GST_ZMQSINK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ZMQSINK, GstZmqsinkClass))
#define GST_IS_ZMQSINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_ZMQSINK))
#define GST_IS_ZMQSINK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_ZMQSINK))

typedef struct _GstZmqsink GstZmqsink;
typedef struct _GstZmqsinkClass GstZmqsinkClass;

struct _GstZmqsink
{
    GstBaseSink base_zmqsink;
    gchar* endpoint;
    gchar* topic;
    gboolean bind;
    void* context;
    void* socket;
};

struct _GstZmqsinkClass
{
    GstBaseSinkClass base_zmqsink_class;
};

GType gst_zmqsink_get_type(void);

G_END_DECLS

#endif
