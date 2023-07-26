/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_OBJECT_H
#define SSS_OBJECT_H

#include "types.h"

struct Remote_sss_msg;

//! Base class for all Sss objects
/*!

  Basically all objects in the Sss system must inherit from this
  class. It imposes interfaces for drawing and determining motion of
  the object. It also implements some useful behaviour.

 */
class Object
{
public:
  Object(Position p): pos(p), vel(0,0,0), 
    orient(1, 0, 0, 0, 1, 0, 0, 0, 1), // axis-aligned
    rot(0,0,0) {}

  virtual ~Object() {};

  enum Draw_type { NORMAL, SHADOW };
  virtual void draw(Draw_type draw_type) = 0;
  void set_pos(const Position & p) {pos = p;}
  void set_vel(const Velocity & v) {vel = v;}
  void set_orient(const Orientation & o) {orient = o;}
  void set_orient(const Vector & vec_i, 
                  const Vector & vec_j, 
                  const Vector & vec_k);
  
  void set_rot(const Rotation & r) {rot = r;}

  //! Where the eye is
  virtual Position get_eye() const {return pos;} 
  //! What the eye looks at
  virtual Position get_eye_target() const {return get_eye()+get_vec_i_d();}
  //! What direction is upward, as far as the eye is concerned
  virtual Vector get_eye_up() const {return get_vec_k_d();}
  //! Notify the object that it is/is not the eye
  virtual void set_eye(bool is_eye) {};

  const Position & get_pos() const {return pos;}
  const Velocity & get_vel() const {return vel;}
  const Orientation & get_orient() const {return orient;}
  const Rotation & get_rot() const {return rot;}
  
  //! The main physics may be quite involved - if we answer false we
  //just get calc_new_pos_and_orient called.
  virtual bool use_physics() const {return true;}

  //! calculate the new object position and orientation
  virtual void calc_new_pos_and_orient(float dt) {};
  //! calculate the object-specific forces on the object
  virtual void calc_object_specific_force_and_moment(Vector & force, 
                                                     Vector & moment) 
    {}
  //! calculate the object accelerations based on the force and moment
  //! passed in
  virtual void calc_new_vel_and_rot(float dt,
                                    const Vector & force,
                                    const Vector & moment) {};
  
  virtual float get_graphical_bounding_radius() const = 0;
  virtual float get_structural_bounding_radius() const = 0;
  /// object-object collisions done with spheres - may be different size to the actual BS
  virtual float get_collision_sphere_radius() const {return get_structural_bounding_radius();}

  //! Ensures that velocity etc are not inf/nan etc
  bool validate();

  const Vector get_vec_i_d() const {return orient.get_col(0);}
  const Vector get_vec_j_d() const {return orient.get_col(1);}
  const Vector get_vec_k_d() const {return orient.get_col(2);}
  
  //! return mass (used by physics in calculating collision response) or
  //! -1.0 indicates object is "transparent"
  //! -2.0 indicates object is rock-solid (default)
  enum Mass_type {MASS, MASS_TRANSPARENT, MASS_SOLID};
  virtual Mass_type get_mass(float & mass) const {mass = 0; return MASS_TRANSPARENT;}

  //! objects can get notified before the physics update. 
  virtual void pre_physics(float dt) {}
  //! objects can get notified after the physics update
  virtual void post_physics(float dt) {}

  //! send an application-specific update to a remote object
  virtual void send_remote_update() {}
  virtual void recv_remote_update(Remote_sss_msg & msg, float dt) {}

  virtual void show() const;
  
  void basic_draw() const; // does the basic translate and rotate
protected:

  Position pos;
  Velocity vel;
  Orientation orient;
  Rotation rot;
};

#endif // file included
