/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_POWER_H
#define GLIDER_POWER_H

#include "glider_engine.h"

#include <string>
#include <vector>
using namespace std;

class Glider;
class Vector3;
class Config_file;

//! Base class for the power component of a glider
/*!
  This class is both a factory for the creation of engine
  components, as well as a virtual base class for the actual
  component.

  It is also a concrete class for the case of a single "jet" engine
  (no torque, as may be induced by a propellor, and no interaction
  with the aerodynamics).
 */
class Glider_power
{
public:
  //! Creates a sub-class of this one, depending on the type specified
  //! in the config file.
  static Glider_power * create(const string & engine_file, 
                               const string & engine_type,
                               Glider & glider);
  
  Glider_power(Config_file & engine_config, Glider & glider);
  virtual ~Glider_power() {};

  //! Return the net force and moment in world coordinates
  virtual void get_force_and_moment(Vector3 & force,
                                    Vector3 & moment);
  virtual void post_physics(float dt);
  virtual void show();
private:
  Glider & m_glider;

  vector<Glider_engine> m_engines;
};

#endif
