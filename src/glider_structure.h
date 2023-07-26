/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_STRUCTURE_H
#define GLIDER_STRUCTURE_H

#include <string>
using namespace std;

class Vector3;
class Glider;
class Joystick;

//! Base class for the structural component of a glider
/*!
  This class is both a factory for the creation of structure
  components (for collision response), as well as a virtual base class
  for the actual component.
 */
class Glider_structure
{
public:
  //! Creates a sub-class of this one, depending on the type specified
  //! in the config file.
  static Glider_structure * create(const string & structure_file,
                                   const string & structure_type,
                                   Glider & glider);
  
  virtual ~Glider_structure() {};

  //! Return the net force and moment in world coordinates
  virtual void get_force_and_moment(Vector3 & force,
                                    Vector3 & moment) = 0;

  /// Return the angular momentum associated with "static" objects
  /// such as propellers.
  virtual void get_extra_ang_mom(const Joystick & joystick, 
                                 Vector3 & ang_mom) const {};

  //! Return the radius of the bounding sphere
  virtual float get_bounding_radius() const = 0;

  /// How much the bs should be scaled by during object-object collisions.
  virtual float get_bounding_sphere_scale() const {return 1.0f;}

  virtual bool is_on_ground() const = 0;

  /// Allow for any moving bits to be updated
  virtual void update_structure(const Joystick &) {};

  virtual void show() = 0;
};

#endif
