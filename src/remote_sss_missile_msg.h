/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef REMOTE_SSS_MISSILE_MSG_H
#define REMOTE_SSS_MISSILE_MSG_H

struct Remote_sss_create_missile_msg
{
  float mass;
  float length;
};

struct Remote_sss_destroy_missile_msg
{
};

struct Remote_sss_update_missile_msg
{
};

#endif
