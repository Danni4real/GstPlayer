#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#include <string>
#include <iostream>

#include "tools.h"
#include "client.h"
#include "config.h"
#include "socket.h"

std::string get_my_tid()
{
    std::string tid_str;
    char        tid_chars[64] = {0};

    sprintf(tid_chars,"%ld",syscall(SYS_gettid));

    tid_str = tid_chars;

    return tid_str;
}

void* MediaPlayer::backstage_thread(void* media_player)
{
    MediaPlayer* player = (MediaPlayer*)media_player;

    Socket* sock  = (Socket*)player->socket;

    std::string client_sock_file_path = CLIENT_SOCKET_FILE_PATH_PREFIX;
    client_sock_file_path += get_my_tid();

    sock->open_recv_sock(client_sock_file_path.c_str());
    sock->open_send_sock(SERVER_SOCKET_FILE_PATH);

    if(player->player_type == AUDIO_PLAYER)
        sock->send(gen_message(CLIENT_MESSAGE_HEAD_REG,CLIENT_MESSAGE_NECK_AUDIO,client_sock_file_path));
    if(player->player_type == VIDEO_PLAYER)
        sock->send(gen_message(CLIENT_MESSAGE_HEAD_REG,CLIENT_MESSAGE_NECK_VIDEO,client_sock_file_path));

    while(true)
    {
        if(sock->recv() > 0)
        {
            std::string msg      = sock->get_recv_buf();
            std::string msg_head = extract_field(msg,0);
            std::string msg_neck = extract_field(msg,1);

            std::cout << "client recvd:" << msg << std::endl;

            if(equal(msg_head, SERVER_MESSAGE_HEAD_EOS))
            {
                if(player->eos_occur != 0)
                    player->eos_occur();
            }
            else if(equal(msg_head, SERVER_MESSAGE_HEAD_ERROR))
            {
                if(player->err_occur != 0)
                    player->err_occur(0);
            }
            else if(equal(msg_head, SERVER_MESSAGE_HEAD_PPLAYER))
            {
                player->p_player = msg_neck;
            }
            else if(equal(msg_head, SERVER_MESSAGE_HEAD_POSITION))
            {
                int pos = to_int(msg_neck);

                if(player->position_changed != 0)
                    player->position_changed(pos);
            }
            else if(equal(msg_head, SERVER_MESSAGE_HEAD_DURATION))
            {
                int dur = to_int(msg_neck);

                if(player->duration_changed != 0)
                    player->duration_changed(dur);
            }
            else if(equal(msg_head, SERVER_MESSAGE_HEAD_STATE))
            {
                std::string state = msg_neck;

                if(equal(state, SERVER_MESSAGE_NECK_PAUSED))
                {
                    if(player->state_changed != 0)
                        player->state_changed(PAUSED);

                    player->state = PAUSED;
                }
                if(equal(state, SERVER_MESSAGE_NECK_PLAYING))
                {
                    if(player->state_changed != 0)
                        player->state_changed(PLAYING);

                    player->state = PLAYING;
                }
                if(equal(state, SERVER_MESSAGE_NECK_STOPPED))
                {
                    if(player->state_changed != 0)
                        player->state_changed(STOPPED);

                    player->state = STOPPED;
                }
            }
        }
    }
}

MediaPlayer::MediaPlayer(int _player_type)
{
    state            = STOPPED;
    muted            = false;
    speed            = 1.0;
    socket           = new Socket();
    p_player         = "0";
    full_screen_now  = false;

    eos_occur        = 0;
    err_occur        = 0;
    state_changed    = 0;
    position_changed = 0;
    duration_changed = 0;

    player_type = _player_type;

    int       ret;
    pthread_t tid;
    ret = pthread_create(&tid, NULL, backstage_thread, this);
    if(ret != 0)
    {
        std::cout << "backstage_thread create failed!" << std::endl;
        return;
    }
}


bool MediaPlayer::stop()
{
    if(state == STOPPED)
        return true;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_STOP, p_player));

    int counts = 1000;
    while(counts-- > 0)
    {
        if(state == STOPPED)
            return true;

        usleep(10000);
    }

    std::cout << "stop() time out!" << std::endl;

    return false;
}

void MediaPlayer::mute()
{
    if(state == STOPPED || muted == true)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_MUTE, p_player));

    muted = true;
}

void MediaPlayer::seek(int position)
{
    if(state == STOPPED)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_SEEK, p_player, position));
}

bool MediaPlayer::play(std::string path)
{
    if(state != STOPPED)
        stop();

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_PLAY, p_player, path));

    int counts = 1000;
    while(counts-- > 0)
    {
        if(state == PLAYING)
            return true;

        usleep(10000);
    }

    std::cout << "play() time out!" << std::endl;

    return false;
}

bool MediaPlayer::pause()
{
    if(state != PLAYING)
        return true;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_PAUSE, p_player));

    int counts = 1000;
    while(counts-- > 0)
    {
        if(state == PAUSED)
            return true;

        usleep(10000);
    }

    std::cout << "pause() time out!" << std::endl;

    return false;
}

void MediaPlayer::unmute()
{
    if(state == STOPPED || muted == false)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_UNMUTE, p_player));

    muted = false;
}

bool MediaPlayer::resume()
{
    if(state != PAUSED)
        return true;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_RESUME, p_player));

    int counts = 1000;
    while(counts-- > 0)
    {
        if(state == PLAYING)
            return true;

        usleep(10000);
    }

    std::cout << "pause() time out!" << std::endl;

    return false;
}

void MediaPlayer::set_speed(double _speed)
{
    if(speed == _speed)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_SPEED, p_player, _speed));

    speed = _speed;
}

void MediaPlayer::set_win_screen()
{
    if(full_screen_now == false)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_WINSCREEN, p_player));

    full_screen_now = false;
}

void MediaPlayer::set_full_screen()
{
    if(full_screen_now == true)
        return;

    Socket* sock = (Socket*)socket;

    sock->send(gen_message(CLIENT_MESSAGE_HEAD_FULLSCREEN, p_player));

    full_screen_now = true;
}

#define COMPILE_CLIENT
#ifdef COMPILE_CLIENT

int main()
{
    MediaPlayer* player = 0;

    MediaPlayer audio_player_1(AUDIO_PLAYER);
    MediaPlayer audio_player_2(AUDIO_PLAYER);
    MediaPlayer audio_player_3(AUDIO_PLAYER);
    MediaPlayer video_player_1(VIDEO_PLAYER);
    MediaPlayer video_player_2(VIDEO_PLAYER);
    MediaPlayer video_player_3(VIDEO_PLAYER);

    while(true)
    {
        printf("1. resume\n"
               "2. pause\n"
               "3. stop\n"
               "4. mute\n"
               "5. unmute\n"
               "6. seek\n"
               "7. play\n"
               "8. set speed\n"
               "9. full screen\n"
               "10.window screen\n"
               "11.switch to audio player 1\n"
               "12.switch to audio player 2\n"
               "13.switch to audio player 3\n"
               "14.switch to video player 1\n"
               "15.switch to video player 2\n"
               "16.switch to video player 3\n");

        int cmd;
        scanf("%d", &cmd);

        switch(cmd)
        {
        case 1:
            player->resume();
            break;
        case 2:
            player->pause();
            break;
        case 3:
            player->stop();
            break;
        case 4:
            player->mute();
            break;
        case 5:
            player->unmute();
            break;
        case 6:
        {
            int seek_value;
            printf("seek to: ");
            scanf("%d", &seek_value);

            player->seek(seek_value);
            break;
        }
        case 7:
        {
            char path[128] = {0};
            printf("file path: ");
            scanf("%s", path);

            player->play(path);
            break;
        }
        case 8:
        {
            double speed;
            printf("speed: ");
            scanf("%lf", &speed);
            player->set_speed(speed);
            break;
        }
        case 9:
        {
            player->set_full_screen();
            break;
        }
        case 10:
        {
            player->set_win_screen();
            break;
        }
        case 11:
        {
            if(player != 0)
                player->pause();

            player = &audio_player_1;
            break;
        }
        case 12:
        {
            if(player != 0)
                player->pause();

            player = &audio_player_2;
            break;
        }
        case 13:
        {
            if(player != 0)
                player->pause();

            player = &audio_player_3;
            break;
        }
        case 14:
        {
            if(player != 0)
                player->pause();

            player = &video_player_1;
            break;
        }
        case 15:
        {
            if(player != 0)
                player->pause();

            player = &video_player_2;
            break;
        }
        case 16:
        {
            if(player != 0)
                player->pause();

            player = &video_player_3;
            break;
        }
        default:
            break;
        }
    }
}

#endif
