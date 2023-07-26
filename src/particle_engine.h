/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PARTICLE_ENGINE_H
#define PARTICLE_ENGINE_H

#include "types.h"

#include <map>
#include <vector>

class Object;
class Particle_source;

/// The types of particle supported. These define the texture,
/// blending, and behaviour...
enum Particle_type {PARTICLE_SMOKE_LIGHT,
		    PARTICLE_SMOKE_MEDIUM,
		    PARTICLE_SMOKE_DARK,
		    PARTICLE_FLAME};

class Particle_engine
{
public:

  static inline Particle_engine * instance();

  /// @returns ID for the new source.
  /// @param num: The max number of particles in use by this source
  /// @param type: Type of particle emitted by this source
  /// @param particle_size: Size of the billboard used for this particle
  int register_source(Particle_type type,
                      int num,
                      const Position & pos,
                      const Velocity & vel,
                      float alpha_scale,
                      float init_size,
                      float max_size,
                      float lifetime,
                      float rate);

  /// deregisters a source. If wait is true, then the particles will
  /// be maintained until they expire. The source will not be referenced
  /// during this time.
  void deregister_source(int id);

  void set_rate(int id,
                float new_rate);
                
  /// Updates the position of the source and emits particles between 
  /// the new and old position at the already-specified rate, with
  /// the pos/vel parameters
  void update_source(int id,
                     float dt,
                     const Position & source_pos,
                     const Velocity & source_vel,
		     float vel_jitter_mag = 2.0f);

  // functions called by the core part of SSS
  void draw_particles(const Object * eye);
  
  /// Does 2 things:
  /// 1. Moves the particles by dt
  /// 2. Removes any expired particles
  /// 3. Removes any dead sources
  void move_particles(float dt);

private:

  Particle_engine();
  ~Particle_engine();

  static Particle_engine * m_instance;

  typedef std::map<int, Particle_source *> Particle_source_map;
  Particle_source_map particle_sources;

  std::vector<int> free_ids;
};

inline Particle_engine * Particle_engine::instance()
{
  return (m_instance ? m_instance : (m_instance = new Particle_engine));
}

#endif
