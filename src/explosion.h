/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "object.h"
#include "environment.h"

/// Deals with the sound, display etc of an explosion. It does not 
/// need an owner - it makes sure that when it decays it gets deleted.
class Explosion : public Object, public Wind_modifier
{
public:
	Explosion(const Position & pos, 
            const Velocity & vel, 
            float max_size,
            bool local = true);
	virtual ~Explosion();

  void draw(Draw_type draw_type);
  
  void post_physics(float dt);

  float get_graphical_bounding_radius() const {return radius;}
  float get_structural_bounding_radius() const {return radius;}
  void get_wind(const Position & pos, Vector & wind) const;
private:
  float max_radius;
  float lifetime;
  float sec_running;
  float radius;
  bool m_local;
  /// ID for the particle engine
  int m_particle_id_flame;
  int m_particle_id_smoke;
  Velocity m_ambient_wind;
};

#endif // file included

