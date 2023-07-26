/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_AERO_H
#define GLIDER_AERO_H

#include <string>
using namespace std;

class Vector3;
class Glider;

//! Base class for the aerodynamics component of a glider
/*!
  This class is both a factory for the creation of aerodynamic
  components, as well as a virtual base class for the actual
  component.
 */
class Glider_aero
{
public:
  //! Creates a sub-class of this one, depending on the type specified
  //! in the config file.
  static Glider_aero * create(const string & aero_file, 
                              const string & aero_type, 
                              Glider & glider);
  
  virtual ~Glider_aero() {};

  //! Return the net force and moment in world coordinates
  virtual void get_force_and_moment(Vector3 & force,
                                    Vector3 & moment) = 0;
  virtual void show() = 0;
};

#endif
