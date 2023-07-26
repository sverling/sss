/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_GRAPHICS_CRRCSIM_H
#define GLIDER_GRAPHICS_CRRCSIM_H

#include "glider_graphics.h"
#include "config_file.h"

#ifdef WITH_GLE
#include "GL/gle.h"
#else
// just enough to compile
typedef double gleDouble;
typedef gleDouble gleAffine[2][3];
typedef gleDouble gleAffine[2][3];

#endif

#include "sss_glut.h"

#include <string>

class Glider;

/// Displays a glider based on Crrcsim
class Glider_graphics_crrcsim : public Glider_graphics
{
public:
  Glider_graphics_crrcsim(Config_file graphics_config,
                          Glider & glider);

  virtual void draw(Draw_type draw_type);

  //! maximum distance of any point from the glider origin
  virtual float get_bounding_radius() const {return max_dist;}

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

  Glider & m_glider;

  GLenum current_shade_model;
  GLuint list_num;

  // now stuff for the drawing

  struct Crrcsim_extrusion
  {
    std::string extrusion_name;
    gleDouble up[3];
    int num_points_on_path;
    GLfloat translate_x,translate_y,translate_z;
    GLfloat rotate_x,rotate_y,rotate_z;
    gleDouble **point_array;
    gleAffine *scaling;
    GLfloat **colors;
    int num_contour_elements;
    gleDouble **contour;
    gleDouble **contour_normal_vectors;
    Crrcsim_extrusion *next_extrusion;
  };
  struct Crrcsim_cylinder
  {
    std::string cylinder_name;
    GLfloat translate_x,translate_y,translate_z;
    GLfloat rotate_x,rotate_y,rotate_z;
    GLfloat radius_1,radius_2,length;
    int radial_sampling,longitudinal_sampling;
    GLfloat colors[4];
    Crrcsim_cylinder *next_cylinder;
  };
  struct Crrcsim_sphere 
  {
    std::string sphere_name;
    GLfloat translate_x,translate_y,translate_z;
    GLfloat rotate_x,rotate_y,rotate_z;
    GLfloat radius;
    int slices,stacks;
    GLfloat colors[4];
    Crrcsim_sphere *next_sphere;
  };
  struct Crrcsim_triangle
  {
    std::string triangle_name;
    unsigned long int num_triangles;
    GLfloat translate_x,translate_y,translate_z;
    GLfloat rotate_x,rotate_y,rotate_z;
    GLfloat colors[4];
    GLfloat **vertices;
    GLfloat **normals;
    Crrcsim_triangle *next_triangle;
  };

  Crrcsim_extrusion *extrusions;
  Crrcsim_cylinder *cylinders;
  Crrcsim_sphere *spheres;
  Crrcsim_triangle *triangles;
};


#endif
