/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef BODY_H
#define BODY_H

#include "object.h"

class Graphics_3ds;

//! The pilot's body
/*!

  The body draws itself to look like a body (not one with very much
  life in it) - that is interested in a particular target. It can also
  move, with the constraint that it stays on the terrain surface.

 */
class Body : public Object
{
public:
  Body(Position p, const Object * target,  bool local = true);

  Position get_eye() const;
  Position get_eye_target() const;
  Vector get_eye_up() const;

  bool use_physics() const {return false;}

  void calc_new_pos_and_orient(float dt);

  void draw(Draw_type draw_type);
  
  enum Move_dir { NORTH, SOUTH, WEST, EAST, TOWARDS, AWAY, LEFT, RIGHT };
  void move(Move_dir dir, bool start); //!< Moves the body

  float get_structural_bounding_radius() const {
    return torso_length + leg_length + foot_depth + 
      head_radius + head_radius + neck_length ;} 
  float get_graphical_bounding_radius() const {
    return get_structural_bounding_radius();}

  //! body is rock-solid
  Mass_type get_mass(float & mass) const {mass = 0 ; return MASS_SOLID;}

  //! Allow changing of the target
  void set_target(const Object * new_target);

  void set_eye(bool is_eye);

  virtual void post_physics(float dt);

private:
  const Object * m_target; // what the body looks at

  void draw_body(Draw_type draw_type);
  const float torso_length, torso_width, torso_depth;
  const float leg_length;
  const float arm_length;
  const float foot_length, foot_depth;
  const float head_radius;
  const float leg_width;
  const float arm_width;
  const float neck_length;
  const float eye_height;

  float m_move_decay_time;
  float m_max_vel;
  Vector m_accel;

	Vector m_eye_target;
	Vector m_eye_target_rate;

  bool m_local;

  Graphics_3ds * m_model_3ds; // if non-zero then this is available
  bool m_model_3ds_cull_backface;

};
#endif // file included
