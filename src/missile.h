/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#ifndef MISSILE_H
#define MISSILE_H

#include "object.h"
#include "fuselage.h"
#include "aerofoil.h"

#include <vector>
using namespace std;

class Glider;

/// Represents a missile. Note that the missile will take care of its 
/// own destruction, which could happen at any time.
///
/// \todo implement a communication between the missile and its parent
/// so that (a) the parent knows if it hit something and (b) the missile
/// knows if it becomes orphaned.
class Missile : public Object  
{
public:
	Missile(const Position & pos, 
          const Velocity & vel, 
          const Orientation & orient, 
          const Rotation & rot,
          float length,
          float mass,
          const Object * parent,
          bool local = true);
	virtual ~Missile();
  
  void draw(Draw_type draw_type);
  
  void post_physics(float dt);
  
  //! deal with any pending remote updates here
  void pre_physics(float dt);

  virtual void send_remote_update();
  virtual void recv_remote_update(Remote_sss_msg & msg, float dt);

  float get_graphical_bounding_radius() const {return max_dist;}
  float get_structural_bounding_radius() const {return max_dist;}
  
  void calc_new_vel_and_rot(float dt,
                            const Vector & force,
                            const Vector & moment);
  void calc_new_pos_and_orient(float dt);
  
  void calc_object_specific_force_and_moment(Vector & force, 
                                             Vector & moment);
private:
  void calculate_max_dist();
  
  void get_aero_force_and_moment(Vector3 & force,
                                 Vector3 & moment);
  
  void get_engine_force_and_moment(Vector3 & force,
                                   Vector3 & moment);
  
  void get_structure_force_and_moment(Vector3 & force,
                                      Vector3 & moment);
  
  void calculate_hard_points();
  
  float m_mass;
  float m_I_x, m_I_y, m_I_z; //!< moments of inertia.
  float sec_running;
  float max_dist;
  
  vector<Aerofoil> m_aerofoils;
  vector<Fuselage> m_fuselages;
  
  vector<Position> m_hard_points;
  
  // note that there is no guarantee that this parent will still exist
  // when we come to hit something
  const Object * m_parent; 

  bool m_local;

  /// whenever we get an update message just store it and process it
  /// in pre_physics. Then we don't waste time processing out-of-date
  /// updates. Need to make sure that if we get a reset we wipe it.
  Remote_sss_msg * m_last_update_msg;
  float m_last_update_dt;

  bool m_die;

  /// ID for the particle engine
  int m_particle_id;
  Velocity m_ambient_wind;
};

#endif // file included
