#ifndef PLAYER_H
#define PLAYER_H

#include <gst/gst.h>
#include "socket.h"

#define AUDIO_PLAYER 0
#define VIDEO_PLAYER 1

class Player
{
private:
    gint32      player_type;
    gint32      duration;
    gint32      position;
    gboolean    seekable;
    GstState    state;

    GstElement* pipeline;
    GstElement* uridecodebin;
    GstElement* audio_convert;
    GstElement* audio_resample;
    GstElement* audio_volume;
    GstElement* audio_sink;
    GstElement* video_convert;
    GstElement* video_resize;
    GstElement* video_sink;

    Socket socket;

public:
    Player(gint32 player_type);

    void bind_to_client(const gchar* client_socket_file_path);

    gboolean init();
    void     deinit();

    void play();
    void stop();
    void pause();
    static void deamon();
    void query_seekable();

    void seek(guint seconds);
    void set_path(const gchar* path);
    void set_speed(gdouble speed);
    void mute(gboolean true_is_mute);
    void resize_video(guint width, guint height);
    void set_display(guint x_start, guint y_start, guint width, guint height);

    static void handle_eos(GstBus*,             GstMessage *msg, Player* self);
    static void handle_error(GstBus*,           GstMessage *msg, Player* self);
    static void query_duration(GstBus*,         GstMessage *msg, Player* self);
    static void handle_state_change(GstBus*,    GstMessage *msg, Player* self);
    static void handle_pad_add(GstElement *src, GstPad *new_pad, Player* self);

    static gboolean query_position(Player* self);
};

#endif // PLAYER_H
