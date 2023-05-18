#ifndef CONFIG_H
#define CONFIG_H

#define PLANE_ID             70
#define FULL_SCREEN_WIDTH    1920
#define FULL_SCREEN_HEIGHT   720
#define WINDOW_SCREEN_WIDTH  1486
#define WINDOW_SCREEN_HEIGHT 630

#define AUDIO_PLAYER_NUM 3
#define VIDEO_PLAYER_NUM 3

#define CLIENT_MESSAGE_HEAD_REG        "$REG"        // register
#define CLIENT_MESSAGE_HEAD_PLAY       "$PLAY"
#define CLIENT_MESSAGE_HEAD_STOP       "$STOP"
#define CLIENT_MESSAGE_HEAD_SEEK       "$SEEK"
#define CLIENT_MESSAGE_HEAD_MUTE       "$MUTE"
#define CLIENT_MESSAGE_HEAD_PAUSE      "$PAUSE"
#define CLIENT_MESSAGE_HEAD_SPEED      "$SPEED"
#define CLIENT_MESSAGE_HEAD_RESUME     "$RESUME"
#define CLIENT_MESSAGE_HEAD_UNMUTE     "$UNMUTE"
#define CLIENT_MESSAGE_HEAD_WINSCREEN  "$WINSCREEN"  // window screen
#define CLIENT_MESSAGE_HEAD_FULLSCREEN "$FULLSCREEN" // full   screen

#define CLIENT_MESSAGE_NECK_AUDIO      "AUDIO"
#define CLIENT_MESSAGE_NECK_VIDEO      "VIDEO"

#define SERVER_MESSAGE_HEAD_EOS        "#EOS"
#define SERVER_MESSAGE_HEAD_ERROR      "#ERROR"
#define SERVER_MESSAGE_HEAD_STATE      "#STATE"
#define SERVER_MESSAGE_HEAD_PPLAYER    "#PPLAYER"     // player pointer, used to identify player which client bind to
#define SERVER_MESSAGE_HEAD_DURATION   "#DURATION"
#define SERVER_MESSAGE_HEAD_POSITION   "#POSITION"

#define SERVER_MESSAGE_NECK_PAUSED     "PAUSED"
#define SERVER_MESSAGE_NECK_PLAYING    "PLAYING"
#define SERVER_MESSAGE_NECK_STOPPED    "STOPPED"

#define SERVER_SOCKET_FILE_PATH         "/tmp/media_player_socket_file.server"
#define CLIENT_SOCKET_FILE_PATH_PREFIX  "/tmp/media_player_socket_file.client_"

#endif // CONFIG_H
