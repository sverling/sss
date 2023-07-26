/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef THERMAL_H
#define THERMAL_H

#include "object.h"
#include "sss_glut.h"

/*!  This describes a single thermal which evolves over a certain
  period, and then dies. When it dies, it notifies the
  Thermal_manager, which may then start another one, or reset this
  one. */
class Thermal : public Object
{
public:
  Thermal(Position start_pos,
          float updraft,
          float updraft_radius,
          float zero_radius,
          float downdraft_radius,
          float outer_radius,
          float height, // height above ground
          float inflow_height,
          float lifetime,
          float lifecycle); // between 0 and 1 indicating
                            // point in lifecycle to start
  
 
  void reset(const Position & start_pos);

  void draw(Draw_type draw_type) {}
  float get_graphical_bounding_radius() const {return 0;}
  float get_structural_bounding_radius() const {return 0;}

  //! indicates if this thermal is still going
  bool get_active() const {return m_active;}

  //! indicates if the position is under our influence
  inline bool in_range(float x, float y) const {
    return ( ( (x-pos[0])*(x-pos[0]) + 
               (y-pos[1])*(y-pos[1]) ) <  m_range2 );
  }
  

  //! note that we don't need to be updated very often
  void update(float dt);

  //! Draw the central column
  void draw() const;
  
  Velocity get_wind(const Position & pos) const;
  
private:
  //! calculates the upward flux for w = Ar + b, between radii ra and rb
  inline float calc_flux(float A, float B, float ra, float rb) const;

  float m_time_elapsed;
  float m_updraft; 
  float m_updraft_radius;
  float m_zero_radius;
  float m_downdraft_radius;
  float m_outer_radius;
  float m_height;
  float m_inflow_height;
  float m_range2; // the square of the outer radius
  float m_downdraft; //!< Note that this is calculated from the above parameters
  float m_lifetime;
  float m_strength;
  bool m_active;
  
  // constants used in calculating w
  float A01, B01;
  float A12, B12;
  float A23, B23;
  float A34, B34;

  // the fluxes in each region (+ve up)
  float flux01;
  float flux12;
  float flux23;
  float flux34;

  mutable GLuint m_list_num;
};

#endif
