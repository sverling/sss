/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef BLUDGER_H
#define BLUDGER_H

#include "object.h"
#include <list>

/// A nasty beast that chases the main glider.
class Bludger : public Object
{
public:
  Bludger(Position p,
          bool local = true);
  ~Bludger();

  void draw(Draw_type draw_type);

  void calc_new_pos_and_orient(float dt);
  void calc_object_specific_force_and_moment(Vector & force, 
                                             Vector & moment);
  //! The way that the glider accelerates is object-dependant
  void calc_new_vel_and_rot(float dt,
                            const Vector & force,
                            const Vector & moment);

  void post_physics(float dt);

  //! maximum distance of any graphics point from the glider origin -
  //! used for shadow calculation
  float get_graphical_bounding_radius() const {return m_radius;}

  //! maximum distance of any structure point from the glider origin -
  //! could be used for collision detection etc
  float get_structural_bounding_radius() const {return m_radius;}
  //! returns the mass for collision response
  Mass_type get_mass(float & _mass) const {_mass = m_mass ; return MASS;}

  void send_remote_update() {};
  void recv_remote_update(Remote_sss_msg & msg, float dt) {};

private:
  Object * choose_new_target(std::list<Object *> & objects);

  float m_mass;
  float m_radius;
  float m_time_remaining; // time left annoying this object
  bool m_local;

  const Object * m_current_object;
};

#endif
