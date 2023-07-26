/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_STRUCTURE_3DS_H
#define GLIDER_STRUCTURE_3DS_H

#include "glider_structure.h"
#include "types.h"
#include <vector>

class Glider;
class Config_file;
class Ski;
class CLoad3DS;
struct t3DModel;

//! Concrete class for representing the structure of a glider
/*!
  
  Uses the 3D Studio Max (3ds) export format structure to get the points.
  
  \todo This is far too slow - need to get rid of redundant points
*/
class Glider_structure_3ds : public Glider_structure
{
public:
  Glider_structure_3ds(Config_file structure_config, Glider & glider);
  ~Glider_structure_3ds();
  
  
  //! Return the net force and moment in world coordinates
  void get_force_and_moment(Vector3 & force,
                            Vector3 & moment);

  //! maximum distance of any point from the glider origin
  float get_bounding_radius() const {return max_dist;} 

  float get_bounding_sphere_scale() const {return bounding_sphere_scale;}

  bool is_on_ground() const {return on_ground;}

  void update_structure(const Joystick & joystick);

  virtual void show();
private:
  //! Helper fn to calculate max_dist below
  float calc_max_dist_and_hard_points();

  /*! maximum distance of any structural point - used for ground
      collision detection. Updated at end of ctor. */
  float max_dist; 

  float bounding_sphere_scale;

  bool on_ground;

  /// all the hard points.
  std::vector<const Hard_point *> m_hard_points;
  /// The hard points from the 3ds file
  std::vector<Hard_point> m_hard_points_3ds;
  std::vector<Ski *> m_skis;
  Glider & m_glider;
  
  CLoad3DS * load3ds;	//!< This is 3DS class.  This should go in a good model class.
  t3DModel * model;	//!< This holds the 3D Model info that we load in
};


#endif
