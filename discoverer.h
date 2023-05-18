#ifndef DISCOVERER_H
#define DISCOVERER_H

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

class Discoverer
{
private:
    GstDiscoverer* discoverer;

public:
    gboolean init();
    void     deinit();

    gboolean is_video(gchar* path);
};

#endif // DISCOVERER_H
