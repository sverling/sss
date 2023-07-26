#ifndef GRAGPHICS_3DS
#define GRAGPHICS_3DS

#include "glider_graphics.h"
#include "my_glut.h"

class Glider;
class CLoad3DS;
struct t3DModel;


class Graphics_3ds
{
public:
  Graphics_3ds(const char * strFilename_3ds);

  ~Graphics_3ds();

  enum Draw_type { NORMAL, SHADOW };
  void draw_thing(Draw_type draw_type);
  
  void draw(Draw_type draw_type);
  
  void show();
private:
  //! Private helper to set up the display list
  void draw_glider(Draw_type draw_type);
  float calc_bounding_radius() const;

  float max_dist; 
  
  CLoad3DS * load3ds;	//!< This is 3DS class.  This should go in a good model class.
  t3DModel * model;	//!< This holds the 3D Model info that we load in

  GLenum current_shade_model;
  GLuint list_num;
  
  bool cull_backface;
};

#endif
