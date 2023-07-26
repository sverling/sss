/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_STRUCTURE_COMPONENT_H
#define GLIDER_STRUCTURE_COMPONENT_H

#include "glider_structure.h"

#include "types.h"

#include <vector>

class Glider;
class Config_file;
class Aerofoil;
class Fuselage;
class Ski;
class Propeller;

//! Concrete class for representing the structure of a glider
/*! 

  Uses the aerodynamic component structure (+ a "fuselage") to
  determine where the hard points of the structure are.

*/
class Glider_structure_component : public Glider_structure
{
public:
  Glider_structure_component(Config_file & structure_config, Glider & glider);

  ~Glider_structure_component();
  
  //! Return the net force and moment in world coordinates
  void get_force_and_moment(Vector3 & force,
                            Vector3 & moment);

  /// Return the angular momentum associated with "static" objects
  /// such as propellers.
  void get_extra_ang_mom(const Joystick & joystick, Vector3 & ang_mom) const;

  //! maximum distance of any point from the glider origin
  float get_bounding_radius() const {return max_dist;} 

  float get_bounding_sphere_scale() const {return bounding_sphere_scale;}

  bool is_on_ground() const {return on_ground;}

  void update_structure(const Joystick & joystick);

  virtual void show();
private:
  //! Helper fn to calculate max_dist below
  void calculate_max_dist();
  /*! maximum distance of any structural point - used for ground
      collision detection. Updated at end of ctor, after all
      aerofoils/fuslages have been added. */
  float max_dist; 

  float bounding_sphere_scale;

  bool on_ground;

  /// m_hard_points contains pointers to all the hard points in the
  /// components (easy to iterate over)
  std::vector<const Hard_point *> m_hard_points;
  std::vector<Aerofoil *> m_aerofoils;
  std::vector<Fuselage *> m_fuselages;
  std::vector<Ski *> m_skis;
  std::vector<Propeller *> m_propellers;
  
  Glider & m_glider;
};


#endif
