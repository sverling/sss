/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_PHYSICS_H
#define SSS_PHYSICS_H

#include "types.h"

#include <list>
using namespace std;

class Object;

//! Deals with the general physics integration
/*!  Note - does not calculate object-specific forces, such as
  aerodynamic forces, or even interactions between objects and the
  ground (maybe it should do the latter). It is needed because
  calculations of the interactions between objects should be done in
  an object independant way.  */
class Physics
{
public:
  static inline Physics * instance();
  ~Physics();
  
  //! Does the timestep for all registered objects
  void do_timestep(float dt0);

  //! Does an individual timestep for one object, including object
  //! specific forces and collisions. Splits dt if necessary
  void do_timestep(Object * object, float dt0);

  void register_object(Object * object);
  int deregister_object(const Object * object);
  
  // this shouldn't really be necessary...
  const list<Object *> & get_objects() {return m_objects;}

private:
  Physics();

  // the collision force is added to force/moment
  void calc_collision_force(const Object * obja,
			    Vector & force,
			    Vector & moment);

  //! as do_timestep, but doesn;t split dt
  void real_do_timestep(Object * object, float dt);

  static Physics * m_instance;
  
  list<Object *> m_objects;
  typedef list<Object *>::iterator Objects_it;
};

Physics * Physics::instance()
{
  if (m_instance == 0)
    m_instance = new Physics();
  return m_instance;
}


#endif
