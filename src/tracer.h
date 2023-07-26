/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  A Tracer just follows the wind field. When it becomes further from
  the eye than max_dist, it re-initialises itself in a random position
  within max_hor_dist, and max_ver_dist.

*/
#ifndef TRACER_H
#define TRACER_H

#include "object.h"
#include <list>
using namespace std;

class Tracer;
//! Controls a collection of tracers
class Tracer_collection
{
public:
  static Tracer_collection * instance() 
    { return (m_instance == 0) ? m_instance = new Tracer_collection : m_instance; }

  void add_tracer(Object * focus, int num = 1);
  void remove_tracer(int num = 1);
  void remove_all_tracers();
  ~Tracer_collection();
private:
  Tracer_collection() {};

  static Tracer_collection * m_instance;
  list<Tracer *> tracers;
};

//! Traces the wind field
/*!
  
  This simple object just follows the local wind field, and displays
  itself as a simple object.

*/

class Tracer : public Object
{
public:
  Tracer(Object * focus);
  ~Tracer();
  
  virtual void draw(Draw_type draw_type);

  bool use_physics() const {return false;}

  virtual void calc_new_pos_and_orient(float dt);

  float get_structural_bounding_radius() const {return m_radius;} 
  float get_graphical_bounding_radius() const {return m_radius;} 

private:
  void init_pos();

  const float m_radius;
  const float m_max_hor_dist;
  const float m_max_ver_dist;

  Object * m_focus;  

  bool valid;
  float elapsed_time;
  float timestep; // only update when elapsed_time > timestep
};

#endif
