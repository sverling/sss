/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_STRUCTURE_CRRCSIM_H
#define GLIDER_STRUCTURE_CRRCSIM_H

#include "glider_structure.h"

#include "types.h"
#include <vector>
using namespace std;

class Glider;
class Config_file;

/*! 
  Concrete class for representing the structure of a glider
*/
class Glider_structure_crrcsim : public Glider_structure
{
public:
  Glider_structure_crrcsim(Config_file & structure_config, Glider & glider);
  
  //! Return the net force and moment in world coordinates
  void get_force_and_moment(Vector3 & force,
                            Vector3 & moment);

  //! maximum distance of any point from the glider origin
  float get_bounding_radius() const {return max_dist;} 

  float get_bounding_sphere_scale() const {return bounding_sphere_scale;}

  bool is_on_ground() const {return on_ground;}

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

  vector<Position> m_hard_points;
  Glider & m_glider;
};


#endif
