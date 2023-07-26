/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_GRAPHICS_3DS_H
#define GLIDER_GRAPHICS_3DS_H

#include "glider_graphics.h"
#include "config_file.h"

#include "sss_glut.h"

class Glider;
class CLoad3DS;
struct t3DModel;

//! Concrete class for displaying a glider
/*!
  
  Uses the 3D Studio Max (3ds) export format structure for the display.
  
*/
class Glider_graphics_3ds : public Glider_graphics
{
public:
  Glider_graphics_3ds(Config_file graphics_config, Glider & glider);
  ~Glider_graphics_3ds();
  
  void draw(Draw_type draw_type);
  
  //! maximum distance of any drawn point from the glider origin
  float get_bounding_radius() const {return max_dist;} 
  
  virtual void show();
private:
  /*! maximum distance of any graphical point - used for shadow
    calculation. Updated at end of ctor, after all
    aerofoils/fuslages have been added. */
  float max_dist; 
  
  //! Private helper to set up the display list
  void draw_glider(Draw_type draw_type);
  float calc_bounding_radius() const;

  Glider & m_glider;
  
  CLoad3DS * load3ds;	//!< This is 3DS class.  This should go in a good model class.
  t3DModel * model;	//!< This holds the 3D Model info that we load in

  GLenum current_shade_model;
  GLuint list_num;
  
  bool cull_backface;
};


#endif
