/*
  Sss - a slope soaring simulater.
  Copyright (C) 2003 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef RACE_MANAGER_H
#define RACE_MANAGER_H

#include "object.h"
#include "text_overlay.h"
#include "sss_glut.h"

#include <list>
#include <vector>

enum Race_type
{
  NORMAL_RACE,
  F3F_RACE,
  TIMED_RACE
};

enum Race_status
{
  RACE_WAITING_FOR_RACE,
  RACE_COUNTDOWN,
  RACE_RACING,
  RACE_FINISHED_RACE
};
  
enum Record_status
{
  RECORD_WAITING_FOR_RACE,
  RECORD_READY_FOR_RACE, // glider is in position (in checkpoint 0 for f3f)
  RECORD_RACING,
  RECORD_FINISHED_RACE
};

/// Base class for checkpoints.
class Checkpoint
{
public:
  virtual ~Checkpoint() {};

  virtual bool reached_checkpoint(const Position & pos) const = 0;

  /// Returns the vector to reach the closest point that would meet
  /// the checkpoint
  virtual const Position & get_target() const = 0;

  virtual void draw() = 0;
};

class Checkpoint_sphere : public Checkpoint
{
public:
  Checkpoint_sphere(const Position & pos, float radius) : pos(pos), radius(radius) {};
  Position pos;
  float radius;
  bool reached_checkpoint(const Position & pos) const;
  const Position & get_target() const {return pos;}
  void draw();
};

class Checkpoint_cylinder : public Checkpoint
{
public:
  Checkpoint_cylinder(float x, float y, float radius) : x(x), y(y), radius(radius), target(x, y, 80) {};
  float x, y, radius;
  Position target;
  bool reached_checkpoint(const Position & pos) const;
  const Position & get_target() const {return target;}
  void draw();
};

class Checkpoint_plane : public Checkpoint
{
public:
  // This is a vertical plane. It is defined by the locations of the
  // two poles
  Checkpoint_plane(float x0, float y0, float x1, float y1);
  float x0, y0, x1, y1;
  // Also store the plane in the for Ax + By + C = 0
  float A, B, C;
  Position target;
  bool reached_checkpoint(const Position & pos) const;
  const Position & get_target() const {return target;}
  void draw();
};

class Checkpoint_gate : public Checkpoint
{
public:
  // This is a vertical "gate". It is defined by the locations of the
  // two poles and the plane between them
  Checkpoint_gate(float x0, float y0, float x1, float y1);
  float x0, y0, z0, x1, y1, z1;
  float max_z;
  // Also store the plane in the for Ax + By + C = 0
  float A, B, C;
  Position target;
  bool reached_checkpoint(const Position & pos) const;
  const Position & get_target() const {return target;}
  void draw();
  GLuint list_num;
};

/// Keeps track of any objects involved in an racing competition
/// (there may be different race modes). Every frame it looks at all
/// the registered objects and works out how far they've got through
/// the competition by seeing if they've reached their destination. A
/// destination could consist of a sphere (where the object must get
/// inside the sphere) or a plane, where the object needs to get onto
/// a particular side of the plane.
///
/// It also draws anything associated with the race such as the poles,
/// and does audio.
///
/// We inherit from Object just so we can register with other
/// things...
class Race_manager : public Object
{
public:
  /// returns non-zero if a race manager exists
  static inline Race_manager * get_instance() {return s_instance;}

  Race_type get_race_type() const {return m_race_type;}

  /// Create the Race_manager.
  static Race_manager * create_instance(
    Race_type type, 
    const std::vector<Checkpoint *> & checkpoints);
  
  /// Allow addition of checkpoints after creation (they may be
  /// calculated on the fly after the terrain is generated) or before
  /// it. I.e. if checkpoints are added before creation then they will
  /// be used when the race is eventually created.  If called more
  /// than once then subsequent checkpoints get added to the end.
  static void add_checkpoints(
    const std::vector<Checkpoint *> & checkpoints);

  void register_object(Object * object);
  void deregister_object(const Object * object);
  void reset_object(const Object * object);

  const Checkpoint * get_checkpoint(const Object * object);
  Record_status get_status(const Object * object);

  void draw(Draw_type draw_type);

  /// use this to update ourselves
  void post_physics(float dt);

  bool use_physics() const {return false;}
  float get_graphical_bounding_radius() const {return 0;}
  float get_structural_bounding_radius() const {return 0;}
  
private:
  Race_manager(Race_type type, 
               const std::vector<Checkpoint *> & checkpoints);
  ~Race_manager();
  
  void do_waiting_for_race();
  void do_countdown();
  void do_racing();
  void do_finished_race();

  void do_glider_text();

  static Race_manager * s_instance;
  
  struct Record
  {
    Object * object;
    // stage is such that the object destination is
    // m_destinations[stage] (unless stage is out of rage)
    int stage; 
    Record_status status;
    std::vector<float> leg_times;
    // penalty times incurred on the way through the leg
    std::vector<float> leg_penalty_times; 
    float time_at_start_of_leg;
    float total_time;
    float total_penalty_time;
    float time_for_position; // smaller is better!
  };

  Race_type m_race_type;

  Race_status m_race_status;
  // time since the last status change
  float m_timer;

  // best ever time
  float m_best_time;

  std::list<Record> m_objects;
  typedef std::list<Record>::iterator Objects_it;

  std::vector<Checkpoint *> m_checkpoints;
  typedef std::vector<Checkpoint *>::iterator Checkpoint_it;

  // Assuming that repeated checkpoints are actually the same object, 
  // then when there are multiple legs there will be duplication - we 
  // don't want to draw the same checkpoint again.
  std::vector<Checkpoint *> m_checkpoints_to_draw;

  Text_overlay m_text_overlay;

  /// Checkpoints added before creation.
  static std::vector<Checkpoint *> s_pending_checkpoints;
};

#endif





