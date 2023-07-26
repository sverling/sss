/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  Variometer code added by Esteban Ruiz on August 2006 - cerm78@gmail.com

  \file vario.cpp

*/

#include "vario.h"
#include "config_file.h"
#include "config.h"
#include "glider.h"
#include "audio.h"
#include "physics.h"
#include "sss.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
Vario::Vario(Config_file & config_file, const Glider * glider)
  : Object(glider->get_eye()),
    silence(false),
    speed_variation(0.5f),
    volume(0.8),
    dz_max(0.5f),
    dz_min(-0.5f),
    dz_vol(0.15f),
    max_speed(15),
    min_speed(-15),
    rate_base(1),
    speed_div(10),
    location(Config::EYE)
{
  TRACE_METHOD_ONLY(1);
  parent_glider = glider;

  // Initialize values from the general SSS config file
  string vario_audio_file(Sss::instance()->config().vario_audio_file);
  volume = Sss::instance()->config().vario_volume;
  speed_variation = Sss::instance()->config().vario_speed_variation;
  max_speed = Sss::instance()->config().vario_max_speed;
  min_speed = Sss::instance()->config().vario_min_speed;
  speed_div = Sss::instance()->config().vario_speed_div;
  rate_base = Sss::instance()->config().vario_rate_base;
  dz_max = Sss::instance()->config().vario_dz_max;
  dz_min = Sss::instance()->config().vario_dz_min;
  dz_vol = Sss::instance()->config().vario_dz_vol;
  location = Sss::instance()->config().vario_location;

  // Initialize the vario parameters from the glider config file
  config_file.get_value("vario_audio_file", vario_audio_file);
  config_file.get_value("vario_volume", volume);
  config_file.get_value("vario_speed_var", speed_variation);
  config_file.get_value("vario_dz_max", dz_max);
  config_file.get_value("vario_dz_min", dz_min);
  config_file.get_value("vario_dz_vol", dz_vol);
  config_file.get_value("vario_max_speed", max_speed);
  config_file.get_value("vario_min_speed", min_speed);
  config_file.get_value("vario_rate_base", rate_base);
  config_file.get_value("vario_speed_div", speed_div);
  string vl = "";
  config_file.get_value("vario_location", vl);
  if (vl == "eye")
  {
    location = Config::EYE;
    volume = volume + 0.5f;
  }
  else if (vl ==  "glider")
    location = Config::GLIDER;

  Audio::instance()->register_instance(this,vario_audio_file,0,1.0f,400.0f,true);
  Physics::instance()->register_object(this);
  Sss::instance()->add_object(this);
}

Vario::~Vario()
{
  TRACE_METHOD_ONLY(1);
  Audio::instance()->deregister_instance(this);
  Physics::instance()->deregister_object(this);
  Sss::instance()->remove_object(this);
}

//////////////////////////////////////////////////////////////////////
// Physic tasks
//////////////////////////////////////////////////////////////////////
void Vario::pre_physics(float dt)
{
  // If the parent glider is not the main glider, the current vario
  // will always be at the glider
  if ((int)parent_glider != (int)&Sss::instance()->glider())
    this->set_pos(parent_glider->get_pos());
  else
  {
    // If the parent glider is the main galider, then we set the vario
    // location accordingly
    switch (location)
    {
      // Variometer at eye position (glider, body, fixed position,
      // chasing position, etc.)
    case Config::EYE:
      this->set_pos(Sss::instance()->eye().get_eye());
      break;
      // Variometer at glider cabin
    case Config::GLIDER:
      this->set_pos(parent_glider->get_pos());
      break;
    }
  }
}

void Vario::post_physics(float dt)
{
  TRACE_METHOD_ONLY(3);

  // If vario has been silenced, set volume to cero and don't do
  // anything else for now
  if (silence)
    Audio::instance()->set_vol_scale(this, 0);
  else
  {
    // Get the parent glider vertical speed (currently relative to
    // ground, but can be relative to wind to compensate for speed
    // loss elevation)
    float verticalSpeed = parent_glider->get_vel()[2];
    // If the new vertical speed is out of the configurated speed
    // variation range, proceed to adjust the vario rate
    if (verticalSpeed >= last_vertical_speed + speed_variation ||
        verticalSpeed <= last_vertical_speed - speed_variation)
    {
      // This is our last known vertical speed
      last_vertical_speed = verticalSpeed;
      float rate, vol = volume;
      // If the vertical speed is in the "dead zone" reduce the vario
      // volumen to specified value. This is designed mainly to shut
      // off vario when the vertical speed is near zero, but can be
      // easily disabled by setting dz_max = dz_min, or dz_vol =
      // volume
      if (verticalSpeed < dz_max && verticalSpeed > dz_min)
        vol = dz_vol;
      // If the current vertical speed is within the allowed range,
      // proceed with the rate calculation and audio update
      if (verticalSpeed <= max_speed && verticalSpeed >= min_speed)
      {
        // Calculate the vario rate based on vertical speed.
        rate = rate_base + (verticalSpeed / speed_div);
        // Update audio volume and frequency rate
        Audio::instance()->set_pitch_scale(this, rate);
        Audio::instance()->set_vol_scale(this, vol);
      }
    }
  }
}
