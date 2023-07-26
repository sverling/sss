/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  Renderer deals with rendering details, and window management
*/

#ifndef SSS_RENDERER_H
#define SSS_RENDERER_H

#include "sss_glut.h"
#include <vector>
using namespace std;

class Object;
class Text_overlay;
class Environment;

//! Responsible for displaying all the objects
/*!

  There is only one display context (at the moment) so the renderer is
  a singleton.

*/

class Renderer
{
public:

  static inline Renderer * instance();

  /// There are certain objects that always get rendered
  /// We allow the caller to ask that all non-text things
  /// get dimmed. this is to allow text to show up better 
  /// when there's lots of it
  void render_objects(const vector<Object *> & object_list,
                      const Object * eye,
                      Environment * environment,
                      Object * shadow_maker,
                      bool dim_non_text = false);

  void reshape(int w, int h); //!< Called by Sss
  
  /*! We store a list of text overlays - different components can have
    their own overlays. They (currently) get reset after each drawing. */
  void add_text_overlay(Text_overlay * overlay);
  int remove_text_overlay(Text_overlay * overlay);

  //! Set the field of view
  void set_fov(float fov);
  void set_clip_near(float clip_near);
  void set_clip_far(float clip_far);
  //! indicate the amount of dynamic zoom
  void set_fov_zoom(float zoom);
  
  void recalculate_quadric();

  /// writes the next rendered frame to a ppm image
  void do_screenshot() {m_do_screenshot = true;}

  /// sets up model/projection matrices, ready for drawing
  void setup_view_matrices(const Object * eye);

  //! Returns the quadric object
  GLUquadricObj * quadric() {return m_quadric;}
private:

  Renderer();
  
  void initialise_gl();
  void setup_projection();
  /// apply a white layer with alpha
  void apply_whitewash(float alpha);
  static Renderer * m_instance;

  GLUquadricObj * m_quadric;

  vector<Text_overlay *> m_text_overlay_list;

  float m_fov_zoom;

  bool m_do_screenshot;
};

inline Renderer * Renderer::instance()
{
  if (0 == m_instance)
  {
    m_instance = new Renderer();
  }
  return m_instance;
}


#endif

