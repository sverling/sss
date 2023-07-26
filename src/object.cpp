/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file object.cpp
*/

#include "object.h"
#include "log_trace.h"

#include "sss_glut.h"

using namespace std;

/*! Helper function that the derived class will call to convert into
  object space prior to drawing */
void Object::basic_draw() const
{
  // translation's easy...
  glTranslatef(pos[0], pos[1], pos[2]);
  
  float matrix[] = 
    {
      orient(0, 0), // 1st column
      orient(1, 0),
      orient(2, 0),
      0,
      orient(0, 1), // 2nd column
      orient(1, 1),
      orient(2, 1),
      0,
      orient(0, 2), // 3rd column
      orient(1, 2),
      orient(2, 2),
      0,
      0, 0, 0, 1        // 4th column
    };
  
  glMultMatrixf(&matrix[0]);
}


//! Sets the object orientation
void Object::set_orient(const Vector & vec_i, 
                        const Vector & vec_j, 
                        const Vector & vec_k)
{
  orient = Matrix3(vec_i, vec_j, vec_k);
}

void Object::show() const
{
  pos.show("Object::pos"); 
  vel.show("Object::vel"); 
  orient.show("Object::orient"); 
  rot.show("Object::rot");
}

bool Object::validate()
{
  bool rv = true;
  
  if (get_vel().sensible() == false)
  {
    TRACE("resetting velocity for %p\n", this);
    get_vel().show();
    set_vel(Velocity(0));
    rv = false;
  }

  if (get_pos().sensible() == false)
  {
    TRACE("resetting position for %p\n", this);
    get_pos().show();
    set_pos(Position(0));
    rv = false;
  }

  if (get_rot().sensible() == false)
  {
    TRACE("resetting rotation for %p\n", this);
    get_rot().show();
    set_rot(Rotation(0, 0, 0));
    rv = false;
  }

  if (get_orient().sensible() == false)
  {
    TRACE("resetting orientation for %p\n", this);
    get_orient().show();
    set_orient(Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1));
    rv = false;
  }

  return rv;
}

