/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_GRAPHICS_COMPONENT_H
#define GLIDER_GRAPHICS_COMPONENT_H

#include "glider_graphics.h"

#include "fuselage.h"
#include "aerofoil.h"
#include "propeller.h"

#include <vector>

class Glider;

//! Concrete class for displaying a glider
/*!

  Uses the aerodynamic component structure for the display.

*/
class Glider_graphics_component : public Glider_graphics
{
public:
  Glider_graphics_component(Config_file graphics_config, Glider & glider);
  
  void draw(Draw_type draw_type);

  //! maximum distance of any drawn point from the glider origin
  float get_bounding_radius() const {return max_dist;} 

  virtual void show();
private:
  //! Helper fn to calculate max_dist below
  void calculate_max_dist();
  /*! maximum distance of any graphical point - used for shadow
      calculation. Updated at end of ctor, after all
      aerofoils/fuslages have been added. */
  float max_dist; 

  //! Private helper to set up the display list
  void draw_glider(Draw_type draw_type);

  std::vector<Fuselage> m_fuselages;
  std::vector<Aerofoil> m_aerofoils;
  std::vector<Propeller> m_propellers;
  Glider & m_glider;

  GLenum current_shade_model;
  GLuint list_num;
   
};


#endif
