/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file audio.cpp
*/
#include "audio.h"
#include "sss_assert.h"
#include "object.h"
#include "log_trace.h"
#include "sss.h"
#include "config.h"
#include <iostream>
using namespace std;

Audio * Audio::m_instance = 0;

#ifdef WITH_FMOD

#ifdef _WIN32
// unbelievable - headers are located in different place on different platforms!
#include <fmod.h>
#include <fmod_errors.h>
#else
#include <fmodex/fmod.h>
#include <fmodex/fmod_errors.h>
#endif

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
//#include <wincompat.h>
#endif

FMOD_SYSTEM *fmod_system = 0;
FMOD_RESULT result;

Audio::Audio()
{
  TRACE_METHOD_ONLY(1);
  
  m_enable = Sss::instance()->config().use_audio;
  if (!m_enable)
    return;
  
  result = FMOD_System_Create(&fmod_system);
  if (result != FMOD_OK)
  {
    TRACE("Unable to create FMOD system: %s\n", FMOD_ErrorString(result));
    Sss::instance()->set_config().use_audio = false;
    m_enable = false;
    return;
  }
  assert1(fmod_system);
  
  result = FMOD_System_Init(fmod_system, 32, FMOD_INIT_NORMAL, 0);
  if (result != FMOD_OK)
  {
    TRACE("Unable to initialise FMOD system: %s\n", FMOD_ErrorString(result));
    Sss::instance()->set_config().use_audio = false;
    m_enable = false;
    fmod_system = 0;
    return;
  }
  
    
  /*
    if (!(FMOD_RESULT result = FMOD_System_Init(44100, 32, 0)))
    {
    TRACE("Init: %s\n", FMOD_ErrorString(FMODE_ErrorString(result)));
    TRACE("Error opening audio, so disabling it\n");
    Sss::instance()->set_config().use_audio = false;
    m_enable = false;
    return;
    }
  */

  FMOD_CHANNELGROUP *channel_group;
  result = FMOD_System_GetMasterChannelGroup(fmod_system, &channel_group);
  if (result != FMOD_OK)
    TRACE("FMOD Get master channel error: %s\n", FMOD_ErrorString(result));
  result = FMOD_ChannelGroup_SetVolume(channel_group, 1.0f);
  if (result != FMOD_OK)
    TRACE("FMOD set master channel volume error: %s\n", FMOD_ErrorString(result));

  
  TRACE_FILE_IF(1)
    TRACE("FMOD setup done\n");
}

void Audio::register_instance(Object * object,
                              string & file,
                              float vol_scale,
                              float pitch_scale,
                              float dist2_offset,
                              bool loop)
{
  if (!m_enable)
    return;
  TRACE_FILE_IF(1) {
    TRACE_METHOD(); TRACE("Registering %p with audio file %s\n", object, file.c_str());}
  //  cout << "Registering " << object << " with audio file " << file << "\n";
  Audio_params params;
  params.vol_scale = vol_scale;
  params.pitch_scale = pitch_scale;
  params.dist2_offset = dist2_offset;
  string real_file = "sounds/" + file;
  result = FMOD_System_CreateSound(fmod_system, real_file.c_str(),
                                   FMOD_3D, 0, &params.sample);
  if (!params.sample)
  {
    TRACE("FMOD create sound: %s\n", FMOD_ErrorString(result));
    assert1(!"Error");    
  }
  
  result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, 
                                 params.sample, false, &params.channel);
  
  // increasing mindistance makes it louder in 3d space
  result = FMOD_Sound_Set3DMinMaxDistance(params.sample, 1.0f, 200.0f);

  if (loop)
    FMOD_Sound_SetMode(params.sample, FMOD_LOOP_NORMAL);
  else
    FMOD_Sound_SetMode(params.sample, FMOD_LOOP_OFF);
  
  params.paused = true;
  
  result = FMOD_Channel_GetVolume(params.channel, &params.orig_vol);
  if (result != FMOD_OK)
    TRACE("FMOD Get Vol error: %s\n", FMOD_ErrorString(result));
  result = FMOD_Channel_GetFrequency(params.channel, &params.orig_freq);
  if (result != FMOD_OK)
    TRACE("FMOD Get Freq error: %s\n", FMOD_ErrorString(result));
  

  FMOD_Channel_SetVolume(params.channel, 0.0f);

  bool ok = 
    (m_instances.insert(Instance_map::value_type(object, params))).second;
  assert1(ok);
}


void Audio::deregister_instance(Object * object)
{
  if (!m_enable)
    return;
  TRACE_FILE_IF(1) {
    TRACE_METHOD(); TRACE("Deregistering %p\n", object);}
  Audio_params params = m_instances[object];
  result = FMOD_Sound_Release(params.sample);  
  if (result != FMOD_OK)
    TRACE("FMOD release error: %s\n", FMOD_ErrorString(result));
  m_instances.erase(object);
}


void Audio::set_vol_scale(Object * object,
                          float vol_scale)
{
  if (!m_enable)
    return;
  m_instances[object].vol_scale = vol_scale * 0.5;
}

void Audio::set_pitch_scale(Object * object,
                            float pitch_scale)
{
  if (!m_enable)
    return;
  m_instances[object].pitch_scale = pitch_scale;
}

void Audio::set_dist2_offset(Object * object,
                             float dist2_offset)
{
  if (!m_enable)
    return;
  m_instances[object].dist2_offset = dist2_offset;
}

void Audio::do_audio(const Object * ear)
{
  if (!m_enable)
    return;
  // FMOD treats +X as right, +Y as up, and +Z as forwards.
  FMOD_VECTOR fmod_vel;
  FMOD_VECTOR fmod_pos;
  FMOD_VECTOR fmod_fwd;
  FMOD_VECTOR fmod_up;
  
  const Velocity & ear_vel = ear->get_vel();
  const Position & ear_pos = ear->get_eye();
  const Vector & ear_fwd = (ear->get_eye_target() - ear->get_eye()).normalise();
  const Vector & ear_up = ear->get_eye_up();
  
  fmod_vel.x = -ear_vel[1];
  fmod_vel.y = ear_vel[2];
  fmod_vel.z = ear_vel[0];
  fmod_pos.x = -ear_pos[1];
  fmod_pos.y = ear_pos[2];
  fmod_pos.z = ear_pos[0];
  fmod_fwd.x = -ear_fwd[1];
  fmod_fwd.y = ear_fwd[2];
  fmod_fwd.z = ear_fwd[0];
  fmod_up.x = -ear_up[1];
  fmod_up.y = ear_up[2];
  fmod_up.z = ear_up[0];
  
  result = FMOD_System_Set3DListenerAttributes(
    fmod_system, 0, &fmod_pos, &fmod_vel, &fmod_fwd, &fmod_up);
  if (result != FMOD_OK)
    TRACE("FMOD set 3D listener att error: %s\n", FMOD_ErrorString(result));
  
  Instance_map::iterator it;
  for (it = m_instances.begin() ;
       it != m_instances.end() ; 
       ++it)
  {
    Object * object = it->first;
    Audio_params &ap = it->second;
    assert1(object);
    
    float freq = (ap.pitch_scale * ap.orig_freq);
    float vol = (Sss::instance()->config().global_audio_scale * 
                 ap.vol_scale * ap.orig_vol);
    
    if (vol < 0.0f) vol = 0.0f;
    else if (vol > 1.0f) vol = 1.0f;
    
    const Velocity & vel = object->get_vel();
    const Position & pos = object->get_pos();
    
    fmod_vel.x = -vel[1];
    fmod_vel.y = vel[2];
    fmod_vel.z = vel[0];
    fmod_pos.x = -pos[1];
    fmod_pos.y = pos[2];
    fmod_pos.z = pos[0];
    

    // for some reason FMOD kicks out my sounds!
    FMOD_BOOL is_playing;
    result = FMOD_Channel_IsPlaying(ap.channel, &is_playing);
    if (!is_playing)
    {
      result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, 
                                     ap.sample, false, &ap.channel);
    }


    result = FMOD_Channel_Set3DAttributes(
      ap.channel, &fmod_pos, &fmod_vel);
    if (result != FMOD_OK)
      TRACE("FMOD Set3DAttributes error: %s\n", FMOD_ErrorString(result));

    
    result = FMOD_Channel_SetFrequency(ap.channel, freq);
    if (result != FMOD_OK)
      TRACE("FMOD Set Freq error: %s\n", FMOD_ErrorString(result));

    result = FMOD_Channel_SetVolume(ap.channel, vol);
    if (result != FMOD_OK)
      TRACE("FMOD Set Vol error: %s\n", FMOD_ErrorString(result));

    if (ap.paused)
    {
      result = FMOD_Channel_SetPaused(ap.channel, false);
      if (result != FMOD_OK)
        TRACE("FMOD Set Paused error: %s\n", FMOD_ErrorString(result));

      ap.paused = false;
    }
  }

  FMOD_System_Update(fmod_system);
}

#else // not fmod
#ifdef WITH_PLIB
#include <plib/sl.h>
#include <plib/sm.h>

Audio::Audio()
{
  TRACE_METHOD_ONLY(1);
  m_enable = Sss::instance()->config().use_audio;
  if (!m_enable)
    return;
  m_scheduler = new slScheduler(8000);
  m_mixer = new smMixer("/dev/mixer");
  m_mixer->setMasterVolume ( 100 ) ;
  // Windows seems to cope with a safety margin < 0.128, but linux
  // needs .128?
  m_scheduler->setSafetyMargin ( 0.128f ) ;
  
}

void Audio::register_instance(Object * object,
                              string & file,
                              float vol_scale,
                              float pitch_scale,
                              float dist2_offset,
                              bool loop)
{
  if (!m_enable)
    return;
  TRACE_FILE_IF(1) {
    TRACE_METHOD(); TRACE("Registering %p with audio file %s\n", object, file.c_str());}
  //  cout << "Registering " << object << " with audio file " << file << "\n";
  Audio_params params;
  params.vol_scale = vol_scale;
  params.pitch_scale = pitch_scale;
  params.dist2_offset = dist2_offset;
  params.rate_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
  params.vol_envelope = new slEnvelope( 1, SL_SAMPLE_ONE_SHOT );
  string real_file = "sounds/" + file;
  params.sample = new slSample(real_file.c_str(), m_scheduler);
//  cout << "sample = " << params.sample << endl;
  if (loop)
    m_scheduler->loopSample(params.sample);
  else
    m_scheduler->playSample(params.sample);
  
  bool ok = 
    (m_instances.insert(Instance_map::value_type(object, params))).second;
  assert1(ok);
  
//  cout << "done\n";
}


void Audio::deregister_instance(Object * object)
{
  if (!m_enable)
    return;
  TRACE_FILE_IF(1) {
    TRACE_METHOD(); TRACE("Deregistering %p\n", object);}
  Audio_params params = m_instances[object];
  m_scheduler->stopSample(params.sample);
  m_scheduler->addSampleEnvelope(
    params.sample, 0, 1, 0, SL_PITCH_ENVELOPE);
  m_scheduler->addSampleEnvelope(
    params.sample, 0, 0, 0, SL_VOLUME_ENVELOPE);
  m_scheduler->update();
// plib is broken so that deleting the sample makes it crash. For now just leak it...
//  delete params.rate_envelope;
//  delete params.vol_envelope;
//  delete params.sample;
  m_instances.erase(object);
}


void Audio::set_vol_scale(Object * object,
                          float vol_scale)
{
  if (!m_enable)
    return;
  m_instances[object].vol_scale = vol_scale;
}

void Audio::set_pitch_scale(Object * object,
                            float pitch_scale)
{
  if (!m_enable)
    return;
  m_instances[object].pitch_scale = pitch_scale;
}

void Audio::set_dist2_offset(Object * object,
                             float dist2_offset)
{
  if (!m_enable)
    return;
  m_instances[object].dist2_offset = dist2_offset;
}

void Audio::do_audio(const Object * ear)
{
  if (!m_enable)
    return;
  Instance_map::iterator it;
//  int count = 0;
  for (it = m_instances.begin() ;
       it != m_instances.end() ; 
       ++it)
  {
//    TRACE("%d\n", count++);
    Object * object = it->first;
    Audio_params &ap = it->second;
    assert1(object);
    
    float rate = ap.pitch_scale;
    float vol = Sss::instance()->config().global_audio_scale * 
      ap.vol_scale;
    float dist2_offset = ap.dist2_offset;
    
    const Velocity & vel = object->get_vel();
    const Position & pos = object->get_pos();
    
    float dist2 = (pos - ear->get_eye()).mag2();
    float rel_vel = 0.0f;
    // Doppler
    if (dist2 > 0.001)
    {
      Vector dir = (pos - ear->get_eye()).normalise();
      rel_vel = dot((vel - ear->get_vel()), dir);
      rate *= (1 - rel_vel / 330.0); // speed of sound
    }
    
//    cout << "sample = " << ap.sample << endl;
    
    assert1(ap.sample);
    assert1(ap.rate_envelope);
    ap.rate_envelope->setStep(0, 0.0, rate);
    m_scheduler->addSampleEnvelope(
      ap.sample, 0, 1, ap.rate_envelope, SL_PITCH_ENVELOPE);
    
    // volume
    vol *= dist2_offset/(dist2_offset + dist2);
    
    // get compression of waves when the source comes towards you -
    // I guessed this next equation...
//    vol *= (1 - rel_vel/330 ) * (1 - rel_vel/330);
    
    assert1(ap.vol_envelope);
    ap.vol_envelope->setStep(0, 0.0, vol);
    m_scheduler->addSampleEnvelope(
      ap.sample, 0, 0, ap.vol_envelope, SL_VOLUME_ENVELOPE);
  }
  m_scheduler->update();
}

#else // no plib - just stub out

Audio::Audio()
{
  TRACE_METHOD_ONLY(1);
}

void Audio::register_instance(Object * object,
                              std::string & file,
                              float vol_scale,
                              float pitch_scale,
                              float dist2_offset,
                              bool loop)
{
}


void Audio::deregister_instance(Object * object)
{
}


void Audio::set_vol_scale(Object * object,
                          float vol_scale)
{
}

void Audio::set_pitch_scale(Object * object,
                            float pitch_scale)
{
}

void Audio::set_dist2_offset(Object * object,
                             float dist2_offset)
{
}

void Audio::do_audio(const Object * ear)
{
}


#endif // with plib
#endif // with FMOD
