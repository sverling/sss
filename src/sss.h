/*
Sss - a slope soaring simulater.
Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

Sss provides the core of the flight simulator.

It is passed the objects that are always required:

Glider - the thing that flies
Joystick - the thing that links the user to the glider (and to the body?)
Environment - where you fly
Body - the lump of meat on the ground
Config - controls the overall configuration

It also maintains an "eye" - a pointer to the object that is acting as the 
eye or camera.

Originally this was meant to be just a repository of objects, with no real 
intelligence about how they interact, and the main interface to the GLUT
callbacks (though using GLUI means that that role is subverted) and the user
through keyboard input etc. It has grown beyond that now, and is not very elegant.

*/

#ifndef SSS_MAIN_H
#define SSS_MAIN_H

#include "sss_assert.h"
#include "types.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

class Object;
class Glider;
class Environment;
class Body;
class Pilot;
class Config;
class Joystick;
class Renderer;
class Text_overlay;
class jsJoystick;
class Heli_controller;

//! The main object controlling the simulation
class Sss
{
public:

  //! Initialise the Sss state. 
  void initialise(Glider * glider,
    Environment * environment,
    Body * body,
    Joystick * joystick);

  //! Sss is a singleton - use this in creation
  static Sss * create_instance(Config & config);
  //! Sss is a singleton - use this to just get the instance
  inline static Sss * instance();
  //! Indicates if the instance is created yet
  static bool is_instance_available();
  //! In principle deletion is allowed(?)
  ~Sss();

  //! enter the main loop
  void start(); 

  enum Sss_state { SSS_FLYING, SSS_PAUSED, SSS_UNPAUSED };
  Sss_state sss_state() const {return m_sss_state;}
  void toggle_paused();
  void hide_config();

  //! Used to get the config for read-only access
  const Config & config() const {return m_config;}
  //! Used to get the config for write-access
  Config & set_config() const {return m_config;}

  const Joystick & joystick() const {return *m_joystick;}
  const Object & eye() const {return *m_eye;}
  const Body & body() const {return *m_body;}
  Glider & glider() const {return *m_glider;}
  void set_eye(Object * eye) {m_eye = eye;}

  //! Returns the time of last update - arbitrary offset
  float last_update_time() const {return m_last_update_time;}

  //! Returns the current time (in seconds) - arbitrary offset
  float get_seconds() const;

  //! Returns the current time (in millisec) - arbitrary offset and maybe rounded
  int get_milliseconds() const;

  //! Add an object to the simulation
  void add_object(Object * new_object);
  //! Remove an object from the simulation
  int remove_object(const Object * object);

  //! Add a robot
  void add_robot();
  //! Removes the first robot found
  void remove_robot();

  //! called by Gui
  void set_mouse_or_joystick();

  /// reset the "joystick" so that channels that are not used get set to zero
  void reset_joystick();

  //! Called by Remote_sss_iface
  static void recv_text_msg(const char * text);

  // fns to be called by GLUT
  static void _idle();
  static void _display();
  static void _mouse(int button, int state, int x, int y);
  static void _motion(int x, int y);
  static void _reshape(int w, int h);
  static void _key(unsigned char key, int x, int y);
  static void _keyUp(unsigned char key, int x, int y);
  static void _special(int key, int x, int y);
  static void _special_up(int key, int x, int y);

private:
  Sss(Config & config);

  // GLUT fns called indirectly
  void idle();
  void display(void);
  void mouse(int button, int state, int x, int y);
  void motion(int x, int y);
  void key(unsigned char key, int px, int py);
  void keyUp(unsigned char key, int px, int py);
  void special(int key,int x, int y);
  void special_up(int key,int x, int y);
  void reshape(int w, int h);

  // helper fns

  void initialise_gl();
  void perform_time_step(float dt);
  bool add_letter_to_text(unsigned char key);
  void send_text_message();
  void do_joystick();
  void do_tx_audio();

  // move the body on terrain in response to a mouse click
  void move_body(int x, int y);

  // objects passed in
  Config & m_config;
  Glider *        m_glider;
  Environment *   m_environment;
  Body *          m_body;

  // eye points to either glider or body
  Object * m_eye;

  /// objects owned by this
  Joystick * m_joystick; /// general joystick

  /// plib joystick
  jsJoystick * m_js;

  vector<Object *> m_object_list;
  Renderer * m_renderer;
  Text_overlay * m_text_overlay;

  Sss_state m_sss_state;
  bool m_allow_config;

  static Sss * m_instance;

  // in seconds
  float m_last_update_time;

  string m_local_text;
  string m_remote_text;
  bool m_enter_text_mode;
  float m_remote_text_timer;

  bool m_show_help;

  /// next click sets the body position
  bool m_move_body_mode;

  bool m_LookAround;
  int m_StartX;
  int m_StartY;
  bool m_TranslateMode;
  bool m_ElevationMode;
  bool m_RotateMode;
  bool m_ZoomMode;
  Position m_Center;
  float m_Theta;
  float m_Phi;
  float m_Radius;
  Object* m_OldCamera;
  Object* m_Camera;

  map<Pilot *, int> m_robot_offsets;
  typedef map<Pilot *, int>::iterator Robot_offset_it;

  /// Special controller for the human-controller helicopter - 
  /// only non-zero if in use
  Heli_controller *m_heli_controller;
};

inline Sss * Sss::instance()
{
  assert1(0 != m_instance);
  return m_instance;
}

inline bool Sss::is_instance_available()
{
  return m_instance != 0;
}
#endif

