/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file robot_pilot.cpp
*/
#include "robot_pilot.h"
#include "body.h"
#include "config_file.h"
#include "config.h"
#include "environment.h"
#include "sss.h"
#include "misc.h"
#include "renderer.h"
#include "glider.h"
#include "joystick.h"
#include "pilot_manager.h"
#include "misc.h"
#include "race_manager.h"

#include <string>
using namespace std;

/*!  Constructor needs to create a glider and a joystick.  If it turns
  out that more than one AI type is needed, the AI can be taken out of
  this class and put into a Brain class, etc. */
Robot_pilot::Robot_pilot(Config_file & robot_config,
                         Position pos,
                         float offset,
                         bool pause_on_startup)
  :
  m_tactic(CRUISE)
{
  Renderer::instance()->add_text_overlay(&m_text_overlay);
  
  m_joystick = new Joystick(10);
  
  // we assume (for now) that the strategy will be to attempt to stay
  // in a patch of lift directly ahead of the start location at the
  // time of our creation. Of course this could go horribly wrong!
  
  float speed = 10; // hard-code for the moment
  
//   float x = dot(Vector(1, 0, 0), dir);
//   float y = dot(Vector(0, 1, 0), dir);
  
//   float orient = atan2_deg(y, x);
  
  // need to parse the robot config to get the glider config file
  // if there are multiple glider files specified, cycle through them (for now)
  vector<string> possible_glider_files;
  robot_config.get_values("glider_file", possible_glider_files);
  static int glider_count = -1;
  unsigned possible_count = possible_glider_files.size();
  assert2(possible_count > 0, "Need at least one robot glider file");
  glider_count = (glider_count + 1 ) % possible_count;
  int index = glider_count;
  string glider_file = "gliders/" + possible_glider_files[index];
  bool success;
  Config_file glider_config_file(glider_file,
                                 success);
  if (false == success)
  {
    TRACE("Unable to open glider file %s for robot\n", glider_file.c_str());
    assert1(!"Error!");
  }
  
  TRACE_FILE_IF(2)
    TRACE("Robot pilot using %s\n", glider_file.c_str());;
  
  Vector dir = -Environment::instance()->get_ambient_wind(pos);
  dir[2] = 0;
  if (dir.mag2() > 0.0001f)
    dir.normalise();
  else
    dir = Vector(-1, 0, 0);

  pos[2] = 4;
  pos += 2*dir;
  
  Vector dir_perp = cross(Vector(0, 0, 1), dir);
  dir_perp.normalise();
  m_glider = new Glider(glider_config_file, 
                        pos + offset*dir_perp,
                        speed, 
                        m_joystick);
  if (pause_on_startup)
  {
    // pause the glider - after a bit we will wake it up
    m_doing_initial_wait = true;
    m_glider->toggle_paused();
    m_initial_pause_time = ranged_random(0, 5);
  }
  else
    m_doing_initial_wait = false;
  
  Sss::instance()->add_object(m_glider);
  Pilot_manager::instance()->register_pilot(this);
  
  // Set up the path-planning properties
  m_start_offset = offset;
  m_focal_dist = 25;
  m_speed_min = 10;
  m_speed_ideal = 15;
  m_focal_point_par_range = 50;
  m_focal_point_perp_range = 10;
  m_focal_point_height_range = 10;
  m_target_range = 15;
  m_max_bank_angle = 45;
  m_max_pitch_angle = 30;
  m_max_pitch_amount = 20;
  m_elev_aileron_frac = 0.4f;
  m_bank_angle_for_max_elev = 70;
  m_pitch_per_height_offset = 0.2f;
  m_chase_time = 60;
  m_cruise_time = 60;
  m_elapsed_time = 0;
  m_reset_timer = 0;
  m_missile_recharge_time = 10;
  m_missile_trigger = 5;
  
  robot_config.get_value("focal_dist", m_focal_dist);
  robot_config.get_value("focal_point_par_range", m_focal_point_par_range);
  robot_config.get_value("focal_point_perp_range", m_focal_point_perp_range);
  robot_config.get_value("focal_point_height_range", m_focal_point_height_range);
  robot_config.get_value("speed_min", m_speed_min);
  robot_config.get_value("speed_ideal", m_speed_ideal);
  robot_config.get_value("target_range", m_target_range);
  robot_config.get_value("max_bank_angle", m_max_bank_angle);
  robot_config.get_value("max_pitch_angle", m_max_pitch_angle);
  robot_config.get_value("max_pitch_amount", m_max_pitch_amount);
  robot_config.get_value("elev_aileron_frac", m_elev_aileron_frac);
  robot_config.get_value("bank_angle_for_max_elev", m_bank_angle_for_max_elev);
  robot_config.get_value("pitch_per_height_offset", m_pitch_per_height_offset);
  robot_config.get_value("chase_time", m_chase_time);
  robot_config.get_value("cruise_time", m_cruise_time);
  robot_config.get_value("missile_recharge_time", m_missile_recharge_time);
  robot_config.get_value("missile_trigger", m_missile_trigger);
  
  m_missile_timer = m_missile_recharge_time;

  reset(m_glider);
}

void Robot_pilot::reset(const Object * obj)
{
  TRACE_FILE_IF(3)
    TRACE("Recalculating focal point based on %p", obj);
  
  Vector dir = -Environment::instance()->get_ambient_wind(obj->get_pos());
  dir[2] = 0;
  if (dir.mag2() > 0.0001f)
    dir.normalise();
  else
    dir = Vector(-1, 0, 0);
  
  Vector dir_perp = cross(Vector(0, 0, 1), dir);
  dir_perp.normalise();

  // remove the start offset
  m_focal_point = 
    obj->get_pos() + 
    m_focal_dist * dir - 
    m_start_offset * dir_perp; 

  m_focal_par_dir = cross(dir, Vector(0, 0, 1)).normalise();
  m_focal_perp_dir = dir.normalise();
  
  choose_target_point();
  
}


Robot_pilot::~Robot_pilot()
{
  Renderer::instance()->remove_text_overlay(&m_text_overlay);
  Pilot_manager::instance()->deregister_pilot(this);
  Sss::instance()->remove_object(m_glider);
  
  if (m_glider)
    delete m_glider;
  
  if (m_joystick)
    delete m_joystick;
}

void Robot_pilot::choose_target_point()
{
  m_target = m_focal_point + 
    ranged_random(-m_focal_point_par_range, m_focal_point_par_range) * 
    m_focal_par_dir + 
    ranged_random(-m_focal_point_perp_range, m_focal_point_perp_range) * 
    m_focal_perp_dir;
  
  m_target[2] += ranged_random(-m_focal_point_height_range, m_focal_point_height_range);
}

void Robot_pilot::try_missile(const Object * target)
{
  const Position & glider_pos = m_glider->get_pos();
  const Vector & glider_dir = m_glider->get_vec_i_d();
  
  const Position & target_pos = target->get_pos();
  const Velocity & target_vel = target->get_vel();
  
  float dist = (target_pos - glider_pos).mag();
  // assume some missile speed.
  const float missile_speed = 70;
  const float time = dist/missile_speed;

  Position proj_target_pos = target_pos + time * target_vel;
  // aim for above the target
  proj_target_pos[2] += 0.5 * Sss::instance()->config().gravity * time * time;

  Vector3 aim_dir = (proj_target_pos - glider_pos).normalise();
  float likely_error = (aim_dir-glider_dir).mag() * dist;
  
  float bound_rad = m_missile_trigger * target->get_structural_bounding_radius();

  if (fabs(likely_error) < fabs(bound_rad))
  {
    // go for it!
    TRACE_FILE_IF(2)
      TRACE("Robot glider %p about to fire at object %p", m_glider, target);
    m_joystick->set_value(5, 1);
    // reset the timer
    m_missile_timer = m_missile_recharge_time;
  }

#if 0    
//=============================

  const Position & glider_pos = m_glider->get_pos();
  const Vector & glider_dir = m_glider->get_vec_i_d();
  
  const Position & target_pos = target->get_pos();
  const Velocity & target_vel = target->get_vel();
  
  float dist = (target_pos - glider_pos).mag();
  // assume some missile speed.
  const float missile_speed = 100;
  const float time = dist/missile_speed;
  
  Position proj_pos = glider_pos 
    + dist * glider_dir;
  // account for gravity
  proj_pos[2] -= 0.5 * Sss::instance()->config().gravity * time * time;
  
  Position proj_target_pos = target_pos + time * target_vel;
  
  float error2 = (proj_pos - proj_target_pos).mag2();
  float bound_rad2 = m_missile_trigger * target->get_structural_bounding_radius();
  bound_rad2 *= bound_rad2;
  
  if (error2 < bound_rad2)
  {
    // go for it!
    TRACE_FILE_IF(2)
      TRACE("Robot glider %p about to fire at object %p", m_glider, target);
    m_joystick->set_value(5, 1);
    // reset the timer
    m_missile_timer = m_missile_recharge_time;
  }
#endif
}

void Robot_pilot::update(float dt)
{
  m_elapsed_time += dt;
  
  if (m_doing_initial_wait)
  {
    if (m_elapsed_time > m_initial_pause_time)
    {
      TRACE_FILE_IF(2)
        TRACE("Releasing glider %p\n", m_glider);
      m_glider->toggle_paused();
      m_doing_initial_wait = false;
      m_elapsed_time = 0;
    }
    else
      return;
  }
  
  // This is the glider we might be chasing.
  const Glider * main_glider = &(Sss::instance()->glider());
  
  // reset glider position if it's on the ground, and pretty well stationary
//  if ( (m_glider->get_pos()[2] < 
//        (1.0f + Environment::instance()->get_z(m_glider->get_pos()[0], 
//                                               m_glider->get_pos()[1]) ) ) &&
//       ( ( m_reset_timer > 20.0f ) || (m_glider->get_vel().mag2() < 16.0f) ) )
  if ( m_glider->is_on_ground() &&
       ( ( m_reset_timer > 20.0f ) || (m_glider->get_vel().mag2() < 16.0f) ) )
  {
    m_reset_timer += dt;
    if (m_reset_timer > 5.0f)
    {
      printf("Resetting glider\n");
      m_glider->reset();
      m_tactic = CRUISE;
      m_elapsed_time = 0;
      choose_target_point();
      m_missile_timer = m_missile_recharge_time;
    }
  }
  else
  {
    m_reset_timer = 0;
  }
  
  // change from cruise to race after a short interval (to let things 
  // stabilise) if there is a race
  if ( (m_tactic != RACE) &&
       (m_elapsed_time > 5.0) &&
       (Race_manager::get_instance()) && 
       (Race_manager::get_instance()->get_checkpoint(m_glider)) )
  {
    TRACE_FILE_IF(2)
      TRACE("Robot glider %p changing to RACE\n", m_glider);
    m_tactic = RACE;
  }
  
  // if there's a race and we've done it, reset ourselves ready 
  // for the next race
  if ( (Race_manager::get_instance()) &&
       (Race_manager::get_instance()->get_status(m_glider) == RECORD_FINISHED_RACE) )
  {
    TRACE_FILE_IF(2)
      TRACE("Robot glider %p finished race, so resetting race status\n", m_glider);
    Race_manager::get_instance()->reset_object(m_glider);
  }
  
  // choose tactic
  switch (m_tactic)
  {
  case CRUISE:
  {
    // If we've reached our target, select a new one 
    Vector delta = m_target - m_glider->get_pos();
    // ignore height differences
    delta[2] = 0;
    if (delta.mag() < m_target_range)
      choose_target_point();
    
    if (m_elapsed_time > m_cruise_time)
    {
      m_elapsed_time = 0;
      if (m_glider != main_glider)
      {
        m_tactic = CHASE;
        TRACE_FILE_IF(2)
          {TRACE_METHOD(); TRACE("Robot glider %p switching to CHASE\n", m_glider);}
//      cout << "Robot switching to chase mode\n";
      }
    }
    
    break;
  }
  case CHASE:
  {
    if (m_glider == main_glider)
    {
      // Can't chase ourselves, for now just use cruise, though we
      // could find another robot
      m_elapsed_time = 0;
      m_tactic = CRUISE;
      TRACE_FILE_IF(2)
        {TRACE_METHOD(); TRACE("Robot glider %p switching to CRUISE\n", m_glider);}
      break;
    }
    else
    {
      m_target = main_glider->get_pos();
      // see if we're on for firing a missile
      m_missile_timer -= dt;
      if (m_missile_timer < 0)
        try_missile(main_glider);
    }
    if (m_elapsed_time > m_chase_time)
    {
      m_elapsed_time = 0;
      m_tactic = CRUISE;
      TRACE_FILE_IF(2)
        {TRACE_METHOD(); TRACE("Robot glider %p switching to CRUISE\n", m_glider);}
    }
    break;
  }
  case RACE:
  {    
    const Checkpoint * checkpoint = Race_manager::get_instance()->get_checkpoint(m_glider);
    if (checkpoint)
    {
      const Position & new_target = checkpoint->get_target();
      if ((new_target - m_target).mag2() > 0.1)
      {
        m_target = new_target;
        TRACE_FILE_IF(2)
          TRACE("New target for glider %p is (%f, %f, %f)\n", 
                m_glider, m_target[0], m_target[1], m_target[2]);
      }
    }
    else
    {
      TRACE_FILE_IF(2)
        TRACE("No target from race - going to CRUISE\n");
      choose_target_point();
      m_elapsed_time = 0;
      m_tactic = CRUISE;
    }
    break;
  }
  default:
    assert1(!"Impossible tactic!");
  }
  
  // We want to make sure we don't aim at the ground - e.g. if the
  // target glider has crashed.
  float alt = m_target[2] - Environment::instance()->get_z(m_target[0], m_target[1]);
  if (alt < 5.0)
    m_target = m_focal_point;
  
  // To actually reach our target we will need to aim upwind. 
  
  Vector local_wind = Environment::instance()->get_non_turbulent_wind(m_glider->get_pos());
#if 0
  local_wind[2] = 0;
  Velocity glider_vel = m_glider->get_vel();
  glider_vel[2] = 0;
  float ground_speed = m_speed_ideal + fabs(dot(local_wind, m_target - m_glider->get_pos()));
  float time_to_dest = (m_target - m_glider->get_pos()).mag() / ground_speed;
  Position actual_target(m_target - local_wind * 0.25 * time_to_dest);
#else
  Position actual_target(m_target);
#endif
  // display info
  
  static char s1[100]=" ";
  static char s2[100]=" ";
  static char s3[100]=" ";
  static char s4[100]=" ";
  
  static int loop = 0;
  if ( (Sss::instance()->config().text_overlay > 
        Config::TEXT_LITTLE) &&
       (*(Pilot_manager::instance()->get_pilots().begin()) == this) )
  {
    if (loop++ == 10)
    {
      loop = 0;
      // glider speed
      Velocity glider_vel = m_glider->get_vel();
      float speed = glider_vel.mag();
      float climb_rate = glider_vel[2];
      sprintf(s1, "ground speed, climb rate = %5.1f, %5.2f",
              speed,
              climb_rate);
      
      Velocity ambient_wind = Environment::instance()->get_non_turbulent_wind(
        m_glider->get_pos());
      Velocity air_vel = (glider_vel-ambient_wind);
      
      sprintf(s2, "air speed, climb rate = %5.1f, %5.2f",
              air_vel.mag(),
              air_vel[2]);
      
      // pos
      Position pos = m_glider->get_pos();
      sprintf(s3, "x, y, z, h = %5.1f, %5.1f, %5.2f, %5.2f",
              pos[0], pos[1], pos[2],
              pos[2] - Environment::instance()->get_z(pos[0], pos[1]));
      
      // target pos
      sprintf(s4, "Target (%5.1f, %5.1f, %5.2f) (%5.1f, %5.1f, %5.2f)",
              m_target[0], m_target[1], m_target[2],
              actual_target[0], actual_target[1], actual_target[2]);
      
    }
    m_text_overlay.add_entry(55, 10, s1);
    m_text_overlay.add_entry(55, 7, s2);
    m_text_overlay.add_entry(55, 4, s3);
    m_text_overlay.add_entry(55, 1, s4);
  }
  
  // Do control
  Position glider_pos = m_glider->get_pos();
  float aileron_input = 0;
  float elevator_input = 0;
  
  // ===== work out the left/right changes required ========
#if 1
  local_wind[2] = 0;
  Velocity glider_vel = m_glider->get_vel();
  glider_vel[2] = 0;
  float wind_speed = local_wind.mag();
  float glider_speed = m_speed_ideal;
  float wind_frac = 1.0f * wind_speed/(wind_speed + glider_speed);
  float glider_frac = 1.0f - wind_frac;
#endif  
  
//  Vector glider_dir = m_glider->get_vel();
  Vector glider_dir = m_glider->get_vec_i_d();
  glider_dir[2] = 0;
  
  glider_dir.normalise();
  
  Vector req_dir = (actual_target - glider_pos);
  req_dir[2] = 0;
  req_dir.normalise();
  
#if 1
  // now find the weighted average of req_dir and "into wind"
  Vector into_wind = -local_wind;
  if (into_wind.mag2() > 0)
    into_wind.normalise();
  else
    into_wind = Vector(-1.0f, 0, 0);
  Vector new_req_dir = req_dir * glider_frac + into_wind * wind_frac;
  req_dir = new_req_dir;
  req_dir.normalise();
#endif
  float x = dot(req_dir, glider_dir);
  float y = dot(req_dir, cross(glider_dir, Vector(0, 0, -1)));
  
  float change_angle = atan2_deg(y, x);
  
  // If we need to turn more than 135 deg, always turn into wind, at least at first
  if (fabs(change_angle) > 135)
  {
    Vector req_dir = -local_wind;
    if (req_dir.mag2() > 0.0f)
      req_dir.normalise();
    else
      req_dir = Vector(-1, 0, 0);
    x = dot(req_dir, glider_dir);
    y = dot(req_dir, cross(glider_dir, Vector(0, 0, -1)));
    
    change_angle = atan2_deg(y, x);
    change_angle *= 1000;
  }
//  printf("change_angle = %f\n", change_angle);
  
  // assume that a 90deg change induces full bank. Remember that the
  // change_angle is +ve to the left, but positive bank is roll to the
  // right.
  float desired_bank_angle = -change_angle * 1.0f;
  if (desired_bank_angle > m_max_bank_angle)
    desired_bank_angle = m_max_bank_angle;
  else if (desired_bank_angle < -m_max_bank_angle)
    desired_bank_angle = -m_max_bank_angle;
  
  
  // work out current bank angle
  float current_bank_angle = asin_deg(m_glider->get_vec_j_d()[2]);
  
  // assume we want full aileron/rudder for 90 deg difference
  // we will let the joystick clip!
  aileron_input += (desired_bank_angle - current_bank_angle)/90.0f;
  
  elevator_input += fabs(aileron_input) * m_elev_aileron_frac;
  
  elevator_input += fabs(current_bank_angle) / m_bank_angle_for_max_elev;
  
  //======= Now the pitch changes ============
  
  // do this on the basis of airspeed
  Velocity ambient_wind = Environment::instance()->get_non_turbulent_wind(
    m_glider->get_pos());
  float air_speed = 
    dot(m_glider->get_vel(), m_glider->get_vec_i_d()) - 
    dot(ambient_wind, m_glider->get_vec_i_d());
//  printf("air speed = %f\n", air_speed);
  
  // If the airspeed is equal to our ideal, then we aim to be
  // horizontal. If less, then we aim to put the nose down a little.
  // the angles are constrained to +/- 20 deg
  // here, +ve pitch is nose up
  float desired_pitch_angle = (air_speed - m_speed_ideal) * 
    m_max_pitch_amount / (m_speed_ideal - m_speed_min);
  
  // adjust the desired pitch according to whether we want to go up or
  // down
  float pitch_adjust = (actual_target[2] - m_glider->get_pos()[2]) * 
    m_pitch_per_height_offset;
  if (pitch_adjust > 1)
    pitch_adjust = 1.0f;
  
  desired_pitch_angle += pitch_adjust;
  
  float current_pitch_angle = asin_deg(m_glider->get_vec_i_d()[2]);
  
  // assume we want full elevator for 45 deg difference
  elevator_input += (desired_pitch_angle - current_pitch_angle) / 
    m_max_pitch_angle;
  
  // now actually set the joystick
  m_joystick->set_value(1, aileron_input);
  m_joystick->set_value(2, elevator_input);
  m_joystick->set_value(3, -1); // throttle
  m_joystick->set_value(4, 0); // rudder
}
