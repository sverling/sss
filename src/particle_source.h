/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PARTICLE_SOURCE_H
#define PARTICLE_SOURCE_H

/// This file defines the particle and its source

#include "types.h"
#include "particle_engine.h"

#include <list>

class Object;

/// A particle
class Particle
{
public:
  Particle(Particle_type type,
	   const Position & init_pos, 
           const Velocity & init_vel,
           float init_size,
           float max_size,
           float lifetime,
           float alpha_scale);
  
  void update(float dt);
  // for now we do all the calculations...
  void draw(const Object * eye, const float * rot_matrix);

  float get_time_left() const {return m_time_left;}
private:
  Particle_type m_type;
  Position m_pos;
  Velocity m_vel;
  float m_size;
  float m_init_size;
  float m_max_size;
  float m_time_left;
  float m_lifetime;
  float m_init_alpha;
  float m_rotation;
  float m_rotation_rate;
  // basic colour
  float m_colour[4];
};

/// Source of many particles
class Particle_source
{
public:

  Particle_source(Particle_type type,
                  int max_num,
                  const Position & init_pos,
                  const Velocity & init_vel,
                  float alpha_scale,
                  float init_size,
                  float max_size,
                  float lifetime,
                  float rate);
  ~Particle_source() {};

  void set_rate(float new_rate) {m_rate = new_rate;}

  void update_source(float dt,
                     const Position & new_pos,
                     const Velocity & new_vel,
		     float vel_jitter_mag);
  void draw_particles(const Object * eye);

  void move_particles(float dt);

  void disable() {m_suicidal = true;}

  bool dead() const {return m_dead;}
private:

  Particle_type m_type;
  int m_max_num; // max num particles
  int m_num;     // num in use - list::size() may be slow
  float m_rate;

  float m_init_size;
  float m_max_size;
  float m_lifetime;

  // The source position and velocity
  Position m_pos;
  Velocity m_vel;
  
  typedef std::list<Particle> Particle_list;
  Particle_list particles;

  bool m_suicidal, m_dead;

  // we can only add an integer number each frame... keep track of how
  // many we should have added.
  float m_num_to_add_remainder;

  float m_alpha_scale;
};

#endif
