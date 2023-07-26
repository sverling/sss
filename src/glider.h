/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_H
#define GLIDER_H

#include "object.h"

#include <string>

class Glider_aero;
class Glider_structure;
class Glider_graphics;
class Glider_power;
class Joystick;
class Config_file;
class Body;
class Rgba_file_texture;
class Vario;

//! The glider
/*!

  The Glider class hands off dealing with aerodynamics, collision
  response and rendering to components, but integrates the results.

  The components can be different types - i.e. the aerodynamics mcan
  use the component model, whilst rendering can be done (in principle)
  using a completely independant model.

 */
class Glider : public Object
{
public:
  Glider(Config_file & config_file,
         Position p,
         float speed,
         Joystick * joystick,
         bool local = true);
  ~Glider();

  //! Can be called at any time.
  void initialise_from_config(Config_file & config_file,
                              bool send_config_file_update = true);
  
  //! Returns the position of the eye
  Position get_eye() const;

  //! Returns the position of what the eye looks at
  Position get_eye_target() const;

  //! When hold, up is up
  Vector get_eye_up() const;

  //! We can have multiple eye positions, so we are interested in
  //! whether we are the eye.
  void set_eye(bool is_eye);
  
  //! offset the eye offset/direction by "some angle" - x and y range from -1 to 1
  void set_eye_offset(const Vector3 &offset) {m_desired_eye_offset = offset;}

  //! Sets the glider altitude relative to the terrain
  void set_altitude(float alt); 
  
  //! The way that the glider moves is object-dependant
  void calc_new_pos_and_orient(float dt);
  //! The glider has aerodynamics and ground collision
  void calc_object_specific_force_and_moment(Vector & force, 
                                             Vector & moment);
  //! The way that the glider accelerates is object-dependant
  void calc_new_vel_and_rot(float dt,
                            const Vector & force,
                            const Vector & moment);

  void post_physics(float dt);

  //! deal with any pending remote updates here
  void pre_physics(float dt);

  //! Draws the glider
  void draw(Draw_type draw_type);

  //! resets everything to starting parameters. if body is non-zero
  //then the starting parameters are recalculated.
  void reset(const Body * body = 0); 

  //! Returns the glider mass
  float get_mass() const {return mass;}

  //! Returns the glider's airspeed
  Velocity get_airspeed() const;

  //! Returns the joystick - currently used by the components to get
  //! the values, but could also be used by a "robot" pilot to set the
  //! joystick values.
  Joystick & get_joystick() {return *joystick;}

  /// Returns the "real" joystick - taking into account the servo speed
  const Joystick & get_real_joystick() {return *m_real_joystick;}

  /// Allows a joystick to be substituted (temporarily) for the
  /// original one.  If control has already been taken, the new
  /// joystick replaces the old one.
  /// If 0 is passed in we revert to the original joystick
  void take_control(Joystick * new_joystick);
  
  //! maximum distance of any graphics point from the glider origin -
  //! used for shadow calculation
  float get_graphical_bounding_radius() const;

  //! maximum distance of any structure point from the glider origin -
  //! could be used for collision detection etc
  float get_structural_bounding_radius() const;

  float get_collision_sphere_radius() const;

  // indicates if we're touching the ground
  bool is_on_ground() const;

  //! pauses just this glider
	virtual void toggle_paused();

  //! Resets the gliders race status (if there is a race)
  void reset_in_race();

  //! Handle a missile hit
  void handle_missile_hit(float missile_mass,
                          const Position & missile_pos,
                          const Velocity & missile_vel);

  //! Respond to a missile hit
  void respond_to_missile_hit(float missile_mass,
                              const Position & missile_pos,
                              const Velocity & missile_vel);

  //! returns the mass for collision response
  Mass_type get_mass(float & _mass) const {
    _mass = mass ;
    return (m_paused ? MASS_TRANSPARENT : MASS);}

  virtual void send_remote_update();
  virtual void recv_remote_update(Remote_sss_msg & msg, float dt);

  //! Dumps all available info
  void show();

  //! crrcsim stuff reads these from the aero file...
  void set_physics_params(float _mass, float _I_x, float _I_y, float _I_z)
    { mass = _mass ; I_x = _I_x ; I_y = _I_y ; I_z = _I_z;}

  // Gets the glider variometer reference
  Vario & get_vario() const { return * vario; }
private:
  void draw_cockpit();
  void draw_cross_hair() const;

  float mass;
  float I_x, I_y, I_z; //!< moments of inertia.

  Matrix I_inv_body; //!< inverse inertia tensor in body coords

  //! deals with the aerodynamic properties
  Glider_aero      * aero;
  //! deals with the collision detection etc
  Glider_structure * structure;    
  //! responsible for displaying the model
  Glider_graphics  * graphics; 
  //! deals with the "power"
  Glider_power     * power;

  /// joystick passed to us
  Joystick * joystick;
  /// represent the finite speed of sevos by having a joystick that 
  /// is always catching up with the original. This catch up is only done
  /// on the first 4 channels (say - analogue channels).
  Joystick * m_real_joystick;

  /// The original joystick may be swapped out... but we always
  /// maintain a pointer to the original so that we can go back to it
  /// (see take_control).
  Joystick * m_original_joystick;

  Position m_orig_pos;
  float m_orig_speed;
  
  bool m_paused;
  bool m_eye; // are we the eye?
  // Note that we always reach HOLD via CHASE. HOLD is where the eye position is fixed
  enum Eye_type {PILOT, CHASE, CHASE_FAR, HOLD} m_eye_type; 
  Position chase_pos; // position of the eye when chasing
  bool m_local;
  bool do_reset;

	Vector m_eye_target;
	Vector m_eye_target_rate;

  Vector3 m_eye_offset;
  Vector3 m_desired_eye_offset;
  Vector3 m_eye_offset_rate;

  Rgba_file_texture *m_cockpit_texture;

  /// whenever we get an update message just store it and process it
  /// in pre_physics. Then we don't waste time processing out-of-date
  /// updates. Need to make sure that if we get a reset we wipe it.
  Remote_sss_msg * m_last_update_msg;
  float m_last_update_dt;

  struct Glider_audio
  {
    Glider_audio() : audio_file("glider.wav"), 
      vol_offset(0.0f), pitch_offset(1.0f), pitch_scale_per_vel(1.0f),
      vol_scale_per_vel(1.0f), inside_vol_scale(0.1f) {}
    std::string audio_file;
    float vol_offset;
    float pitch_offset;
    float pitch_scale_per_vel;
    float vol_scale_per_vel;
    float inside_vol_scale; // scale vol by this when inside
  };
  Glider_audio audio;

  Vario * vario;
};

#endif
