/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_GRAPHICS_H
#define GLIDER_GRAPHICS_H

#include <string>
using namespace std;

class Glider;

//! Base class for the graphical component of a glider
/*!

  This class is both a factory for the creation of graphics
  components, as well as a virtual base class for the actual
  component.

 */
class Glider_graphics
{
public:
  //! Creates a sub-class of this one, depending on the type specified
  //! in the config file.
  static Glider_graphics * create(const string & graphics_file,
                                  const string & graphics_type,
                                  Glider & glider);
  virtual ~Glider_graphics() {};

  enum Draw_type { NORMAL, SHADOW };
  virtual void draw(Draw_type draw_type) = 0;

  //! maximum distance of any point from the glider origin
  virtual float get_bounding_radius() const = 0;

  virtual void show() = 0;

};

#endif
