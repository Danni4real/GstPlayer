#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <string>

#include "tools.h"
#include "player.h"
#include "config.h"

Player::Player(gint32 player_type)
{
    this->player_type = player_type;
}

gboolean Player::init()
{
    GstBus  *bus = NULL;

    duration = -1;
    position = -1;
    seekable = false;
    state    = GST_STATE_NULL;

    pipeline       = NULL;
    uridecodebin   = NULL;
    audio_convert  = NULL;
    audio_resample = NULL;
    audio_volume   = NULL;
    audio_sink     = NULL;
    video_convert  = NULL;
    video_resize   = NULL;
    video_sink     = NULL;

    gst_init(NULL, NULL);

    pipeline       = gst_pipeline_new("pipeline");
    uridecodebin   = gst_element_factory_make("uridecodebin",        "uridecodebin");
    audio_convert  = gst_element_factory_make("audioconvert",        "audioconvert");
    audio_resample = gst_element_factory_make("audioresample",       "audioresample");
    audio_volume   = gst_element_factory_make("volume",              "audiovolume");
    audio_sink     = gst_element_factory_make("alsasink",            "audiosink");
  //video_convert  = gst_element_factory_make("imxvideoconvert_g2d", "videoconvert"); // for nxp imx8
    video_convert  = gst_element_factory_make("videoconvert",        "videoconvert"); // for x64 linux
    video_resize   = gst_element_factory_make("capsfilter",          "videoresize");
  //video_sink     = gst_element_factory_make("kmssink",             "videosink");    // for nxp imx8
    video_sink     = gst_element_factory_make("ximagesink",          "videosink");    // for x64 linux

    if(!pipeline || !uridecodebin || !audio_convert || !audio_resample || !audio_volume || !audio_sink ||
                                     !video_convert || !video_resize                    || !video_sink)
    {
        g_printerr("some elements could not be created.\n");
        goto err;
    }

    // set sink property
    g_object_set(G_OBJECT(audio_sink), "device",      "default", NULL);
    g_object_set(G_OBJECT(audio_sink), "buffer-time", 220000,    NULL);
    g_object_set(G_OBJECT(audio_sink), "blocksize",   4096 * 3,  NULL);
    g_object_set(G_OBJECT(video_sink), "plane-id",    PLANE_ID,  NULL);

    // set pipeline
    if(player_type == AUDIO_PLAYER)
    {
        gst_bin_add_many(GST_BIN(pipeline), uridecodebin, audio_convert, audio_resample, audio_volume, audio_sink, NULL);
    }
    if(player_type == VIDEO_PLAYER)
    {
        gst_bin_add_many(GST_BIN(pipeline), uridecodebin, audio_convert, audio_resample, audio_volume, audio_sink,
                                                          video_convert, video_resize  ,               video_sink, NULL);

        if(!gst_element_link_many(video_convert, video_resize, video_sink, NULL))
        {
            g_printerr("video elements could not be linked.\n");
            goto err;
        }
    }

    if(!gst_element_link_many(audio_convert, audio_resample, audio_volume, audio_sink, NULL))
    {
        g_printerr("audio elements could not be linked.\n");
        goto err;
    }

    // set call back
    g_signal_connect(uridecodebin, "pad-added", G_CALLBACK(handle_pad_add), this);

    bus = gst_element_get_bus(pipeline); if(!bus) {goto err;}

    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::eos",              (GCallback)handle_eos,             this);
    g_signal_connect(G_OBJECT(bus), "message::error",            (GCallback)handle_error,           this);
    g_signal_connect(G_OBJECT(bus), "message::state-changed",    (GCallback)handle_state_change,    this);
    g_signal_connect(G_OBJECT(bus), "message::duration-changed", (GCallback)query_duration,         this);
    gst_object_unref(bus);

    g_timeout_add_seconds(1, (GSourceFunc)query_position, this);

    return true;

err:
    if(pipeline       != NULL) {gst_object_unref(pipeline);}
    if(uridecodebin   != NULL) {gst_object_unref(uridecodebin);}
    if(audio_convert  != NULL) {gst_object_unref(audio_convert);}
    if(audio_resample != NULL) {gst_object_unref(audio_resample);}
    if(audio_volume   != NULL) {gst_object_unref(audio_volume);}
    if(audio_sink     != NULL) {gst_object_unref(audio_sink);}
    if(video_convert  != NULL) {gst_object_unref(video_convert);}
    if(video_resize   != NULL) {gst_object_unref(video_resize);}
    if(video_sink     != NULL) {gst_object_unref(video_sink);}
    if(bus            != NULL) {gst_object_unref(bus);}

    return false;
}

void Player::deinit()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void Player::bind_to_client(const gchar* client_socket_file_path)
{
    if(!socket.open_send_sock(client_socket_file_path))
    {
        g_printerr("bind_to_client failed!\n");
        return;
    }

    socket.send(gen_message(SERVER_MESSAGE_HEAD_PPLAYER,(long)this));
}

void Player::set_path(const gchar* path)
{
    duration = -1;
    position = -1;
    seekable = false;

    g_object_set(uridecodebin, "uri", path, NULL);
}

void Player::resize_video(guint width, guint height)
{
    GstCaps* resize_caps = gst_caps_new_simple("video/x-raw",
                                               "width",  G_TYPE_INT, width,
                                               "height", G_TYPE_INT, height,
                                               NULL);

    g_object_set(G_OBJECT(video_resize), "caps", resize_caps, NULL);
}

void Player::play()
{
    if(state == GST_STATE_PLAYING)
        return;

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void Player::pause()
{
    if(state != GST_STATE_PLAYING)
        return;

    gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

void Player::stop()
{
    if(state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        return;

    //gst_element_set_state(pipeline, GST_STATE_READY);
    gst_element_set_state(pipeline, GST_STATE_NULL);
}

void Player::mute(gboolean true_is_mute)
{
    if(state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        return;

    g_object_set(G_OBJECT(audio_volume), "mute", true_is_mute, NULL);
}

void Player::seek(guint seconds)
{
    if(state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        return;

    gst_element_seek_simple(pipeline,
                            GST_FORMAT_TIME,
                            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), // GST_SEEK_FLAG_ACCURATE instead of GST_SEEK_FLAG_KEY_UNIT
                            (gint64)(seconds * GST_SECOND));
}

void Player::set_speed(gdouble speed)
{
    gint64       stop_position;
    gint64       start_position;
    GstEvent     *seek_event = NULL;

    if(speed == 0 || position < 0 || duration < 0 || position > duration)
        return;

    if(speed > 0)
    {
        start_position = position * GST_SECOND;
        stop_position  = duration * GST_SECOND;
    }
    else
    {
        start_position = 0;
        stop_position  = position * GST_SECOND;
    }

    seek_event = gst_event_new_seek(speed, GST_FORMAT_TIME,
                                    (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                                    GST_SEEK_TYPE_SET, start_position,
                                    GST_SEEK_TYPE_SET, stop_position);

    if(seek_event == NULL)
    {
        g_printerr("gst_event_new_seek failed!\n");
        return;
    }

    if(!gst_element_send_event(pipeline, seek_event))
        g_printerr("gst_element_send_event failed!\n");
}

void Player::set_display(guint x_start, guint y_start, guint width, guint height)
{
    if(GST_IS_VIDEO_OVERLAY(video_sink))
    {
        gboolean ret = gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(video_sink),
                                                              x_start, y_start, width, height);
        if(ret == true)
            gst_video_overlay_expose(GST_VIDEO_OVERLAY(video_sink));
        else
            g_printerr("gst_video_overlay_set_render_rectangle failed!\n");
    }
    else
        g_printerr("GST_IS_VIDEO_OVERLAY failed!\n");
}

gboolean Player::query_position(Player* self)
{
    if(self->state != GST_STATE_PLAYING)
        return true;

    gint64 nanoseconds = -1;

    if(gst_element_query_position(self->pipeline, GST_FORMAT_TIME, &nanoseconds))
    {
        self->position = nanoseconds/GST_SECOND;
        g_print("%d\n", self->position);

        self->socket.send(gen_message(SERVER_MESSAGE_HEAD_POSITION,self->position));
    }
    else
        g_printerr("gst_element_query_position failed!\n");

    return true;
}

void Player::query_seekable()
{
    GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
    if(query == NULL)
    {
        g_printerr("gst_query_new_seeking failed!\n");
        return;
    }

    if(gst_element_query(pipeline, query))
    {
        gst_query_parse_seeking(query, NULL, &seekable, NULL, NULL);

        if(seekable)
            g_print("seekable.\n");
        else
            g_print("not seekable.\n");
    }
    else
        g_printerr("gst_element_query failed!\n");

    gst_query_unref(query);
}

void Player::handle_error(GstBus*, GstMessage *msg, Player* self)
{
    GError *err;
    gchar  *debug_info;

    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    gst_element_set_state(self->pipeline, GST_STATE_NULL);
    self->state = GST_STATE_NULL; // won't trigger handle_state_change() when err, so need set state here

    self->socket.send(gen_message(SERVER_MESSAGE_HEAD_ERROR,""/*TODO err content*/));
}

void Player::handle_eos(GstBus*, GstMessage *msg, Player* self)
{
    g_print("eos\n");

    gst_element_set_state(self->pipeline, GST_STATE_NULL);
    self->state = GST_STATE_NULL; // won't trigger handle_state_change() when eos, so need set state here

    self->socket.send(SERVER_MESSAGE_HEAD_EOS);
}

void Player::query_duration(GstBus*, GstMessage *msg, Player* self)
{
    gint64 nanoseconds = -1;

    if(gst_element_query_duration(self->pipeline, GST_FORMAT_TIME, &nanoseconds))
    {
        self->duration = nanoseconds/GST_SECOND;
        g_print("duration: %d\n", self->duration);

        self->socket.send(gen_message(SERVER_MESSAGE_HEAD_DURATION,self->duration));
    }
    else
        g_printerr("gst_element_query_duration failed!\n");
}

void Player::handle_state_change(GstBus*, GstMessage *msg, Player* self)
{
    GstState old_state;
    GstState new_state;
    GstState pending_state;

    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    if(GST_MESSAGE_SRC(msg) == GST_OBJECT(self->pipeline))
    {
        self->state = new_state;
        g_print("State set to %s\n", gst_element_state_get_name(new_state));
        if(new_state == GST_STATE_PLAYING)
        {
            query_duration(NULL,NULL,self);
            self->query_seekable();

            self->socket.send(gen_message(SERVER_MESSAGE_HEAD_STATE, SERVER_MESSAGE_NECK_PLAYING));
        }
        if(new_state == GST_STATE_PAUSED)
        {
            self->socket.send(gen_message(SERVER_MESSAGE_HEAD_STATE, SERVER_MESSAGE_NECK_PAUSED));
        }
        if(new_state == GST_STATE_READY || new_state == GST_STATE_NULL)
        {
            self->socket.send(gen_message(SERVER_MESSAGE_HEAD_STATE, SERVER_MESSAGE_NECK_STOPPED));
        }
    }
}

void Player::handle_pad_add(GstElement *src, GstPad *new_pad, Player* self)
{
    GstCaps     *new_pad_caps = NULL;
    const gchar *new_pad_type = NULL;

    GstPad *audio_convert_sink_pad = gst_element_get_static_pad(self->audio_convert, "sink");
    GstPad *video_convert_sink_pad = gst_element_get_static_pad(self->video_convert, "sink");
    if(!audio_convert_sink_pad || !video_convert_sink_pad)
    {
        g_printerr("gst_element_get_static_pad failed.\n");
        goto end;
    }

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    if(!new_pad_caps)
    {
        g_printerr("gst_pad_get_current_caps failed.\n");
        goto end;
    }
    new_pad_type = gst_structure_get_name(gst_caps_get_structure(new_pad_caps, 0));
    if(!new_pad_type)
    {
        g_printerr("gst_structure_get_name failed.\n");
        goto end;
    }

    if(g_str_has_prefix(new_pad_type, "audio/x-raw") && !gst_pad_is_linked(audio_convert_sink_pad))
    {
        if(GST_PAD_LINK_FAILED(gst_pad_link(new_pad, audio_convert_sink_pad)))
            g_print("Type is '%s' but link failed.\n", new_pad_type);
    }
    if(g_str_has_prefix(new_pad_type, "video/x-raw") && !gst_pad_is_linked(video_convert_sink_pad))
    {
        if(GST_PAD_LINK_FAILED(gst_pad_link(new_pad, video_convert_sink_pad)))
            g_print("Type is '%s' but link failed.\n", new_pad_type);
    }

end:
    // free resource
    if(!new_pad_caps)           {gst_caps_unref(new_pad_caps);}
    if(!audio_convert_sink_pad) {gst_object_unref(audio_convert_sink_pad);}
    if(!video_convert_sink_pad) {gst_object_unref(video_convert_sink_pad);}
}

void Player::deamon()
{
    GMainLoop* main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // deamon exit
    g_main_loop_unref(main_loop);
}
