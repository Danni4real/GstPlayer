#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <string>
#include <iostream>

#include "tools.h"
#include "config.h"
#include "socket.h"
#include "player.h"

Player* AudioPlayerList[AUDIO_PLAYER_NUM];
Player* VideoPlayerList[VIDEO_PLAYER_NUM];

Player* find_a_idle_player(int player_type)
{
    Player* idle_player = 0;

    static int idle_audio_player_index = 0;
    static int idle_video_player_index = 0;

    if(player_type == AUDIO_PLAYER && idle_audio_player_index < AUDIO_PLAYER_NUM)
    {
        idle_player = AudioPlayerList[idle_audio_player_index];
        idle_audio_player_index++;
    }

    if(player_type == VIDEO_PLAYER && idle_video_player_index < VIDEO_PLAYER_NUM)
    {
        idle_player = VideoPlayerList[idle_video_player_index];
        idle_video_player_index++;
    }

    return idle_player;
}

Player* to_pplayer(std::string str)
{
    return (Player*)atol(str.c_str());
}

void* thread_server(void*)
{
    Socket server;
    server.open_recv_sock(SERVER_SOCKET_FILE_PATH);

    while(true)
    {
        if(server.recv() > 0)
        {
            std::string msg      = server.get_recv_buf();
            std::string msg_head = extract_field(msg,0);
            std::string msg_neck = extract_field(msg,1);

            std::cout << "server recvd:" << msg << std::endl;

            if(equal(msg_head, CLIENT_MESSAGE_HEAD_REG))
            {
                Player* idle_player = 0;

                if(equal(msg_neck, CLIENT_MESSAGE_NECK_AUDIO))
                    idle_player = find_a_idle_player(AUDIO_PLAYER);

                if(equal(msg_neck, CLIENT_MESSAGE_NECK_VIDEO))
                    idle_player = find_a_idle_player(VIDEO_PLAYER);

                if(idle_player != 0)
                {
                    std::string client_socket_file_path = extract_field(msg,2);
                    idle_player->bind_to_client(client_socket_file_path.c_str());
                }
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_PLAY))
            {
                Player*     player    = to_pplayer(msg_neck);
                std::string play_path = extract_field(msg,2);

                player->set_path(play_path.c_str());
                player->play();
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_PAUSE))
            {
                Player* player  = to_pplayer(msg_neck);

                player->pause();
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_RESUME))
            {
                Player* player  = to_pplayer(msg_neck);

                player->play();
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_STOP))
            {
                Player* player  = to_pplayer(msg_neck);

                player->stop();
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_MUTE))
            {
                Player* player  = to_pplayer(msg_neck);

                player->mute(true);
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_UNMUTE))
            {
                Player* player  = to_pplayer(msg_neck);

                player->mute(false);
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_SPEED))
            {
                Player* player  = to_pplayer(msg_neck);
                double  speed   = to_double(extract_field(msg,2));

                player->set_speed(speed);
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_SEEK))
            {
                Player* player   = to_pplayer(msg_neck);
                int     seek_pos = to_int(extract_field(msg,2));

                player->seek(seek_pos);
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_WINSCREEN))
            {
                Player* player   = to_pplayer(msg_neck);

                player->resize_video(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
                player->set_display((FULL_SCREEN_WIDTH -WINDOW_SCREEN_WIDTH)/2 +1,
                                    (FULL_SCREEN_HEIGHT-WINDOW_SCREEN_HEIGHT)/2+1,
                                    FULL_SCREEN_WIDTH,
                                    FULL_SCREEN_HEIGHT);
            }
            else if(equal(msg_head, CLIENT_MESSAGE_HEAD_FULLSCREEN))
            {
                Player* player   = to_pplayer(msg_neck);

                player->resize_video(FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT);
                player->set_display(0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT);
            }
        }
    }
}

#define COMPILE_SERVER
#ifdef COMPILE_SERVER

int main()
{
    gint      ret;
    pthread_t tid;

    ret = pthread_create(&tid, NULL, thread_server, NULL);
    if(ret != 0)
    {
        g_printerr("thread_server create failed!");
        return 1;
    }

    for(int i=0;i< AUDIO_PLAYER_NUM;i++)
    {
        AudioPlayerList[i] = new Player(AUDIO_PLAYER);

        if(!AudioPlayerList[i]->init())
            return 1;
    }

    for(int i=0;i< VIDEO_PLAYER_NUM;i++)
    {
        VideoPlayerList[i] = new Player(VIDEO_PLAYER);

        if(!VideoPlayerList[i]->init())
            return 1;
    }

    Player::deamon();
}

#endif
