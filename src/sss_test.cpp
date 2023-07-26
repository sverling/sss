/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file sss.cpp
*/

#include "bludger.h"
#include "body.h"
#include "config_file.h"
#include "control_method.h"
#include "environment.h"
#include "sss_assert.h"
#include "audio.h"
#include "config.h"
#include "gui.h"
#include "sss.h"
#include "physics.h"
#include "renderer.h"
#include "glider.h"
#include "joystick.h"
#include "log_trace.h"
#include "pilot_manager.h"
#include "race_manager.h"
#include "remote_sss_iface.h"
#include "robot_pilot.h"
#include "text_overlay.h"
#include "tracer.h"
#include "tx_audio_input.h"
#include "texture.h"

#ifdef WITH_PLIB
#include <plib/js.h>
#endif

#include "sss_glut.h"
#include <math.h>

#include <algorithm>
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

float get_seconds();
static float last_dt = 0.0f;

Sss * Sss::m_instance = 0;

Sss::~Sss()
{
  TRACE_METHOD_ONLY(1);
  remove_object(m_glider);
  remove_object(m_body);
  
  // delete the things in object_list
  for (unsigned int i = 0 ; i < m_object_list.size() ; ++i)
  {
    if (m_object_list[i])
      delete m_object_list[i];
  }
  m_object_list.clear();
  
  m_renderer->remove_text_overlay(m_text_overlay);
  
  delete m_renderer;
  delete m_text_overlay;
  
  m_instance = 0;
}

Sss * Sss::create_instance(Config & config)
{
  TRACE_METHOD_STATIC_ONLY(1);
  if (0 == m_instance)
    m_instance = new Sss(config);
  return m_instance;
}


Sss::Sss(Config & config)
  :
  m_config(config),
  m_glider(0),
  m_environment(0),
  m_body(0),
  m_eye(0),
  m_joystick(0),
  m_renderer(0),
  m_text_overlay(new Text_overlay),
  m_sss_state(SSS_FLYING),
  m_allow_config(false),
  m_enter_text_mode(false),
  m_remote_text_timer(0),
  m_show_help(false),
  m_move_body_mode(false)
{
  TRACE_METHOD_ONLY(1);
#ifdef WITH_PLIB
  jsInit () ;
  m_js = new jsJoystick(0);
  if (m_js->notWorking())
  {
    TRACE("Joystick not working/detected - use mouse/keyboard instead\n");
  }
#endif
  
  m_instance = this; // m_instance is static - needed for the call to Renderer
  m_renderer = Renderer::instance();
  TRACE_FILE_IF(2)
    TRACE("About to initialise timer\n");
  // probably not the best value... but at least it's not way off.
  m_last_update_time = get_seconds();
  
}

/*!  Prior to calling this, other objects must only access the config
  - everything else is not guaranteed to exist.  */
void Sss::initialise(Glider * glider,
                     Environment * environment,
                     Body * body,
                     Joystick * joystick)
{
  TRACE_METHOD_ONLY(1);
  m_glider = glider;
  m_environment = environment;
  m_body = body;
  m_eye = body;
  m_eye->set_eye(true);
  m_joystick = joystick;
  
  // create the new objects
//    m_text_overlay = new Text_overlay;
//    m_sss_state = SSS_PAUSED;
  
  // register the callbacks with glut
  glutDisplayFunc(_display);
  Gui::instance()->set_keyboard_func(_key);
  Gui::instance()->set_special_func(_special);
  Gui::instance()->set_special_up_func(_special_up);
  Gui::instance()->set_mouse_func(_mouse);
  Gui::instance()->set_reshape_func(_reshape);
  Gui::instance()->set_idle_func(_idle);
  
  // use all these whatever the actual input method
  glutMotionFunc(_motion);
  glutPassiveMotionFunc(_motion);
  
  // store the glider and body objects in the object list
  
  add_object(m_glider);
  add_object(m_body);
  
  for (int i = 0 ; i < m_config.tracer_count ; ++i)
  {
    Tracer_collection::instance()->add_tracer(m_glider);
  }
  
  // set up the joystick
  reset_joystick();

  // we own a text overlay object, and want to register it so that it
  // will get drawn (but we continue to own it).
  m_renderer->add_text_overlay(m_text_overlay);
  
  toggle_paused();
}

/*!
  adding an object transfers ownership - the object will be deleted
  when Sss dies (not that it ever will), unless it is
  removed first.
*/
void Sss::add_object(Object * new_object)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD(); TRACE("new object = %p\n", new_object); }
  m_object_list.push_back(new_object);
}

int Sss::remove_object(const Object * object)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD(); TRACE("object = %p\n", object); }
  
  vector<Object *>::iterator it = remove(m_object_list.begin(),
                                         m_object_list.end(),
                                         object);
  if (it != m_object_list.end())
  {
    m_object_list.erase(it, m_object_list.end());
    return 0;
  }
  return -1;
}

/*! 
  start must be called after initialise. It will never return, as it
  enters the glut main event loop. 
*/
void Sss::start()
{
  TRACE_METHOD_ONLY(1);
  
  if (0 == m_glider)
  {
    TRACE("Sss::start called, but not initialised!\n");
    assert1(!"error!");
  }
  
  TRACE_FILE_IF(1)
    TRACE("Entering GLUT main looop\n");
  glutMainLoop();
  TRACE_FILE_IF(1)
    TRACE("Leaving GLUT main looop\n");
}

float Sss::get_seconds() const
{
#ifdef WIN32
  if (m_config.timer_method == Config::TIMER_PERF)
  {
    LARGE_INTEGER currentTime;
    static LARGE_INTEGER frequency;
    static bool use_perf = QueryPerformanceFrequency(&frequency);
    static bool init = false;
    if (init == false)
    {
      init = true;
      if (use_perf)
        TRACE("Using QueryPerformance for timing\n");
      else
        TRACE("QueryPerformance not available - using GLUT for timing\n");
    }
    if (use_perf)
    {
//      QueryPerformanceFrequency(&frequency);
      QueryPerformanceCounter(&currentTime);
      return (float)currentTime.QuadPart / (float)frequency.QuadPart;
    }
    else
    {
      return 0.001f * glutGet(GLUT_ELAPSED_TIME);
    }
  }
  else // do not use high res timer
  {
    return 0.001f * glutGet(GLUT_ELAPSED_TIME);
  }
#else
  return (float) (0.001 * glutGet(GLUT_ELAPSED_TIME));
#endif
}

void Sss::idle()
{
  TRACE_METHOD_ONLY(3);
  
  static int count = 0;
  int loop = 5;
  static float last_time = get_seconds();
  static char s[100]=" ";
  static char s2[100]=" ";
  static char s3[100]=" ";
  static char s4[100]=" ";
  static char s5[100]=" ";
  
  float fs = 20;
  
  if ((count++ % loop) == 0)
  {
    float this_time = get_seconds();
    float dt = (this_time - last_time); 
    if (this_time == last_time)
      dt = 0.001f;
    dt = dt/loop;
    fs = 1/dt;
    last_time = this_time;
    
    if (m_config.target_frame_rate > 0)
    {
      float target_fs = m_config.target_frame_rate;
      float fac = 1 + 0.1f * (fs-target_fs) / target_fs;
      if (fac < 0.5f)
        fac = 0.5f;
      else if (fac > 2)
        fac = 2;
      
      m_config.lod *= fac;
      if (m_config.lod < 0.1f)
        m_config.lod = 0.1f;
    }
    
    if (m_config.text_overlay > Config::TEXT_NONE)
    {
      if (m_config.text_overlay > Config::TEXT_LITTLE)
      {
        sprintf(s, "%d : %d : %d", 
                (int) fs, 
                (int) m_config.lod, 
                m_environment->get_terrain_triangles());
      }
      else
      {
        sprintf(s, "%d : %d", (int) fs, (int) m_config.lod);
      }
      
      if (m_config.text_overlay > Config::TEXT_LITTLE)
      {
        // wind speed
        Velocity ambient_wind = m_environment->get_non_turbulent_wind(
          m_glider->get_pos());
        sprintf(s2, "Wind (u,v,w) = %5.2f, %5.2f, %5.2f", 
                ambient_wind[0], ambient_wind[1], ambient_wind[2]);
        
        // glider speed
        Velocity glider_vel = m_glider->get_vel();
        float speed = glider_vel.mag();
        float climb_rate = glider_vel[2];
        sprintf(s3, "Glider (ground speed, climb rate) = %5.1f, %5.2f",
                speed,
                climb_rate);
        
        Velocity air_vel = (glider_vel-ambient_wind);
        
        sprintf(s4, "Glider (air speed, climb rate) = %5.1f, %5.2f",
                air_vel.mag(),
                air_vel[2]);
        
        // eye pos
        Position eye_pos = m_eye->get_eye();
        sprintf(s5, "Eye (x, y, z, h) = %5.1f, %5.1f, %5.2f, %5.2f",
                eye_pos[0], eye_pos[1], eye_pos[2],
                eye_pos[2] - m_environment->get_z(eye_pos[0], eye_pos[1]));
      }
    }
    
//      else
//      {
//        TRACE("FPS = %6.2f   lod = %f \n", fs, config.lod);
//      }
  }
  
  if (m_config.text_overlay > Config::TEXT_NONE)
  {
    // either display the framerate, or the outgoing text message
    if (m_enter_text_mode == false)
      m_text_overlay->add_entry(2,95,s);
    
    if (m_config.text_overlay > Config::TEXT_LITTLE)
    {
      m_text_overlay->add_entry(2, 1,s5);
      m_text_overlay->add_entry(2, 4,s4);
      m_text_overlay->add_entry(2, 7,s3);
      m_text_overlay->add_entry(2,10,s2);
    }
    
    if (m_move_body_mode == true)
      m_text_overlay->add_entry(
        20, 50,
        "Click on the terrain to set the viewpoint/body poistion");
  }
  
  if (m_enter_text_mode == true)
  {
    m_text_overlay->add_entry(2,95,m_local_text.c_str());
  }
  // and if there is an incoming text message, display it
  if (m_remote_text.length() > 0)
  {
    m_text_overlay->add_entry(2,85,m_remote_text.c_str());
  }
  
  float current_time = get_seconds();
  float dt = current_time - m_last_update_time;
//  TRACE("%d\n", current_time - last_update_time);
  m_last_update_time = current_time;
  
  // If dt is huge we don't want to go and do millions of physics
  // updates - clamp it and emit a warning.  In addition, if we're
  // doing a movie then clamp dt to the movie dt - this means that the
  // movie will run at an even speed, even if this simulation actually
  // runs slow. It may run slow because of the disk writing.
  if (config().do_movie == true)
  {
    if (dt > config().movie_dt)
      dt = config().movie_dt;
  }
  else if (dt > 0.5f)
  {
    dt = 0.5f;
    TRACE("Warning - dt is too large, clamping to 0.5 sec\n");
  }
  
  // If the time jumps...
  if (dt < 0.0f) dt = 0.0f;
  
  // store it so it can be used elsewhere - e.g. in key presses
  last_dt = dt;

  // set to constant all unused channels
  m_config.control_method->set_const_channels(m_joystick);
  
  // read physical joystick (if there is one)
  if (Sss::instance()->config().tx_audio==false)
    do_joystick();
  else
    do_tx_audio();
  
  // do the sound stuff
  if (true == m_config.use_audio)
    Audio::instance()->do_audio(m_eye);
  
  Environment::instance()->update_environment(dt);
  Pilot_manager::instance()->update_pilots(dt);
  Physics::instance()->do_timestep(dt);
  Remote_sss_iface * iface;
  if ((iface = Remote_sss_iface::instance()))
    iface->recv_msgs();
  
  // decrement the text timer
  if (m_remote_text_timer > 0)
  {
    m_remote_text_timer -= dt;
    if (m_remote_text_timer < 0)
    {
      m_remote_text = ""; // windoze doesn't allow clear
    }
  }
  
  
  if (m_sss_state == SSS_UNPAUSED)
    m_sss_state = SSS_FLYING;
  
  glutPostRedisplay();
}


void Sss::add_robot()
{
  bool success;
  string real_robot_file = "robots/" + config().robot_file;
  Config_file robot_config_file(real_robot_file,
                                success);
  
  // Try to keep the gliders separated a bit...
  int offset;
  for (offset = 3 ; offset < 60 ; offset += 3)
  {
    bool use_offset = true;
    for (Robot_offset_it it = m_robot_offsets.begin();
         it != m_robot_offsets.end();
         ++it)
    {
      if (offset == it->second)
        use_offset = false;
    }
    if (use_offset == true)
      break;
  }
  if (success)
  {
    Robot_pilot * robot_pilot = new Robot_pilot(robot_config_file,
                                                m_body->get_pos(), 
                                                offset);
    TRACE("Created robot pilot with offset %d\n", offset);
    m_robot_offsets.insert(make_pair(robot_pilot, offset));
  }
  else
  {
    TRACE("Unable to get robot pilot config\n");
  }
}

void Sss::remove_robot()
{
  TRACE_METHOD_ONLY(2);
  set<Pilot *> pilots = Pilot_manager::instance()->get_pilots();
  if (pilots.begin() != pilots.end())
  {
    Pilot * robot = *(pilots.begin());
    if (robot->get_glider() == m_glider)
    {
      // if running in demo make sure we don't delete the main glider
      set<Pilot *>::iterator it = pilots.begin();
      ++it;
      if (it != pilots.end())
        robot = *it;
      else
      {
        TRACE("No robots left to remove\n");
        return;
      }
    }
    delete robot;
    m_robot_offsets.erase(robot);
  }
  else
  {
    TRACE("No robots left to remove\n");
  }
}
void Sss::display(void)
{
  TRACE_METHOD_ONLY(3);
#ifdef WIN32
  static unsigned display_count = 0;
  if (display_count == 2)
  {
    if (m_config.close_console && !m_config.trace_enabled)
    {
      TRACE("Displayed a couple of frames - closing the console window\n");
      // close the ugly DOS window
      FreeConsole();
    }
  }
  // paranoia - make sure we don't wrap around!
  if (display_count < 10)
    ++display_count;
#endif
  
  // limit multitexturing
  if ( (multitextureSupported == false) && (m_config.texture_level > 4) )
    m_config.texture_level = 4;

  if ( (m_sss_state == SSS_PAUSED) && 
       (config().text_overlay != Config::TEXT_NONE) && 
       (m_show_help == false)
    )
  {
    TRACE("test 1\n");
    m_text_overlay->add_entry(26, 8, "PAUSED - press 'p' to start flying/toggle paused");
    TRACE("test 2\n");
    m_text_overlay->add_entry(32, 5, "'h' to toggle the on-screen help");
#ifdef WITH_GLUI
    m_text_overlay->add_entry(29, 2, "or 'c' to toggle the configuration window");
    TRACE("test 3\n");
#endif
  }
  
  if (m_show_help == true)
  {
    int y = 99;
    int x = 15;
    int dy = 3;
    m_text_overlay->add_entry(x, y -= dy, "Keys and their actions (possibly in order of usefulness):");
    x += 1;
    m_text_overlay->add_entry(x, y -= dy, "p: Toggle pause (only affects the main glider)");
    m_text_overlay->add_entry(x, y -= dy, "q: Quit");
    m_text_overlay->add_entry(x, y -= dy, "c: Show the configuration GUI");
    m_text_overlay->add_entry(x, y -= dy, "h: Toggle showing of this help text");
    m_text_overlay->add_entry(x, y -= dy, "r: Reset the main glider");
    m_text_overlay->add_entry(x, y -= dy, "g: View from in/behind glider");
    m_text_overlay->add_entry(x, y -= dy, "b: View from body");
    m_text_overlay->add_entry(x, y -= dy, "B: Set the body position");
    m_text_overlay->add_entry(x, y -= dy, "f: Enter full-screen mode");
    m_text_overlay->add_entry(x, y -= dy, "o/O: Add/remove robot");
    m_text_overlay->add_entry(x, y -= dy, "w/W: Add/remove wind tracer");
    m_text_overlay->add_entry(x, y -= dy, "<Space>: Fire missile (via channel 5)");
    m_text_overlay->add_entry(x, y -= dy, "<Right mouse button>: Throttle (via channel 3)");
    m_text_overlay->add_entry(x, y -= dy, "=/- or mouse wheel: Zoom in/out");
    m_text_overlay->add_entry(x, y -= dy, "0/9: Increase/decrease terrain detail");
    m_text_overlay->add_entry(x, y -= dy, "m/j: Use mouse/joystick for control");
    m_text_overlay->add_entry(x, y -= dy, "t: Display more/less text");
    m_text_overlay->add_entry(x, y -= dy, "R: Reset the race (when racing), though robots will continue till they finish!");
    m_text_overlay->add_entry(x, y -= dy, "<arrow keys>: Alternative glider control (with shift, move the body location)");
    m_text_overlay->add_entry(x, y -= dy, "1-8: Zero the trim on the corresponding channel (the most useful is 2 for pitch");
    m_text_overlay->add_entry(x, y -= dy, "<Return>: Start/end text mode (when networked)");
    m_text_overlay->add_entry(x, y -= dy, "J: Toggle jitter correction (when networked)");
    m_text_overlay->add_entry(x, y -= dy, "L: Toggle lag correction (when networked)");
    y -= 1;
    x = 2;
    m_text_overlay->add_entry(x, y -= dy, "Editing sss.cfg (e.g. with notepad under Windows) allows many things to be configured.");
    m_text_overlay->add_entry(x, y -= dy, "Controlling the glider: If you have a joystick, or some other device that your operating");
    m_text_overlay->add_entry(x, y -= dy, "system recognises as a joystick, just pressing 'j' should work. Otherwise, you have to");
    m_text_overlay->add_entry(x, y -= dy, "use a mouse (or arrow keys) - much harder (at first). The idea is that the position of the mouse");
    m_text_overlay->add_entry(x, y -= dy, "pointer on the screen represents the top of the joystick - so moving the mouse forward");
    m_text_overlay->add_entry(x, y -= dy, "results in 'pushing the joystick forward' - i.e. pitch down. The 'joystick' is neutral");
    m_text_overlay->add_entry(x, y -= dy, "when the pointer is at the center of the glider. 'a' toggles the auto-pilot in 'demo' mode.");
    m_text_overlay->add_entry(x, y -= dy, "The easiest glider to fly is probably glider_rudder.dat - select it by pressing 'c'.");
  }
  else
  {
    // When the main glider is set up with a Robot_pilot, it's so easy to bypass it 
    // (i.e. turn the "auto-pilot" off). Much easier that creating a Robot_pilot 
    // when the sim is started without one. However, there is some overhead associated
    // with the robot AI, even when the end result gets ignored, though that could be 
    // easily eliminated.
    if (m_config.sss_mode == Config::DEMO_MODE)
    {
      TRACE("test 4\n");
      bool is_autopilot_on = (m_joystick != &m_glider->get_joystick());
      TRACE("test 5\n");
      char text[128];
      sprintf(text, "Demo mode - press 'a' to turn the auto-pilot %s", 
              is_autopilot_on ? "off" : "on");
      TRACE("test 6\n");
      m_text_overlay->add_entry(30, 97, text);
      TRACE("test 7\n");
    }
  }
  
  TRACE("test 8 - renderer = %p\n", m_renderer);
  
  m_renderer->render_objects(
    m_object_list,
    m_eye, 
    m_environment, 
    m_glider,
    m_show_help); // if help = true, dim everything else
}


void Sss::mouse(int button, int state, int x, int y)
{
  if (m_move_body_mode)
  {
    move_body(x, y);
    m_move_body_mode = false;
  }
  
  Control_method * control_method = m_config.control_method;
  if ( GLUT_DOWN == state)
  {
    switch(button)
    {
    case GLUT_LEFT_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_LEFT) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_LEFT),
          1.0f);
      } 
      break;
    case GLUT_MIDDLE_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_MIDDLE) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_MIDDLE),
          1.0f);
      } 
      break;
    case GLUT_RIGHT_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_RIGHT) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_RIGHT),
          1.0f);
      } 
      break;
    case 3: // mouse scroll up
      m_renderer->set_fov(sss_max(5.0f, config().fov-2));
      Gui::instance()->update_fov();
      break;
    case 4: // mouse scroll down
      m_renderer->set_fov(sss_max(5.0f, config().fov+2));
      Gui::instance()->update_fov();
      break;
    default:
      TRACE("Button down: %d\n", button);
      break;
    }
  } 
  else // ie mouse up
  {
    switch(button)
    {
    case GLUT_LEFT_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_LEFT) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_LEFT),
          -1.0f);
      } 
      break;
    case GLUT_MIDDLE_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_MIDDLE) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_MIDDLE),
          -1.0f);
      } 
      break;
    case GLUT_RIGHT_BUTTON:
      if (control_method->get_channel(Control_method::MOUSE_BUTTON_RIGHT) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(Control_method::MOUSE_BUTTON_RIGHT),
          -1.0f);
      } 
      break;
    default:
//      TRACE("Button UP: %d\n", button);
      break;
    }
  }
  //  joystick->show();
}

void Sss::motion(int x, int y)
{
  Control_method * control_method = m_config.control_method;
  if (control_method->get_channel(Control_method::MOUSE_X) >= 0)
  {
    m_joystick->set_value(
      control_method->get_channel(Control_method::MOUSE_X),
      control_method->get_warped_value(
        Control_method::MOUSE_X, 
        (2.0f * (float) x / m_config.window_x) - 1.0f) );
  }
  
  if (control_method->get_channel(Control_method::MOUSE_Y) >= 0)
  {
    m_joystick->set_value(
      control_method->get_channel(Control_method::MOUSE_Y),
      control_method->get_warped_value(
        Control_method::MOUSE_Y,
        (2.0f * (float) y / m_config.window_y) - 1.0f) );
  }
  //  joystick->show();
}

void Sss::do_joystick()
{
  TRACE_METHOD_ONLY(3);
#ifdef WITH_PLIB
  if (!m_js->notWorking())
  {
    // support up to 6 channels
    static const int num_ax = (m_js->getNumAxes() < 6 ? m_js->getNumAxes() : 6);
    static float * ax = new float[num_ax];
    int i;
    int button;
    
    if (num_ax <= 0)
      return;
    
    for (i = 0 ; i < num_ax ; ++i)
    {
      m_js->read(&button, ax); // in range -1 - +1
    }
    
    static Control_method::Control_input joysticks[] = 
      {
        Control_method::JOYSTICK_1,
        Control_method::JOYSTICK_2,
        Control_method::JOYSTICK_3,
        Control_method::JOYSTICK_4,
        Control_method::JOYSTICK_5,
        Control_method::JOYSTICK_6,
      };
    
    Control_method * control_method = m_config.control_method;
    
    for (i = 0 ; i < num_ax ; ++i)
    {
      if (control_method->get_channel(joysticks[i]) >= 0)
      {
        m_joystick->set_value(
          control_method->get_channel(joysticks[i]),
          control_method->get_warped_value(joysticks[i], ax[i]));
      }
    }
    
//  if (button_mask)
//    m_joystick->set_value(5, 1);
    
//  m_joystick->show();
  }
#endif
}

void Sss::do_tx_audio()
{
  TRACE_METHOD_ONLY(3);
#ifdef WIN32
  if (Sss::instance()->config().tx_audio==false)
    return;
  
  static TX_Audio_Input Audio_Tx;
  // support up to 6 channels
  static const int num_ax = 6;
  static float * ax = new float[num_ax];
  int i, nvalues;
  Audio_Tx.Get_TX_Audio(ax,&nvalues);
  
  static Control_method::Control_input joysticks[] = 
    {
      Control_method::JOYSTICK_1,
      Control_method::JOYSTICK_2,
      Control_method::JOYSTICK_3,
      Control_method::JOYSTICK_4,
      Control_method::JOYSTICK_5,
      Control_method::JOYSTICK_6,
    };
  
  Control_method * control_method = m_config.control_method;
  
  for (i = 0 ; i < num_ax ; ++i)
  {
    if (control_method->get_channel(joysticks[i]) >= 0)
    {
      m_joystick->set_value(
        control_method->get_channel(joysticks[i]),
        control_method->get_warped_value(joysticks[i], ax[i]));
    }
  }
#endif
}


void Sss::reshape(int w, int h)
{
  m_renderer->reshape(w, h);
}

bool Sss::add_letter_to_text(unsigned char key)
{
  if ( (key >= 32) && (key <= 126) )
  {
    // normal character
    if (m_local_text.length() < sizeof(Remote_sss_text_msg))
    {
      m_local_text += key;
    }
    else
    {
      TRACE("Too many characters in text\n");
    }
    return true;
  }
  
  if ( (key == 127) || (key == 8) )
  {
    // delete
    if (m_local_text.length() > 0)
      m_local_text.resize(m_local_text.length() - 1);
    
    return true;
  }
  return false;
}

void Sss::send_text_message()
{
  TRACE_METHOD_ONLY(2);
  Remote_sss_msg msg;
  msg.msg_type = Remote_sss_msg::TEXT;
  strcpy(msg.msg.text.message, m_local_text.c_str());
  
  Remote_sss_iface * iface = Remote_sss_iface::instance();
  if (iface)
  {
    iface->send_msg(msg);
  }
  else
  {
    TRACE("No remote peer\n");
  }
  m_local_text = "";
}

void Sss::recv_text_msg(const char * text)
{
  // note that this is a static method
  m_instance->m_remote_text = string(text);
  m_instance->m_remote_text_timer = 5;
}

void Sss::move_body(int x, int y)
{
// y is 0 at top-left, when it should be bottom left
  
  y = glutGet(GLUT_WINDOW_HEIGHT) - y;
//  TRACE("x = %d, y = %d\n", x, y);
  
  // set up matrices
  
  m_renderer->setup_view_matrices(m_eye);
  
  // Unproject. Note that winz = 0 gives us the world coords at the
  // near clipping plane, and winz = 1 gives us the far plane.
  
  GLint viewport[4];
  GLdouble model_matrix[16], proj_matrix[16];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX, model_matrix);
  glGetDoublev(GL_PROJECTION_MATRIX, proj_matrix);
  
  GLdouble near_x, near_y, near_z;
  GLdouble far_x, far_y, far_z;
  
  // near
  if (gluUnProject((GLdouble) x, (GLdouble) y, 0.0,
                   model_matrix,
                   proj_matrix,
                   viewport,
                   &near_x, &near_y, &near_z) != GL_TRUE)
  {
    TRACE("gluUnProject error!\n");      
  }
  
//   TRACE("near_x = %f, near_y = %f, near_z = %f\n",
//         near_x, near_y, near_z);
  // far
  gluUnProject((GLdouble) x, (GLdouble) y, 1.0,
               model_matrix,
               proj_matrix,
               viewport,
               &far_x, &far_y, &far_z);
  
//   TRACE("far_x = %f, far_y = %f, far_z = %f\n",
//         far_x, far_y, far_z);

  // Now walk along the ray, looking for intersection with the ground.
  Position pos_near(near_x, near_y, near_z);
  Position pos_far(far_x, far_y, far_z);

  Position pos = m_environment->get_ground_intersection(pos_near, pos_far);
  
  // now update
  m_body->set_pos(pos);
  m_body->calc_new_pos_and_orient(0);
  // and all the pilots...
  set<Pilot *> pilots = Pilot_manager::instance()->get_pilots();
  for (set<Pilot *>::iterator it = pilots.begin() ;
       it != pilots.end() ;
       ++it)
  {
    (*it)->reset(m_body);
  }

}


void Sss::key(unsigned char key, int px, int py)
{
  if (m_enter_text_mode)
  {
    if (true == add_letter_to_text(key))
    {
      glutPostRedisplay();
      return;
    }
  }
  
  // Either not in enter text mode, or it's not a key we're interested
  // in for the message.
  
  switch(key)
  {
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
    TRACE("setting trim on channel %c\n", key);
    m_joystick->set_trim(1+key-'1', m_joystick->get_value(1+key-'1'));
    break;
  case 13:
    if (m_enter_text_mode == true)
    {
      TRACE("Sending text message\n");
      send_text_message();
      m_enter_text_mode = false;
    }
    else
    {
      if (Remote_sss_iface::instance())
      {
        TRACE("Enter text mode\n");
        m_enter_text_mode = true;
      }
      else
      {
        TRACE("No remote peer\n");
      }
    }
    break;
  case 'J':
    if (config().jitter_correct == true)
    {
      set_config().jitter_correct = false;
      TRACE("Disabling jitter correction\n");
    }
    else
    {
      set_config().jitter_correct = true;
      TRACE("Enabling jitter correction\n");
    }
    break;
  case 'L':
    if (config().lag_correct == true)
    {
      set_config().lag_correct = false;
      TRACE("Disabling lag correction\n");
    }
    else
    {
      set_config().lag_correct = true;
      TRACE("Enabling lag correction\n");
    }
    break;
  case ' ':
    m_joystick->set_value(5, 1);
    break;
  case 'S':
    m_renderer->do_screenshot();
    break;
  case 'M':
    set_config().do_movie = config().do_movie ? false : true;
    break;
  case 27: // escape
  case 'q':
    exit(0);
    break;
  case 'r':
  {
    m_glider->reset(m_body);
    m_body->calc_new_pos_and_orient(0);
    // and all the pilots...
    set<Pilot *> pilots = Pilot_manager::instance()->get_pilots();
    for (set<Pilot *>::iterator it = pilots.begin() ;
         it != pilots.end() ;
         ++it)
    {
      (*it)->reset(m_body);
    }
    break;
  }
  case 'R':
    m_glider->reset_in_race();
    break;
  case 'a':
  {
    // need to toggle the main glider's joystick (only has an 
    // effect if we have a different joystick to the glider's original)
    Joystick * current_joystick = &m_glider->get_joystick();
    if (current_joystick == m_joystick)
      m_glider->take_control(0);
    else
      m_glider->take_control(m_joystick);
    break;
  }
  case 'o':
  {
    add_robot();
  }
  break;
  case 'O':
  {
    remove_robot();
  }
  break;
  case 'h':
    m_show_help = m_show_help == true ? false : true;
    break;
  case 'f':
    glutFullScreen();
    break;
  case 'w':
    Tracer_collection::instance()->add_tracer(m_glider);
    break;
  case 'W':
    Tracer_collection::instance()->remove_all_tracers();
    break;
  case 'g':
    if (m_eye != m_glider)
      m_eye->set_eye(false); // notify the old eye
    m_eye = m_glider;
    m_eye->set_eye(true); // notify the eye
    break;
  case 'b':
    if (m_eye != m_body)
      m_eye->set_eye(false); // notify the old eye
    m_eye = m_body;
    m_eye->set_eye(true); // notify the eye
    break;
  case 'B':
    m_move_body_mode = !m_move_body_mode;
    break;
  case 't':
    switch (config().text_overlay)
    {
    case Config::TEXT_NONE: 
      set_config().text_overlay = Config::TEXT_LITTLE; break;
    case Config::TEXT_LITTLE: 
      set_config().text_overlay = Config::TEXT_LOTS; break;
    default:
    case Config::TEXT_LOTS: 
      set_config().text_overlay = Config::TEXT_NONE; break;
    }
    Gui::instance()->update_text_overlay();
    break;
  case '=':
    m_renderer->set_fov(sss_min(150.0f, config().fov-2));
    Gui::instance()->update_fov();
    break;
  case '-':
    m_renderer->set_fov(sss_max(5.0f, config().fov+2));
    Gui::instance()->update_fov();
    break;
  case '9':
    set_config().lod /= 1.1f;
    Gui::instance()->update_lod();
    TRACE("LOD = %f\n", config().lod);
    break;
  case '0':
    set_config().lod *= 1.1f;
    Gui::instance()->update_lod();
    TRACE("LOD = %f\n", config().lod);
    break;
  case 'p':
    toggle_paused();
    break;
  case 'c':
    m_allow_config = (m_allow_config == true ? false : true);
    if (m_allow_config == true)
      Gui::instance()->show();
    else
      Gui::instance()->hide();
    break;
  case 'm':
  {
    Config::Control_methods_it it = 
      m_config.control_methods.find("mouse");
    if (it == m_config.control_methods.end())
      TRACE("Cannot find control method: mouse\n");
    else
      m_config.control_method = it->second;
    reset_joystick();
    TRACE("Using mouse for control 0x%x\n", (int) m_config.control_method);
    Gui::instance()->update();
    break;
  }
  case 'j':
  {
    Config::Control_methods_it it = 
      m_config.control_methods.find("joystick");
    if (it == m_config.control_methods.end())
      TRACE("Cannot find control method: joystick\n");
    else
      m_config.control_method = it->second;
    reset_joystick();
    TRACE("Using joystick for control 0x%x\n", (int) m_config.control_method);
    Gui::instance()->update();
    break;
  }
  default:
    TRACE("Unkown key '%c' = %d\n", key, key);
    break;
  }
//  Gui::instance()->update();
  glutPostRedisplay();
  
}

void Sss::special(int key,int x, int y)
{
  if (glutGetModifiers() == GLUT_ACTIVE_SHIFT)
  {
    switch(key)
    {
    case GLUT_KEY_UP:
      m_body->move(Body::TOWARDS, true);
      break;
    case GLUT_KEY_DOWN:
      m_body->move(Body::AWAY, true);
      break;
    case GLUT_KEY_LEFT:
      m_body->move(Body::LEFT, true);
      break;
    case GLUT_KEY_RIGHT:
      m_body->move(Body::RIGHT, true);
      break;
    default:
      TRACE("Unkown special key %d\n", key);
      break;
    }
  }
  else
  {
    // treat as control input
    if (m_config.arrow_keys_for_primary_control)
    {
      switch(key)
      {
      case GLUT_KEY_UP:
        m_joystick->set_value(2, -1);
        break;
      case GLUT_KEY_DOWN:
        m_joystick->set_value(2, 1);
        break;
      case GLUT_KEY_LEFT:
        m_joystick->set_value(1, -1);
        break;
      case GLUT_KEY_RIGHT:
        m_joystick->set_value(1, 1);
        break;
      default:
        TRACE("Unkown special key %d\n", key);
        break;
      }
    }
    else
    {
      switch(key)
      {
      case GLUT_KEY_UP:
        if (m_joystick->get_value(3) < 0.0f)
          m_joystick->set_value(3, 0.0f);
        else
          m_joystick->incr_value(3, 1.0f * last_dt);
//        m_joystick->set_value(3, 1);
        break;
      case GLUT_KEY_DOWN:
        if (m_joystick->get_value(3) > 0.0f)
          m_joystick->set_value(3, 0.0f);
        else
          m_joystick->incr_value(3, -1.0f * last_dt);
//        m_joystick->set_value(3, -1);
        break;
      case GLUT_KEY_LEFT:
        m_joystick->set_value(4, -1);
        break;
      case GLUT_KEY_RIGHT:
        m_joystick->set_value(4, 1);
        break;
      default:
        TRACE("Unkown special key %d\n", key);
        break;
      }
    }
  }
}
void Sss::special_up(int key,int x, int y)
{
  if (glutGetModifiers() == GLUT_ACTIVE_SHIFT)
  {
    switch(key)
    {
    case GLUT_KEY_UP:
      m_body->move(Body::TOWARDS, false);
      break;
    case GLUT_KEY_DOWN:
      m_body->move(Body::AWAY, false);
      break;
    case GLUT_KEY_LEFT:
      m_body->move(Body::LEFT, false);
      break;
    case GLUT_KEY_RIGHT:
      m_body->move(Body::RIGHT, false);
      break;
    default:
      TRACE("Unkown special key %d up\n", key);
      break;
    }
  }
  else
  {
    // treat as control input
    if (m_config.arrow_keys_for_primary_control)
    {
      switch(key)
      {
      case GLUT_KEY_UP:
        m_joystick->set_value(2, 0);
        break;
      case GLUT_KEY_DOWN:
        m_joystick->set_value(2, 0);
        break;
      case GLUT_KEY_LEFT:
        m_joystick->set_value(1, 0);
        break;
      case GLUT_KEY_RIGHT:
        m_joystick->set_value(1, 0);
        break;
      default:
        TRACE("Unkown special key %d up\n", key);
        break;
      }
    }
    else
    {
      switch(key)
      {
      case GLUT_KEY_UP:
//        m_joystick->set_value(3, 0);
        break;
      case GLUT_KEY_DOWN:
//        m_joystick->set_value(3, 0);
        break;
      case GLUT_KEY_LEFT:
        m_joystick->set_value(4, 0);
        break;
      case GLUT_KEY_RIGHT:
        m_joystick->set_value(4, 0);
        break;
      default:
        TRACE("Unkown special key %d up\n", key);
        break;
      }
    }
  }
}

void Sss::toggle_paused()
{
  TRACE_METHOD_ONLY(1);
  m_glider->toggle_paused();
  
  m_sss_state = 
    (m_sss_state == SSS_FLYING ? SSS_PAUSED : SSS_UNPAUSED);
  
  glutPostRedisplay();
}

void Sss::reset_joystick()
{
  m_joystick->zero_all_channels();
  m_joystick->set_value(3, -1.0f);
}

//! gets called by Gui itself
void Sss::hide_config()
{
  Gui::instance()->hide();
  m_allow_config = false;
}

// The static functions registered with glut

void Sss::_idle() 
{
  m_instance->idle();
}
void Sss::_display() 
{
  m_instance->display();
}
void Sss::_mouse(int button, int state, int x, int y)
{
  m_instance->mouse(button, state, x, y);
}
void Sss::_motion(int x, int y) 
{
  m_instance->motion(x, y);
}
void Sss::_reshape(int w, int h) 
{
  m_instance->reshape(w, h);
}
void Sss::_key(unsigned char key, int x, int y) 
{
  m_instance->key(key, x, y);
}
void Sss::_special(int key, int x, int y) 
{
  m_instance->special(key, x, y);
}
void Sss::_special_up(int key, int x, int y) 
{
  m_instance->special_up(key, x, y);
}

