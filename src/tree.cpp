#include "tree.h"
#include "log_trace.h"
#include "tree_collection.h"
#include "sss.h"
#include "physics.h"

#include <string>
using namespace std;

Tree::Tree(const Position & pos, 
           float graphical_size,
           float structural_size,
           Orientation_type orientation_type)
  :
  Object(pos),
  m_graphical_size2(graphical_size*0.5),
  m_structural_size2(structural_size*0.5),
  m_orientation_type(orientation_type)
{
  TRACE_METHOD_ONLY(2);
  Physics::instance()->register_object(this);

  // randomise the initial rotation about the vertical axis...
  float dir = ranged_random(0, 360);
  Vector vec_i = Vector(sin_deg(dir), cos_deg(dir), 0);
  Vector vec_k = Vector(0, 0, 1);
  Vector vec_j = cross(vec_k, vec_i);
  set_orient(vec_i, vec_j, vec_k);
}

Tree::~Tree()
{
  TRACE_METHOD_ONLY(2);
  Physics::instance()->deregister_object(this);
}

void Tree::prepare_for_draw_pos(const Position & eye_pos)
{
  if (m_orientation_type == VERTICAL_BILLBOARD)
  {
    float dx = eye_pos[0] - get_pos()[0];
    float dy = eye_pos[1] - get_pos()[1];
    Vector vec_j = Vector(dx, dy, 0).normalise();
    Vector vec_k = Vector(0, 0, 1);
    Vector vec_i = cross(vec_j, vec_k);
    set_orient(vec_i, vec_j, vec_k);
  }
  else if (m_orientation_type == FULL_BILLBOARD)
  {
    Vector vec_j = (eye_pos - get_pos()).normalise();
    // aim for up being up
    Vector vec_k = Vector(0, 0,1);
    Vector vec_i = cross(vec_j, vec_k).normalise();
    // recalculate up
    vec_k = cross(vec_i, vec_j).normalise();
    set_orient(vec_i, vec_j, vec_k);
  }
  basic_draw();
}

void Tree::prepare_for_draw_dir(const Vector & eye_dir)
{
  if (m_orientation_type == VERTICAL_BILLBOARD)
  {
    Vector vec_j = eye_dir;
    vec_j[2] = 0;
    vec_j.normalise();
    Vector vec_k = Vector(0, 0, 1);
    Vector vec_i = cross(vec_j, vec_k);
    set_orient(vec_i, vec_j, vec_k);
  }
  else if (m_orientation_type == FULL_BILLBOARD)
  {
    Vector vec_j = eye_dir;
    // aim for up being up
    Vector vec_k = Vector(0, 0,1);
    Vector vec_i = cross(vec_j, vec_k).normalise();
    // recalculate up
    vec_k = cross(vec_i, vec_j).normalise();
    set_orient(vec_i, vec_j, vec_k);
  }
  basic_draw();
}


