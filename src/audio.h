/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_AUDIO_H
#define SSS_AUDIO_H

#include <map>
#include <string>

class slEnvelope;
class slScheduler;
class slSample;
class smMixer;
class Object;

struct FMOD_SOUND;
struct FMOD_CHANNEL;

/*! This singleton handles all the audio for all objects, doing
    Doppler, distance stuff etc on them. The instance can modify its
    modifiers at any time - e.g. once per frame for a sound property
    that changes with time (like an explosion).  */

class Audio { 
public: 
  inline static Audio * instance() ;
  
  void register_instance(Object * object,
                         std::string & file,
                         float vol_scale,
                         float pitch_scale,
                         float dist2_offset,
                         bool loop = true);
  
  void deregister_instance(Object * object);

  void set_vol_scale(Object * object,
                     float vol_scale);
  void set_pitch_scale(Object * object,
                       float pitch_scale);
  void set_dist2_offset(Object * object,
                        float dist2_offset);

  void do_audio(const Object * ear);
  
private:
  Audio();
  static Audio * m_instance;

  struct Audio_params
  {
    float vol_scale;
    float pitch_scale;
    float dist2_offset;

#ifdef WITH_FMOD
    FMOD_SOUND * sample;
    FMOD_CHANNEL * channel;
    float orig_freq;
    float orig_vol;
    bool paused;
#else
    slEnvelope * rate_envelope;
    slEnvelope * vol_envelope;
    slSample * sample;
#endif
  };
  
  std::map<Object *, Audio_params> m_instances;
  typedef std::map<Object *, Audio_params> Instance_map;
  
  slScheduler * m_scheduler;
  smMixer * m_mixer;
  bool m_enable;
};

Audio * Audio::instance()
{
  return m_instance ? m_instance : (m_instance = new Audio);
}


#endif
