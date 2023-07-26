/*
  Sss - a slope soaring simulater.
  Copyright (C) 2003 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "race_manager.h"
#include "log_trace.h"
#include "sss_assert.h"
#include "physics.h"
#include "body.h"
#include "renderer.h"
#include "sss.h"
#include "config.h"
#include "environment.h"

#include "sss_glut.h"

#ifdef WIN32
#include <windows.h>
#include <winuser.h>
#define DO_BEEP() MessageBeep(0xFFFFFFFF)
#else
#define DO_BEEP() printf("\a");
#endif

#include <numeric>

using namespace std;

Race_manager * Race_manager::s_instance = 0;

vector<Checkpoint *> Race_manager::s_pending_checkpoints;

Race_manager * Race_manager::create_instance(
  Race_type type, 
  const std::vector<Checkpoint *> & checkpoints)
{
  assert1(s_instance == 0);
  
  return (s_instance = new Race_manager(type, checkpoints));
}

Race_manager::Race_manager(Race_type type, 
                           const vector<Checkpoint *> & checkpoints)
  :
  Object(0),
  m_race_type(type),
  m_race_status(RACE_WAITING_FOR_RACE),
  m_timer(0.0f),
  m_best_time(1.0e10),
  m_checkpoints(checkpoints),
  m_checkpoints_to_draw(checkpoints)
{
  TRACE_METHOD_ONLY(1);
  // for the post_physics kick, where we work out where everything is
  Physics::instance()->register_object(this);

  if (!s_pending_checkpoints.empty())
  {
    if (m_checkpoints.empty())
    {
      TRACE("Adding pre-calculated checkpoints\n");
      m_checkpoints = s_pending_checkpoints;
      m_checkpoints_to_draw = s_pending_checkpoints;
    }
    else 
    {
      TRACE("Error - checkpoints specified at creation and pre-creation");
    }
  }

  // For the graphics
  Sss::instance()->add_object(this);
  
  Renderer::instance()->add_text_overlay(&m_text_overlay);

  // remove duplicates from the checkpoints_to_draw
  sort(m_checkpoints_to_draw.begin(), m_checkpoints_to_draw.end());
  m_checkpoints_to_draw.erase(unique(m_checkpoints_to_draw.begin(),
                                     m_checkpoints_to_draw.end()),
                              m_checkpoints_to_draw.end());
}

Race_manager::~Race_manager()
{
  TRACE_METHOD_ONLY(1);
  Physics::instance()->deregister_object(this);
  Sss::instance()->remove_object(this);

  Renderer::instance()->remove_text_overlay(&m_text_overlay);
}

const Checkpoint * Race_manager::get_checkpoint(const Object * object)
{
  Objects_it it;
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->object == object)
    {
      if ( (it->stage >= 0) && (it->stage < (int) m_checkpoints.size()) )
        return m_checkpoints[it->stage];
      else
        return 0;
    }
  }
  TRACE("Cannot find object %p\n", object);
  return 0;
}

Record_status Race_manager::get_status(const Object * object)
{
  Objects_it it;
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->object == object)
    {
      return it->status;
    }
  }
  TRACE("Cannot find object %p\n", object);
  assert1(!"Error");
  return RECORD_WAITING_FOR_RACE;
}

string get_quote(int pos)
{
  static vector<string> quotes_1;
  static vector<string> quotes_2;
  static vector<string> quotes_other;
  static bool init = false;
  if (init == false)
  {
    init = true;
    quotes_1.push_back("Did you cheat?");
    quotes_1.push_back("Faster than a greased whippet.");
    quotes_1.push_back("Speed isn't everything, you know.");
    quotes_1.push_back("My pet rat is quite quick, too.");
    quotes_1.push_back("Now try doing it inverted.");
    quotes_1.push_back("What's the hurry?");
    quotes_1.push_back("Well done, sausage.");
    quotes_2.push_back("Where were you?");
    quotes_2.push_back("You've got to try harder...");
    quotes_2.push_back("You should take flying lessons from an ostrich.");
    quotes_2.push_back("Better luck next time.");
    quotes_2.push_back("Your AI is nearly as good as mine.");
    quotes_2.push_back("Hmmm, I see I've got competition.");
    quotes_other.push_back("Have you worked out which way it goes yet?");
    quotes_other.push_back("Go on, try again.");
    quotes_other.push_back("You could always try flapping next time.");
    quotes_other.push_back("Tell me this is your first go...");
    quotes_other.push_back("Do you breed snails for a living?");
    quotes_other.push_back("If slugs had wings...");
  }
  // @todo fn to be finished later...
  return string("");
}

void Race_manager::do_glider_text()
{
  // find the Record corresponding to the main glider... actually just
  // assume it is the first one!
  assert1((int) m_objects.size() > 0);
  Objects_it rec = m_objects.begin();
  
  int x = 70;
  int y = 95;
  int dy = 3;
  char text[128];
   
  switch (rec->status)
  {
  case RECORD_WAITING_FOR_RACE:
    m_text_overlay.add_entry(x, y, "Waiting for race");
    break;
  case RECORD_READY_FOR_RACE:
    m_text_overlay.add_entry(x, y, "Ready for race");
    break;
  case RECORD_RACING:
    m_text_overlay.add_entry(x, y, "Racing");
    break;
  case RECORD_FINISHED_RACE:
    m_text_overlay.add_entry(x, y, "Finished race");
    break;
  }

  unsigned int i;
  y -= dy;
  if (rec->status == RECORD_FINISHED_RACE)
  {
    sprintf(text, "Total time: %7.2f + %7.2f", 
            rec->total_time, 
            rec->total_penalty_time);    
  }
  else
    sprintf(text, "Aiming for checkpoint %d", rec->stage);
  m_text_overlay.add_entry(x, y, text);
  
  for (i = 0 ; i < rec->leg_times.size() ; ++i)
  {
    y -= dy;
    sprintf(text, "Stage %2d, time %6.2f (%3.0f)",
            i,
            rec->leg_times[i],
            rec->leg_penalty_times[i]);
    m_text_overlay.add_entry(x, y, text);
  }

  if (rec->status == RECORD_FINISHED_RACE)
  {
    // work out the total times for the others to get the position
    int pos = 1;
    Objects_it pos_it = m_objects.begin();
    ++pos_it;
    for ( ;
          pos_it != m_objects.end() ;
          ++pos_it)
    {
      if ( (pos_it->total_time + pos_it->total_penalty_time) < 
           (rec->total_time + rec->total_penalty_time) )
        ++pos;
    }
    float this_best_time = 1.0e10;
    for ( pos_it = m_objects.begin() ;
          pos_it != m_objects.end() ;
          ++pos_it)
    {
      if ( (pos_it->total_time + pos_it->total_penalty_time) < 
           this_best_time )
      {
        this_best_time = 
          pos_it->total_time + pos_it->total_penalty_time;
      }
    }
    TRACE_FILE_IF(5)
      TRACE("this_best_time = %f, m_best_time = %f, rec = %f, pos = %d\n",
            this_best_time,
            m_best_time,
            (rec->total_time + rec->total_penalty_time),
            pos);
    
    if ( (pos == 1) &&
         (rec->total_time + rec->total_penalty_time) < 
         (m_best_time + 0.001) )
    {
      char time_str[128];
      sprintf(time_str, "New record time: %6.2f seconds", this_best_time);
      m_text_overlay.add_entry(30, 80, time_str);      
    }
    // update the best time
    if (this_best_time < m_best_time)
      m_best_time = this_best_time;


    // It looks a bit odd to be told you're first if you're the
    // only racer.
    if (m_objects.size() > 1)
    {
      char pos_str[128];
      if (pos == 1)
        sprintf(pos_str, "You came 1st");
      else if (pos == 2)
        sprintf(pos_str, "You came 2nd");
      else if (pos == 3)
        sprintf(pos_str, "You came 3rd");
      else
        sprintf(pos_str, "You came %dth", pos);
    
      m_text_overlay.add_entry(40, 90, pos_str);
    }
  }
}

// It's rather an abomination doing this stuff here!
void Race_manager::post_physics(float dt)
{
  if (m_checkpoints.empty())
    return;

  do_glider_text();

  // if everyone is waiting to race, set the overall race state to
  // that.
  bool all_waiting_to_race = true;
  Objects_it it;
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->status != RECORD_WAITING_FOR_RACE)
    {
      all_waiting_to_race = false;
      continue;
    }
  }
  if (all_waiting_to_race)
    m_race_status = RACE_WAITING_FOR_RACE;
        
  m_timer += dt;
  switch (m_race_status)
  {
  case RACE_WAITING_FOR_RACE:
    do_waiting_for_race();
    return;
  case RACE_COUNTDOWN:
    do_countdown();
    return;
  case RACE_RACING:
    do_racing();
    return;
  case RACE_FINISHED_RACE:
    do_finished_race();
    return;
  }
  assert1(!"Unhandled race status");
}

void Race_manager::do_waiting_for_race()
{
  TRACE_METHOD_ONLY(5);
  if (m_checkpoints.empty())
    return;

  m_text_overlay.add_entry(30, 95, "Waiting for race to start");

  // If all the records are waiting for a race, we can start the
  // countdown
  
  Objects_it it;
  bool do_countdown = true;
  
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    switch (it->status)
    {
    case RACE_WAITING_FOR_RACE:
      break;
    case RACE_FINISHED_RACE:
      do_countdown = false;
      break;
    default:
      assert1(!"Impossible state");
    }
  }
  
  if (do_countdown == true)
  {
    // set all the records to racing and reset their total time
    for (it = m_objects.begin() ;
         it != m_objects.end() ;
         ++it)
    {
      it->status = RECORD_READY_FOR_RACE;
      it->total_time = 1.0e9;
    }

    // beep?
    DO_BEEP();
    TRACE("Starting race countdown\n");
    m_race_status = RACE_COUNTDOWN;
    m_timer = 0.0f;
  }
}

void Race_manager::do_countdown()
{
  TRACE_METHOD_ONLY(5);

  if (m_checkpoints.empty())
    return;

  Objects_it it;

  float time_limit;
  if (m_race_type == F3F_RACE)
    time_limit = 30.0f;
  else 
    time_limit = 15.0f;

  char time[64];
  sprintf(time, "Countdown to race: %d", (int) (time_limit - m_timer));

  m_text_overlay.add_entry(30, 95, time);

  if (m_timer >= time_limit)
  {
    // beep
    DO_BEEP();
    TRACE("Starting race\n");
    m_race_status = RACE_RACING;
    m_timer = 0.0f;
    for (it = m_objects.begin() ;
         it != m_objects.end() ;
         ++it)
    {
      it->status = RECORD_RACING;
    }
    return;
  }

  // For F3F the rule is that the timer starts when the glider 
  // crosses the line during the countdown.
  // mark any that are in the first checkpoint as ready for race 
  // - when they leave it (or the countdown expires) the race will 
  // start for all
  //
  // For a normal race the timer is the only thing that counts
  if ( (m_race_type == F3F_RACE) ||
       (m_race_type == TIMED_RACE) )
  {
    Checkpoint * cp = m_checkpoints[0];
    for (it = m_objects.begin() ;
         it != m_objects.end() ;
         ++it)
    {
      if ( (it->status == RECORD_READY_FOR_RACE) &&
           (cp->reached_checkpoint(it->object->get_pos())) )
      {
        DO_BEEP();
        TRACE("Object %p reached checkpoint 0...\n");
        it->status = RECORD_RACING;
      }
      else if ( (it->status == RECORD_RACING) &&
                (false == cp->reached_checkpoint(it->object->get_pos())) )
      {
        TRACE("Object %p left checkpoint 0 - start race\n");
        // mark all as racing, change to race, then return
        // beep
        DO_BEEP();
        TRACE("Starting race\n");
        m_race_status = RACE_RACING;
        m_timer = 0.0f;
        for (it = m_objects.begin() ;
             it != m_objects.end() ;
             ++it)
        {
          it->status = RECORD_RACING;
          it->stage = 1;
          it->leg_times[0] = 0.0f;
          it->leg_penalty_times[0] = 0.0f;
          it->leg_times.push_back(0.0f);
          it->leg_penalty_times.push_back(0.0f);
          it->time_at_start_of_leg = m_timer;
        }
        return;
      }
    }
  } // f3f race

  // make sure that none have gone back to waiting
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->status == RECORD_WAITING_FOR_RACE)
    {
      TRACE("Starting countdown again\n");
      for (it = m_objects.begin() ;
           it != m_objects.end() ;
           ++it)
        it->status = RECORD_READY_FOR_RACE;
      m_race_status = RACE_COUNTDOWN;
      m_timer = 0.0f;
      return;
    }
  }
}


void Race_manager::do_racing()
{
  TRACE_METHOD_ONLY(5);

  if (m_checkpoints.empty())
    return;

  Objects_it it;

  // check each object to see if it has reached its checkpoint
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    TRACE_FILE_IF(5)
      TRACE("object %p status = %d, stage = %d\n", 
            it->object, it->status, it->stage);

    if (it->status == RECORD_RACING)
    {
      assert1(it->stage >= 0);
      assert1(it->stage < (int) m_checkpoints.size());
      
      Checkpoint * cp = m_checkpoints[it->stage];
      
      assert1((int) it->leg_times.size() == (1 + it->stage));
      it->leg_times[it->stage] = m_timer - it->time_at_start_of_leg;

      // if F3f there is a penalty for going downwind of the body
      // (considered unsafe!)
      if (m_race_type == F3F_RACE)
      {
        Velocity body_wind_vel = Environment::instance()->
          get_non_turbulent_wind(Sss::instance()->body().get_pos());
        body_wind_vel[2] = 0;
        const Vector3 glider_vec = it->object->get_pos() - 
          Sss::instance()->body().get_pos();
        
        if (dot(glider_vec, body_wind_vel) > 0.0f)
        {
          TRACE_FILE_IF(5)
            TRACE("object %p in penalty zone!\n", it->object);
          // just one penalty per stage?
          assert1((int) it->leg_penalty_times.size() == (1 + it->stage));
          it->leg_penalty_times[it->stage] = Sss::instance()->config().f3f_penalty;
        }
      } // penalty for f3f
      
      // check for reached the next checkpoint
      if (cp->reached_checkpoint(it->object->get_pos()))
      {
        DO_BEEP();
        TRACE("Object %p reached checkpoint for stage %d\n",
              it->object,
              it->stage);

        if (it->stage == (int) m_checkpoints.size() - 1)
        {
          TRACE("Object %p finished race!\n");
          it->status = RECORD_FINISHED_RACE;
          it->total_time = m_timer;
          // add on the penalties
          it->total_penalty_time += accumulate(it->leg_penalty_times.begin(),
                                               it->leg_penalty_times.end(),
                                               0.0f);
        }
        else
        {
          ++(it->stage);
          it->leg_times.push_back(0.0f);
          it->leg_penalty_times.push_back(0.0f);
          it->time_at_start_of_leg = m_timer;
        }
      } // reached checkpoint
    } // RACING
  } // loop

  bool all_finished_race = true;
  for (it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->status == RECORD_RACING)
    {
      all_finished_race = false;
      continue;
    }
  }
  if (all_finished_race)
    m_race_status = RACE_FINISHED_RACE;
}

void Race_manager::do_finished_race()
{
  TRACE_METHOD_ONLY(5);
  if (m_checkpoints.empty())
    return;
}

void Race_manager::register_object(Object * object)
{
  TRACE_METHOD_ONLY(2);

  Record record;
  record.object = object;
  record.stage = 0;
  record.status = RECORD_WAITING_FOR_RACE;
  record.leg_times.push_back(0.0f);
  record.leg_penalty_times.push_back(0.0f);
  record.total_time = 1.0e9;
  record.time_at_start_of_leg = 0.0f;
  m_objects.push_back(record);
}

void Race_manager::deregister_object(const Object * object)
{
  TRACE_METHOD_ONLY(2);

  for (Objects_it it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->object == object)
    {
      m_objects.erase(it);
      return;
    }
  }
}

void Race_manager::reset_object(const Object * object)
{
  TRACE_METHOD_ONLY(2);
  
  for (Objects_it it = m_objects.begin() ;
       it != m_objects.end() ;
       ++it)
  {
    if (it->object == object)
    {
      it->stage = 0;
      it->status = RECORD_WAITING_FOR_RACE;
      it->leg_times.clear();
      it->leg_times.push_back(0.0f);
      it->leg_penalty_times.clear();
      it->leg_penalty_times.push_back(0.0f);
      it->time_at_start_of_leg = 0.0f;
      return;
    }
  }
}

void Race_manager::add_checkpoints(
  const std::vector<Checkpoint *> & checkpoints)
{
  TRACE_FUNCTION_ONLY(2);
  
  // have we been created?
  if (s_instance)
  {
    copy(checkpoints.begin(), checkpoints.end(), 
         back_inserter(s_instance->m_checkpoints));
    copy(checkpoints.begin(), checkpoints.end(), 
         back_inserter(s_instance->m_checkpoints_to_draw));
    s_instance->m_checkpoints_to_draw.erase(
      unique(s_instance->m_checkpoints_to_draw.begin(),
             s_instance->m_checkpoints_to_draw.end()),
      s_instance->m_checkpoints_to_draw.end());
  }
  else
  {
    copy(checkpoints.begin(), checkpoints.end(), 
         back_inserter(s_pending_checkpoints));
    TRACE_FILE_IF(2)
      TRACE("Inserted %d: Currently %d checkpoints pending\n", 
            checkpoints.size(), s_pending_checkpoints.size());
  }
}


void Race_manager::draw(Object::Draw_type draw_type)
{
  TRACE_METHOD_ONLY(4);
  if (m_checkpoints.empty())
    return;
  int highlight_stage = m_objects.empty() ? 0 : m_objects.begin()->stage;
  Checkpoint * highlight_cp = 0;
  if ((highlight_stage >= 0) && highlight_stage < (int) m_checkpoints.size())
    highlight_cp = m_checkpoints[highlight_stage];

  int i;
  for (i = 0 ; i < (int) m_checkpoints_to_draw.size() ; ++i)
  {
    if ( m_checkpoints_to_draw[i] == highlight_cp )
      glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    else
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    m_checkpoints_to_draw[i]->draw();
  }
}

// the checkpoints

void Checkpoint_sphere::draw()
{
  TRACE_METHOD_ONLY(5);
  glPushMatrix();
  glTranslatef(pos[0], pos[1], pos[2]);
  gluSphere(Renderer::instance()->quadric(), radius, 8, 8);
  glPopMatrix();
}

bool Checkpoint_sphere::reached_checkpoint(const Position & pos1) const
{
  return ( (pos1 - pos).mag() < radius);
}

void Checkpoint_cylinder::draw()
{
  glPushMatrix();
  glTranslatef(x, y, -500.0f);
  gluCylinder(Renderer::instance()->quadric(),
              radius,
              radius,
              1000, 8, 2);
  glPopMatrix();
}

bool Checkpoint_cylinder::reached_checkpoint(const Position & pos) const
{
  return (hypot(pos[0] - x, pos[1] - y) < radius);
}

void Checkpoint_plane::draw()
{
  static GLuint list_num = 0;
  if (list_num == 0)
  {
    list_num = glGenLists(1);
    glNewList(list_num, GL_COMPILE);
    gluCylinder(Renderer::instance()->quadric(),
                0.1,
                0.1,
                500, 4, 4);
    glEndList();
  }

  glPushMatrix();
  glTranslatef(x0, y0, 0.0f);
  glCallList(list_num);
  glTranslatef(x1-x0, y1-y0, 0.0f);
  glCallList(list_num);
  glPopMatrix();
}

Checkpoint_plane::Checkpoint_plane(float x0, float y0, float x1, float y1) : 
  x0(x0), y0(y0), x1(x1), y1(y1) 
{
  // find the normal vector
  Vector p0p1 = Position(x1, y1, 0) - Position(x0, y0, 0);
  Vector up(0, 0, 1);
  Vector norm = cross(up, p0p1);
  norm.normalise();
  
  A = norm[0];
  B = norm[1];
  C = -dot(norm, Position(x0, y0, 0));

  // work out a suitable target position (for robots to use) - let's say 15m
  // into wind relative to the upwind pole.
  Position pos(0.5 * (x0 + x1), 0.5 * (y0 + y1), 0);
  Environment::instance()->set_z(pos);
  pos[2] += 5;
  Velocity wind_vel = -Environment::instance()->get_non_turbulent_wind(pos);
  wind_vel[2] = 0.0f;
  wind_vel.normalise();
  // now wind_vel points horizontally into wind
  if (dot(wind_vel, p0p1) > 0)
  {
    //put target on the side of p1
    TRACE_FILE_IF(3)
      TRACE("Target is on side of p1\n");
    target = pos + p0p1 * 4;
  } 
  else
  {
    TRACE_FILE_IF(3)
      TRACE("Target is on side of p0\n");
    target = pos - p0p1 * 4;
  }
}


bool Checkpoint_plane::reached_checkpoint(const Position & pos) const
{
  float dist = dot(Vector(A, B, 0), pos) + C;
  return dist > 0;
}

void Checkpoint_gate::draw()
{
  glPushMatrix();
  glTranslatef(x0, y0, z0);
  glCallList(list_num);
  glPopMatrix();
}

Checkpoint_gate::Checkpoint_gate(float x0, float y0, float x1, float y1) 
  : 
  x0(x0), y0(y0), x1(x1), y1(y1) 
{
  z0 = Environment::instance()->get_z(x0, y0);
  z1 = Environment::instance()->get_z(x1, y1);

  // find the normal vector
  Vector p0p1 = Position(x1, y1, 0) - Position(x0, y0, 0);
  Vector up(0, 0, 1);
  Vector norm = cross(up, p0p1);
  norm.normalise();
  
  A = norm[0];
  B = norm[1];
  C = -dot(norm, Position(x0, y0, 0));

  // work out a suitable target position (for robots to use) - let's
  // say the mid-point, and up a bit
  target = Position(0.5 * (x0 + x1), 0.5 * (y0 + y1), 0);
  Environment::instance()->set_z(target);
  target[2] += 5;

  // prepare the display list
  float pole_len = 4.0f;
  max_z = pole_len + sss_max(z0, z1);

  list_num = glGenLists(1);
  glNewList(list_num, GL_COMPILE);
  // left
  gluCylinder(Renderer::instance()->quadric(),
              0.2,
              0.2,
              max_z - z0, 4, 4);
  // right
  glTranslatef(x1-x0, y1-y0, z1-z0);
  gluCylinder(Renderer::instance()->quadric(),
              0.2,
              0.2,
              max_z - z1, 4, 4);
  // and the cross-bar - do it starting from the top left one.
  glTranslatef(x0-x1, y0-y1, z0-z1);
  glTranslatef(0, 0, max_z - z0);
  glRotatef(90, -1, 0, 0);
  glRotatef(atan2_deg(x1-x0, y1-y0), 0, 1, 0);
  float cross_len = hypot(x1-x0, y1-y0);
  gluCylinder(Renderer::instance()->quadric(),
              0.2,
              0.2,
              cross_len, 4, 4);
  
  
  glEndList();
}


bool Checkpoint_gate::reached_checkpoint(const Position & pos) const
{
  // under the cross-bar?
  if (pos[2] > max_z)
    return false;
  
  float dist = dot(Vector(A, B, 0), pos) + C;
  // wrong side, or too far on the right side
  if ( (dist < 0) ||
       (dist > 5) )
    return false;
  // need to be right side and within the poles (+ dist < a certain
  // amount?)
  Vector AB = Position(x1, y1, 0) - Position(x0, y0, 0);
  if ( ( dot(pos - Position(x0, y0, 0), AB) > 0) &&
       ( dot(pos - Position(x1, y1, 0), -AB) > 0) )
  {
    TRACE_FILE_IF(3)
      TRACE("Through gate %p\n", this);
    return true;
  }
  return false;
}

