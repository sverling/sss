/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  Renderer deals with rendering details, and window management

  \file renderer.cpp
*/

#include "renderer.h"
#include "sss.h"
#include "object.h"
#include "config.h"
#include "gui.h"
#include "environment.h"
#include "text_overlay.h"
#include "texture.h"
#include "particle_engine.h"

#include <algorithm>
using namespace std;

Renderer * Renderer::m_instance = 0;

//===========================================================
// Code to write out screen shots
//==========================================================
static void flipVertical(unsigned char *data, int w, int h)
{
  int x, y, i1, i2;
  unsigned char temp;
  for (x=0;x<w;x++){
    for (y=0;y<h/2;y++){
      i1 = (y*w + x)*3; // this pixel
      i2 = ((h - y - 1)*w + x)*3; // its opposite (across x-axis)

      // swap pixels
      temp = data[i1];
      data[i1] = data[i2];
      data[i2] = temp;

      i1++; i2++;
      temp = data[i1];
      data[i1] = data[i2];
      data[i2] = temp;

      i1++; i2++;
      temp = data[i1];
      data[i1] = data[i2];
      data[i2] = temp;

    }
  }
}


/* write the current frame buffer to the file 'filename' */
static void writeFrameBuffer(char *filename){
  FILE *fp = fopen(filename, "wb");
  int frame_width = glutGet(GLUT_WINDOW_WIDTH);
  int frame_height = glutGet(GLUT_WINDOW_HEIGHT);
  
  int data_size = frame_width * frame_height * 3;
  unsigned char *framebuffer = 
    (unsigned char *) malloc(data_size * sizeof(unsigned char));
  glReadPixels(0, 0, frame_width, frame_height, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
  flipVertical(framebuffer, frame_width, frame_height);

  fprintf(fp, "P6\n%d %d\n%d\n", frame_width, frame_height, 255);
  fwrite(framebuffer, data_size, 1, fp);
  fclose(fp);
  free(framebuffer);
}

static inline void do_screenshot()
{
  static int count = 0;
  static char screen_file[] = "sss-00000.ppm";
  sprintf(screen_file, "sss-%05d.ppm", count++);
//  printf("%s\n", movie_file);
  writeFrameBuffer(screen_file);
}

// writes out a screen shot at a rate governed by dt
static inline void do_movie(float dt)
{
  static float last_time = -1.0f;
  float now = Sss::instance()->get_seconds();
  
  if ((now - last_time) < dt)
    return;
  
  do_screenshot();
}

//=========================================================================

Renderer::Renderer()
  :
  m_quadric(0),
  m_fov_zoom(1.0),
  m_do_screenshot(false)
{
  TRACE_METHOD_ONLY(1);
  initialise_gl();
  
  setup_projection();
}

void Renderer::initialise_gl(void)
{
  TRACE_METHOD_ONLY(1);
  
//  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH/* | GLUT_ACCUM*/);

  // we really want to try to get 16 bits or more for the depth buffer, but
  // if the graphics card doesn't support that we can't insist on it.

  // use default?
  if (Sss::instance()->config().depth_bits < 8)
  {
    TRACE("Using GLUT defaults for visual\n");
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  }
  else
  {
    char vis_string[64];
    sprintf(vis_string, "rgb double depth>=%d", 
            Sss::instance()->config().depth_bits);
    
    TRACE("trying with visual string \"%s\"\n", vis_string);
    glutInitDisplayString(vis_string);
    TRACE("checking result...\n");
    
    if (glutGet(GLUT_DISPLAY_MODE_POSSIBLE) == 0)
    {
      TRACE("Cannot get requested visual - using a reasonable one (depth>=12)\n");
      glutInitDisplayString("rgb double depth>=12");
// calling glutInitDisplayMode again breaks win32 glut
//    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
      if (glutGet(GLUT_DISPLAY_MODE_POSSIBLE) == 0)
      {
        assert(!"Unable to get suitable visual!");
      }
    }
  }
  
  TRACE("Creating window ");

  glutInitWindowSize(Sss::instance()->config().window_x,
                     Sss::instance()->config().window_y);
  TRACE(" ... ");
  extern const char * program_name;
  int main_window = glutCreateWindow(
    program_name);
  TRACE(" created...");

  int depth_size = glutGet(GLUT_WINDOW_DEPTH_SIZE);
  TRACE(" with depth size = %d\n", depth_size);
  // use default near/far settings?
  if ( (Sss::instance()->config().clip_near < 0.0f) ||
       (Sss::instance()->config().clip_far < 0.0f) )
  {
    if (depth_size > 16)
    {
      Sss::instance()->set_config().clip_near = 0.07f;
      Sss::instance()->set_config().clip_far = 14000.0f;
    }
    else
    {
      Sss::instance()->set_config().clip_near = 1.0f;
      Sss::instance()->set_config().clip_far = 14000.0f;
    }
  }

  // set pointer
  glutSetCursor(GLUT_CURSOR_CROSSHAIR);

  TRACE_FILE_IF(2)
    TRACE("Creating GUI (hidden)\n");
  Gui::create_instance(main_window)->hide();
  
  TRACE_FILE_IF(2)
    TRACE("Initialising multi-texture\n");
  initMultitexture();

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
//  glClear(GL_ACCUM_BUFFER_BIT);

  recalculate_quadric();
  if (true == Sss::instance()->config().fullscreen)
  {
    glutFullScreen();
  }
  TRACE_FILE_IF(2)
    TRACE("Finished initialising OpenGL\n");
}

void Renderer::recalculate_quadric()
{
  if (m_quadric)
    gluDeleteQuadric(m_quadric);
  m_quadric = gluNewQuadric();
  assert1(m_quadric);
  gluQuadricNormals(m_quadric, Sss::instance()->config().shade_model);
  gluQuadricDrawStyle(m_quadric, GLU_FILL);

  glShadeModel(Sss::instance()->config().shade_model);
}

void Renderer::setup_projection()
{
  TRACE_METHOD_ONLY(3);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( 
    Sss::instance()->config().fov / m_fov_zoom, /* field of view in degree */ 
    (float) Sss::instance()->config().window_x/ /* aspect ratio */ 
    Sss::instance()->config().window_y,
    /* near */ Sss::instance()->config().clip_near, 
    /* far  */ Sss::instance()->config().clip_far);
  glMatrixMode(GL_MODELVIEW);
}
  
void Renderer::reshape(int w, int h)
{
  Sss::instance()->set_config().window_x = w;
  Sss::instance()->set_config().window_y = h;
  glViewport(0, 0, w, h);
  Gui::instance()->auto_set_viewport();
  setup_projection();
}

void Renderer::add_text_overlay(Text_overlay * overlay)
{
  m_text_overlay_list.push_back(overlay);
}

//! \todo implement remove_text_overlay
int Renderer::remove_text_overlay(Text_overlay * overlay)
{
  m_text_overlay_list.erase(find(m_text_overlay_list.begin(),
                                 m_text_overlay_list.end(),
                                 overlay));
  return 0;
}

void Renderer::apply_whitewash(float alpha)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  
  // output a white quad
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1, 1, 1, alpha);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(1, 0);
  glVertex2f(1, 1);
  glVertex2f(0, 1);
  glEnd();
  
  // undo
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
}

void Renderer::render_objects(
  const vector<Object *> & object_list,
  const Object * eye,
  Environment * environment,
  Object * shadow_maker,
  bool dim_non_text)
{
  TRACE_METHOD_ONLY(3);
  unsigned int i;

  // do shadow first, as it will trash the buffer
  environment->calculate_shadow(shadow_maker);

  setup_view_matrices(eye);
  
  if (Sss::instance()->config().fog == true)
  {
    glHint(GL_FOG_HINT, GL_NICEST);
    static GLfloat fog_color_air[4] = {0.8, 0.8, 0.85, 1.0};
    static GLfloat fog_color_water[4] = {0.2, 0.4, 0.5, 1.0};
    glEnable(GL_FOG);
    if (eye->get_eye()[2] > environment->get_sea_altitude())
    {
//      glFogi(GL_FOG_MODE, GL_EXP);
      glFogi(GL_FOG_MODE, GL_LINEAR);
      glFogfv(GL_FOG_COLOR, fog_color_air);
//      glFogf(GL_FOG_DENSITY, 0.0003);
      const float d_far = Sss::instance()->config().clip_far*1.0;
      const float d_near = d_far * 0.4;
      glFogf(GL_FOG_START, d_near);
      glFogf(GL_FOG_END, d_far);
    }
    else
    {
      // under water
      glFogi(GL_FOG_MODE, GL_EXP);
      glFogfv(GL_FOG_COLOR, fog_color_water);
      glFogf(GL_FOG_DENSITY, 0.01f);
//        glFogf(GL_FOG_START, 500.0);
//        glFogf(GL_FOG_END, Sss::instance()->config().clip_far*0.95);
    }
  }
  else
  {
    glDisable(GL_FOG);
  }
  
  glClearColor(0.f,0.f,0.f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // setup lighting
  environment->setup_lighting();
  
  glDisable(GL_LIGHTING);
  //  glDisable(GL_DEPTH_TEST);
//    if (sss_options.fog == true)
//      glEnable(GL_FOG);
//    else
  glDisable(GL_FOG);

  glPushMatrix();
  environment->draw_sky();
  glPopMatrix();
  
  glPushMatrix();
  environment->draw_ground(shadow_maker);
  glPopMatrix();
  check_errors("after ground");
  
  // draw real objects
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  static GLfloat mat_specular[] =  {0.7,0.7,0.7,1.0};
  static GLfloat shininess[] = { 30.0f };
  glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  static GLfloat mat_diffuse[]=  {0.4,0.4,0.4,1.0};
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glEnable(GL_COLOR_MATERIAL);
  if (Sss::instance()->config().fog == true)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);
  
  for (i = 0 ; i < object_list.size() ; ++i)
  {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();
    object_list[i]->draw(Object::NORMAL);
    glPopMatrix();
    glPopAttrib();
  }
  
  // the thermals
  if (Sss::instance()->config().thermal_show_type != Config::NONE)
  {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();
    environment->draw_thermals();
    glPopMatrix();
    glPopAttrib();
  }

  // The particle engine
  Particle_engine::instance()->draw_particles(eye);

  // Now the overlays
  glLoadIdentity();

  glPushMatrix();
  environment->draw_lens_flare();
  glPopMatrix();
  
  if (dim_non_text)
  {
    apply_whitewash(0.7f);
  }

  for (i = 0 ; i < m_text_overlay_list.size() ; ++i)
  {
    m_text_overlay_list[i]->display();
    m_text_overlay_list[i]->reset();
  }

//  glAccum(GL_MULT, 0.5f);
//  glAccum(GL_ACCUM, 0.5f);
//  glAccum(GL_RETURN, 1.0f);

  glutSwapBuffers();

  // Hopefully by this stage the previous render will have been
  // flushed...
  if (Sss::instance()->config().do_movie)
    do_movie(Sss::instance()->config().movie_dt);
  else if (m_do_screenshot)
  {
    ::do_screenshot();
    m_do_screenshot = false;
  }
  
  check_errors("end of render_objects");
}

/*!  Sets the value in config, sets up the projection matrix, and
  posts a redisplay */
void Renderer::set_fov(float fov)
{
  Sss::instance()->set_config().fov = fov;
  setup_projection();
  glutPostRedisplay();
}

void Renderer::set_fov_zoom(float zoom)
{
  if (m_fov_zoom != zoom)
  {
    m_fov_zoom = zoom;
    setup_projection();
  }
}
  

/*!  Sets the value in config, sets up the projection matrix, and
  posts a redisplay */
void Renderer::set_clip_near(float clip_near)
{
  Sss::instance()->set_config().clip_near = clip_near;
  setup_projection();
  glutPostRedisplay();
}
/*!  Sets the value in config, sets up the projection matrix, and
  posts a redisplay */
void Renderer::set_clip_far(float clip_far)
{
  Sss::instance()->set_config().clip_far = clip_far;
  setup_projection();
  glutPostRedisplay();
}

void Renderer::setup_view_matrices(const Object * eye)
{
  // If necessary tweak the projection for the auto-zoom
  if (Sss::instance()->config().auto_zoom == true)
  {
    float dist = (eye->get_eye_target() - eye->get_eye()).mag();
    float zoom = 1.0 + dist/Sss::instance()->config().zoom_x2_dist;
    set_fov_zoom(zoom);
  }
  else
  {
    set_fov_zoom(1.0);
  }
  

  glMatrixMode(GL_MODELVIEW);
  
  glLoadIdentity();
  
  // from is easy
  const Vector3 from = eye->get_eye();
  // for to, move from pos along the x' direction
  const Vector3 to = eye->get_eye_target();
  // for up, just use the k' direction
  const Vector3 up = eye->get_eye_up();
  
  gluLookAt(from[0], from[1], from[2],
            to[0], to[1], to[2],
            up[0], up[1], up[2]);
  
  
}

