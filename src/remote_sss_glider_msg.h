/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef REMOTE_SSS_GLIDER_MSG_H
#define REMOTE_SSS_GLIDER_MSG_H

#include "types.h"

struct Remote_sss_create_glider_msg
{
  char glider_file[32];
};

struct Remote_sss_destroy_glider_msg
{
};

struct Remote_sss_update_glider_msg
{
  enum Update_type
  {
    NORMAL,
    CONFIG_FILE,
    MISSILE_HIT,
    RESET_IN_RACE
  };
  
  Update_type update_type;
  struct  Normal_update
  {
    bool paused;
    unsigned char joystick[9];
    unsigned char trims[9];
  };
  
  struct Missile_hit
  {
    float missile_mass;
    float missile_pos[3];
    float missile_vel[3];
  };
  
  struct Config_file_update
  {
    char glider_file[32];
  };

  union
  {
    Normal_update normal;
    Missile_hit missile_hit;
    Config_file_update config_file;
  } update;
};

#endif
