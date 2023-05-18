#ifndef CLIENT_H
#define CLIENT_H

#include <string>

enum PlayState{PLAYING, PAUSED, STOPPED};
enum PlayerType{AUDIO_PLAYER, VIDEO_PLAYER};

class MediaPlayer
{
private:

    int    state;
    bool   muted;
    double speed;
    int    player_type;
    bool   full_screen_now;

    void*  socket;
    std::string p_player;

    void (*eos_occur)();
    void (*err_occur)(int);
    void (*state_changed)(int);
    void (*position_changed)(int);
    void (*duration_changed)(int);

    static void* backstage_thread(void* media_player);

public:

    MediaPlayer(int player_type);

    bool stop();
    void mute();
    void seek(int position);
    bool play(std::string path);
    bool pause();
    void unmute();
    bool resume();
    void set_speed(double speed);
    void set_win_screen();
    void set_full_screen();

    void set_call_back_func_eos_occur(       void (*eos_occur)());
    void set_call_back_func_err_occur(       void (*err_occur)(int));
    void set_call_back_func_state_changed(   void (*state_changed)(int));
    void set_call_back_func_position_changed(void (*position_changed)(int));
    void set_call_back_func_duration_changed(void (*duration_changed)(int));
};

#endif // CLIENT_H
