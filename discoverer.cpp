#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "discoverer.h"

gboolean Discoverer::init()
{
    GError *err = NULL;

    gst_init(NULL,NULL);

    discoverer = gst_discoverer_new(5*GST_SECOND, &err);
    if(!discoverer)
    {
        g_printerr("gst_discoverer_new failed: %s\n", err->message);
        g_clear_error(&err);
        return false;
    }

    return true;
}

void Discoverer::deinit()
{
    g_object_unref(discoverer);
}

gboolean Discoverer::is_video(gchar* path)
{
    GError*             err = NULL;
    GstDiscovererInfo* info = NULL;

    info = gst_discoverer_discover_uri(discoverer, path, &err);
    if(!info)
    {
        g_printerr("gst_discoverer_discover_uri failed.\n");
        return false;
    }

    if(!gst_discoverer_info_get_video_streams(info))
    {
        gst_discoverer_info_unref(info);
        return false;
    }

    gst_discoverer_info_unref(info);
    return true;
}
