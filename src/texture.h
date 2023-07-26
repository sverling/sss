/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include "matrix_vector3.h"
#include "array_2d.h"

#include <stdio.h>
#include "sss_glut.h"
#ifdef WITH_GL_EXT
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#endif

#include <string>

/// @file texture.h Lots of room for improving this texture code...

/// Texture based on the array passed in at creation, which should
/// have values in the range 0-1 (converted to grey.
class Grey_texture
{
public:
  enum Edge {CLAMP, CLAMP_TO_EDGE, REPEAT};
  Grey_texture(const Array_2D<float> & image,
               const Array_2D<float> & alpha,
               Edge edge);
  /// 0 is returned if the texture couldn't be generated
  GLuint get_low_texture() const {return m_texture_low;}
  GLuint get_high_texture() const {return m_texture_high;}
  int get_w() const {return m_w;}
  int get_h() const {return m_h;}
private:
  int m_w, m_h;
  GLuint m_texture_low, m_texture_high; // two resolutions  
};

class Image_texture
{
public:
  enum Edge {CLAMP, CLAMP_TO_EDGE, REPEAT};
  Image_texture(const Array_2D<float> & red,
                const Array_2D<float> & green,
                const Array_2D<float> & blue,
                const Array_2D<float> & alpha,
                Edge edge);
  /// 0 is returned if the texture couldn't be generated
  GLuint get_low_texture() const {return m_texture_low;}
  GLuint get_high_texture() const {return m_texture_high;}
  int get_w() const {return m_w;}
  int get_h() const {return m_h;}
private:
  int m_w, m_h;
  GLuint m_texture_low, m_texture_high; // two resolutions  
};

/// Texture read in from file - provides a high and low quality texture,
/// The ability to set the repeat/clamp modes, and the ability to
/// transform the original RGBA image
class Rgba_file_texture
{
public:
  enum Edge {CLAMP, CLAMP_TO_EDGE, REPEAT};
  enum Type {RGB, RGBA, LUM}; // Use the original image, or convert it?
	enum Generate {GENERATE_HIGH = 1, GENERATE_LOW = 2, GENERATE_ALL = GENERATE_HIGH | GENERATE_LOW};
  Rgba_file_texture(const std::string & file, 
                    Edge edge,
                    Type type = RGBA,
										Generate generate = GENERATE_ALL);
  ~Rgba_file_texture();
  /// 0 is returned if the texture couldn't be generated
  GLuint get_low_texture() const {return m_texture_low;}
  GLuint get_high_texture() const {return m_texture_high;}
  int get_w() const {return m_w;}
  int get_h() const {return m_h;}
private:
  int m_w, m_h;
  GLuint m_texture_low, m_texture_high; // two resolutions
};

/// Special texture - intended for lightmaps (luminance)
class Lightmap_texture
{
public:
  Lightmap_texture(int w, int h, const float * image);
  static unsigned calc_index(int i, int j, int w, int /*h*/) {return i + w * j;}
  GLuint get_texture() const {return m_texture;}
  int get_w() const {return m_w;}
  int get_h() const {return m_h;}
private:
  const int m_w, m_h;
  GLuint m_texture; // one resolution
};

/// Special empty texture - intended for writing to for shadows etc.
class Shadow_texture
{
public:
  Shadow_texture(int w, int h, float range);
  
  const unsigned char * const get_image();
  GLuint get_texture() const {return texture;}
  int get_w() const {return w;}
  int get_h() const {return h;}
private:
  const int w, h;
  GLubyte * image;
  
  GLuint texture; // one resolution
};

/// Special texture for a flare element...
class Flare_texture
{
public:
  Flare_texture();
  
  GLuint get_texture() const {return texture;}
  int get_w() const {return w;}
  int get_h() const {return h;}
private:
  void draw_line(float x0, float y0, float x1, float y1, 
                 unsigned char col, unsigned char alpha);
  void draw_poly(float radius, int num, 
                 unsigned char col, unsigned char alpha);
  
  int w, h;
  GLubyte * image; // only valid during initialisation
  GLuint texture; 
};

/// Special texture for a flare element...
class Smoke_texture
{
public:
  Smoke_texture(int w, int h);
  
  GLuint get_texture() const {return texture;}
private:
  GLuint texture; 
};

/// Special texture for a "detail" texture
class Detail_texture
{
public:
  Detail_texture(int w, int h,
                 float r, // noisiness
                 float l_min,
                 float l_max,
                 Vector3 light_dir); ///< direction towards the light
  
  GLuint get_high_texture() const {return m_texture_high;}
  GLuint get_low_texture() const {return m_texture_low;}
private:
  GLuint m_texture_high, m_texture_low; 
};

void seed_rand(int seed);

void calculate_midpoint_displacement(Array_2D<float> & array,
                                     float r,
                                     float min_z, 
                                     float max_z);


// Stuff for multi-texturing
//#define EXT_INFO	             // Do You Want To See Your Extensions At Start-Up?
#define MAX_EXTENSION_SPACE 10240  // Characters for Extension-Strings
#define MAX_EXTENSION_LENGTH 256   // Maximum Of Characters In One Extension-String
extern bool multitextureSupported; // Flag Indicating Whether Multitexturing Is Supported
extern GLint maxTexelUnits;        // Number Of Texel-Pipelines. This Is At Least 1.

// isMultitextureSupported() Checks At Run-Time If Multitexturing Is Supported
// must be called after a context has been created
bool initMultitexture(void);

class Gl_version
{
public:
  Gl_version(int ma, int mi, int re) 
    : m_major(ma), m_minor(mi), m_release(re) {}
  // initialise from string in form 1.2.3 or 1.2
  void initialise(const char *);
  void show() const;
  bool operator<(const Gl_version & rhs) const;
  bool operator>(const Gl_version & rhs) const;
  bool operator==(const Gl_version & rhs) const;
  bool operator<=(const Gl_version & rhs) const;
  bool operator>=(const Gl_version & rhs) const;
  int m_major;
  int m_minor;
  int m_release;
};

extern Gl_version gl_version; // initially 0, 0, 0

#endif
