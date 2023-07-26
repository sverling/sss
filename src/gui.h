/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_GUI_H
#define SSS_GUI_H

#ifdef WITH_GLUI
#include "glui.h"
#endif

#include "sss_glut.h"

#ifndef GLUTCALLBACK
#define GLUTCALLBACK
#endif

/// This class does everything to do with the configuration GUI. It
/// is a singleton.
class Gui
{
public:
  static Gui * instance() {return m_inst;}
  static Gui * create_instance(int main_window);
  
  void set_idle_func(void (GLUTCALLBACK *func)(void));
  void set_reshape_func(void (GLUTCALLBACK *func)(int width, int height));
  void set_keyboard_func(void (GLUTCALLBACK *func)
                         (unsigned char key, int x, int y));
  void set_keyboard_up_func(void (GLUTCALLBACK *func)
                            (unsigned char key, int x, int y));
  void set_mouse_func(void (GLUTCALLBACK *func)
                      (int button, int state, int x, int y));
  void set_special_func(void (GLUTCALLBACK *func)(int key, int x, int y));
  void set_special_up_func(void (GLUTCALLBACK *func)(int key, int x, int y));

  //! updates all fields (slow)
  void update();
  //! just update the fov field
  void update_fov();
  //! just update the lod field
  void update_lod();
  //! just update the text overlay field
  void update_text_overlay();

  void hide();
  void show();
  
  void auto_set_viewport() {
#ifdef WITH_GLUI
    GLUI_Master.auto_set_viewport();
#endif
  }

  void get_viewport_area(int * x, int * y, int * w, int * h);

private:
  Gui(int main_window);
  ~Gui();
  
  static void idle(void);
  static void action(int control);
#ifdef WITH_GLUI
  GLUI * m_glui;
  
  GLUI_Checkbox * fog_checkbox;
  GLUI_Checkbox * particle_checkbox;
  GLUI_Checkbox * shadow_checkbox;
  GLUI_Checkbox * clip_checkbox;
  GLUI_Checkbox * turbulence_checkbox;
  GLUI_Checkbox * translucent_sea_checkbox;
  GLUI_Checkbox * thermal_show_all_checkbox;
  GLUI_Checkbox * auto_zoom_checkbox;
  GLUI_Checkbox * use_audio_checkbox;
  GLUI_Checkbox * use_tx_audio_checkbox;
  GLUI_Checkbox * arrow_keys_for_primary_control_checkbox;
  
  GLUI_Spinner * lod_spinner;
  GLUI_Spinner * audio_spinner;
  GLUI_Spinner * fov_spinner;
  GLUI_Spinner * wind_scale_spinner;
  GLUI_Spinner * wind_dir_spinner;
  GLUI_Spinner * gravity_spinner;
  GLUI_Spinner * turbulence_scale_spinner;
  GLUI_Spinner * turbulence_shear_offset_spinner;
  GLUI_Spinner * zoom_x2_dist_spinner;
  GLUI_Spinner * dynamic_cloud_layers_spinner;

  GLUI_EditText * target_frame_rate_text;
  GLUI_EditText * clip_near_text;
  GLUI_EditText * clip_far_text;
  GLUI_EditText * physics_freq_text;
  GLUI_EditText * body_view_lag_time_text;
  GLUI_EditText * glider_view_lag_time_text;

  GLUI_EditText * thermal_density_text;
  GLUI_EditText * thermal_updraft_text;
  GLUI_EditText * thermal_radius_text;
  GLUI_EditText * thermal_height_text;
  GLUI_EditText * thermal_inflow_height_text;
  GLUI_EditText * thermal_lifetime_text;
  
  GLUI_Listbox * texture_listbox;
  GLUI_Listbox * shade_listbox;
  GLUI_Listbox * text_overlay_listbox;
  GLUI_Listbox * control_method_listbox;
  GLUI_Listbox * thermal_show_type_listbox;
  GLUI_Listbox * glider_file_listbox;
  GLUI_Listbox * physics_mode_listbox;
  GLUI_Listbox * sky_mode_listbox;
  GLUI_Listbox * timer_method_listbox;
#endif
  int m_main_window;
  bool m_visible;
  static Gui * m_inst;
  static void (GLUTCALLBACK *idle_func)(void);
};



#endif






