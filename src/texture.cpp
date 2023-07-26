
/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file texture.cpp

  \todo improve this texture implementation!
*/
#include "texture.h"
#include "image_from_file.h"
#include "misc.h"
#include "log_trace.h"
#include "sss_assert.h"
#include "array_2d.h"
#include "matrix_vector3.h"

//#include <stdlib.h>
#include <string.h>
#include "sss_glut.h"
#include <math.h>

#include <vector>
using namespace std;

Gl_version gl_version(0, 0, 0);

// stuff for multi-texturing
bool multitextureSupported=false; // Flag Indicating Whether Multitexturing Is Supported
GLint maxTexelUnits=1;            // Number Of Texel-Pipelines. This Is At Least 1.

Grey_texture::Grey_texture(const Array_2D<float> & image,
                           const Array_2D<float> & alpha,
                           Edge edge)
{
  m_w = image.get_nx();
  m_h = image.get_nx();
  assert1(m_w == m_h);
  
  GLint gl_edge = GL_CLAMP;

  switch (edge)
  {
  case CLAMP: gl_edge = GL_CLAMP; break;
  case CLAMP_TO_EDGE: 
    gl_edge = (gl_version >= Gl_version(1, 2, 0)) ? GL_CLAMP_TO_EDGE : GL_CLAMP;
    break;
  case REPEAT: gl_edge = GL_REPEAT; break;
  };

  TRACE("Grey texture: %d %d\n", m_w, m_h);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  
  GLfloat * img = new GLfloat[m_w*m_h*4];
  for (int i = 0 ; i < m_h ; i++)
  {
    for (int j = 0 ; j < m_w ; j++)
    {
      img[(j*m_w + i)*4 + 0] = image(i, j);
      img[(j*m_w + i)*4 + 1] = image(i, j);
      img[(j*m_w + i)*4 + 2] = image(i, j);
      img[(j*m_w + i)*4 + 3] = alpha(i, j);
    }
  }
  
  glGenTextures(1,&m_texture_low);
  glBindTexture(GL_TEXTURE_2D, m_texture_low);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,m_w,m_h,GL_RGBA,GL_FLOAT, img);
  
  glGenTextures(1,&m_texture_high);
  glBindTexture(GL_TEXTURE_2D, m_texture_high);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,m_w,m_h,GL_RGBA,GL_FLOAT, img);

  delete [] img;
}

Image_texture::Image_texture(const Array_2D<float> & red,
                             const Array_2D<float> & green,
                             const Array_2D<float> & blue,
                             const Array_2D<float> & alpha,
                             Edge edge)
{
  m_w = red.get_nx();
  m_h = red.get_nx();

  GLint gl_edge = GL_CLAMP;

  switch (edge)
  {
  case CLAMP: gl_edge = GL_CLAMP; break;
  case CLAMP_TO_EDGE: 
    gl_edge = (gl_version >= Gl_version(1, 2, 0)) ? GL_CLAMP_TO_EDGE : GL_CLAMP;
    break;
  case REPEAT: gl_edge = GL_REPEAT; break;
  };
  
  TRACE("Image texture: %d %d\n", m_w, m_h);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  
  GLfloat * img = new GLfloat[m_w*m_h*4];
  for (int i = 0 ; i < m_h ; i++)
  {
    for (int j = 0 ; j < m_w ; j++)
    {
      img[(j*m_w + i)*4 + 0] = red(i, j);
      img[(j*m_w + i)*4 + 1] = green(i, j);
      img[(j*m_w + i)*4 + 2] = blue(i, j);
      img[(j*m_w + i)*4 + 3] = alpha(i, j);
    }
  }
  
  glGenTextures(1,&m_texture_low);
  glBindTexture(GL_TEXTURE_2D, m_texture_low);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,m_w,m_h,GL_RGBA,GL_FLOAT, img);
  
  glGenTextures(1,&m_texture_high);
  glBindTexture(GL_TEXTURE_2D, m_texture_high);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,m_w,m_h,GL_RGBA,GL_FLOAT, img);

  delete [] img;
}

Rgba_file_texture::Rgba_file_texture(const string & file, 
                                     Edge edge,
                                     Type type,
                                     Generate generate)
{
  string file_name = "textures/" + file;
  TRACE("Reading %s ", file_name.c_str());
  GLubyte * image = read_image(file_name.c_str(), m_w, m_h);
  if (image == 0)
  {
    TRACE("Error reading %s\n", file_name.c_str());
    m_texture_low = 0;
    m_texture_high = 0;
    m_w = m_h = 0;
    return;
  }

  TRACE("%d %d\n", m_w, m_h);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  
  GLint gl_edge = GL_CLAMP;
  GLenum gl_type = GL_RGBA;
  GLenum gl_file_type = GL_RGBA;

  switch (edge)
  {
  case CLAMP: gl_edge = GL_CLAMP; break;
  case CLAMP_TO_EDGE: 
    gl_edge = (gl_version >= Gl_version(1, 2, 0)) ? GL_CLAMP_TO_EDGE : GL_CLAMP;
    break;
  case REPEAT: gl_edge = GL_REPEAT; break;
  };

  switch (type)
  {
  case RGB: 
    gl_type = GL_RGB;
    gl_file_type = GL_RGBA;
    break;
  case RGBA: 
    gl_type = GL_RGBA;
    gl_file_type = GL_RGBA;
    break;
  case LUM:
  {
    // detail is just a "light map" - use the intensity of the original image
    GLubyte * light = new GLubyte[m_w*m_h];
    for (int i = 0 ; i < (m_w*m_h) ; i++)
    {
      light[i] = (image[i*4 + 0] + image[i*4 + 1] + image[i*4 + 2]) / 3;
    }
    delete [] image;
    image = light;
    gl_type = GL_LUMINANCE;
    gl_file_type = GL_LUMINANCE;
    break;
  }
  default:
    TRACE("Unhandled texture modifier type: %d\n", type);
  }

  if (generate & GENERATE_LOW)
  {
    glGenTextures(1,&m_texture_low);
    glBindTexture(GL_TEXTURE_2D, m_texture_low);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
    
    gluBuild2DMipmaps(GL_TEXTURE_2D,gl_type,m_w,m_h,gl_file_type,GL_UNSIGNED_BYTE, image);
  }
  else
  {
    m_texture_low = 0;
  }

  if (generate & GENERATE_HIGH)
  {
    glGenTextures(1,&m_texture_high);
    glBindTexture(GL_TEXTURE_2D, m_texture_high);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
    
    gluBuild2DMipmaps(GL_TEXTURE_2D,gl_type,m_w,m_h,gl_file_type,GL_UNSIGNED_BYTE, image);
  }
  else
  {
    m_texture_high = 0;
  }
  delete [] image;
}

Rgba_file_texture::~Rgba_file_texture()
{
  glDeleteTextures(1, &m_texture_low);
  glDeleteTextures(1, &m_texture_high);
}

Lightmap_texture::Lightmap_texture(int w, int h, const float * image)
  :
  m_w(w), m_h(h)
{
  GLubyte * light = new GLubyte[m_w*m_h];
  for (int i = 0 ; i < (m_w*m_h) ; ++i)
  {
    // Caller must make sure 0 < image < 1.0
    light[i] = (GLubyte) (255 * image[i]);
  }

  GLint gl_edge = GL_CLAMP;
  if (gl_version >= Gl_version(1, 2, 0))
    gl_edge = GL_CLAMP_TO_EDGE;

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, m_w, m_h, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, light);
  delete [] light;
}

Shadow_texture::Shadow_texture(const int w_, const int h_, float range) 
  : 
  w(w_), h(h_)
{
  /*
    The shadow texture will be set up so that the blocker (0,0) location
    is at (0.5,0.5) in texture coords.
    The scaling is determined by range - basically range should be slightly 
    more than the maximum displacement from (0,0) of any part of the blocker.
    I.e. (range, range) maps onto (0,1) (front-left, viewed from above)
    For the initial go, range just determines the radius
  */
  TRACE("Initialising shadow texture ");
  
  image = new GLubyte[h*w*3];
  TRACE("%d %d\n", w, h);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  int i,j;
  for (i = 0 ; i < h ; ++i)
  {
    for (j = 0 ; j < w ; ++j)
    {
      image[i*h*3 + j*3 + 0] = 255;
      image[i*h*3 + j*3 + 1] = 255;
      image[i*h*3 + j*3 + 2] = 255;
    }
  }
  GLint gl_edge = GL_CLAMP;
  if (gl_version >= Gl_version(1, 2, 0))
    gl_edge = GL_CLAMP_TO_EDGE;

  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_edge);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
               w, h, 0, 
               GL_RGB, GL_UNSIGNED_BYTE, image);
  delete [] image;
}


void Flare_texture::draw_line(float x0, float y0, float x1, float y1, 
                              unsigned char col, unsigned char alpha)
{
  float nx = x1-x0;
  float ny = y1-y0;
  
  float n = sss_max(fabs(nx),fabs(ny));
  
  int i;
  int x, y;
  
  for (i = 0 ; i < n ; ++i)
  {
    x = (int) (x0 + i * (x1-x0) / n);
    y = (int) (y0 + i * (y1-y0) / n);
    
    image[x*h*4 + y*4 + 0] = col;
    image[x*h*4 + y*4 + 1] = col;
    image[x*h*4 + y*4 + 2] = col;
    image[x*h*4 + y*4 + 3] = alpha;
  }
}

void Flare_texture::draw_poly(float radius, int num, 
                              unsigned char col, unsigned char alpha)
{
  int i;
  for (i = 0 ; i < num ; ++i)
  {
    float angle1 = i * 360 / num;
    float angle2 = (i+1) * 360 / num;
    
    float x1 = w/2 + (radius * sin_deg(angle1));
    float y1 = h/2 + (radius * cos_deg(angle1));
    float x2 = w/2 + (radius * sin_deg(angle2));
    float y2 = h/2 + (radius * cos_deg(angle2));
    
    draw_line(x1, y1, x2, y2, col, alpha);
  }
}


Flare_texture::Flare_texture()
{
  w = 64;
  h = 64;
  
  image = new GLubyte[h*w*4];
  int i;
  for (i = 0 ; i < h * w * 4 ; ++i)
    image[i] = 0;
  
  TRACE("Flare texture %d %d\n", w, h);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  const float max_radius = 30;
  const float dr = 0.1;
  const float peak_r = 25;
  const float scale_inner = 4;
  const float scale_outer = 2;

  float radius;
  float alpha;
  
  for (radius = 0 ; radius < max_radius ; radius += dr)
  {
    if (radius < peak_r)
      alpha = scale_inner/(scale_inner + (peak_r - radius));
    else
      alpha = scale_outer/(scale_outer + (radius - peak_r));

    draw_poly(radius,60,255,(int) (alpha * 255));
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  
  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,w,h,GL_RGBA,GL_UNSIGNED_BYTE, image);

  delete [] image;
}

void calculate_midpoint_displacement(Array_2D<float> & array,
                                     float r,
                                     float min_z, 
                                     float max_z)
{
  int i, j;

  int rect_size = array.get_nx();
  assert1(rect_size == (int) array.get_ny());
  const int nx = array.get_nx();
  const int ny = array.get_ny();
  const int size = rect_size;
  float dh = 1.0f;

  array(0, 0) = 0.0f;
  array(nx-1, 0) = 0.0f;
  array(0, ny-1) = 0.0f;
  array(nx-1, ny-1) = 0.0f;
  
  
  while (rect_size > 0)
  {
    // diamond step
    for (i = 0 ; i < size ; i += rect_size)
    {
      for (j = 0 ; j < size ; j += rect_size)
      {
        // allow wrapping
        int ni = (i + rect_size) % nx;
        int nj = (j + rect_size) % ny;
        
        int mi = (i + rect_size/2);
        int mj = (j + rect_size/2);        

        // ensure that we go up in the middle
        float offset = ranged_random(-dh, dh);
        
        array(mi, mj) = ( array(i, j) + array(ni, j) + 
                          array(i, nj) + array(ni, nj) ) * 0.25f + offset;
      }
    }
    
    // square step. Note that we have to be more careful about the
    // boundary conditions.

    for (i = 0 ; i < size ; i += rect_size)
    {
      for (j = 0 ; j < size ; j += rect_size)
      {
        int ni = (i + rect_size) % nx;
        int nj = (j + rect_size) % ny;

        int mi = (i + rect_size/2);
        int mj = (j + rect_size/2);

        int pmi = (i-rect_size/2 + nx) % nx;
        int pmj = (j-rect_size/2 + ny) % ny;
        
        /*
          Calculate the square value for the top side of the rectangle
        */
        array(mi, j) = ( array(i, j) + array(ni, j) + 
                         array(mi, pmj) + array(mi, mj) ) * 0.25f +
          ranged_random(-dh, dh);
      
        /*
          Calculate the square value for the left side of the rectangle
        */
        array(i, mj) = ( array(i, j) + array(i, nj) + 
                         array(pmi, mj) + array(mi, mj) ) * 0.25f +
          ranged_random(-dh, dh);
      }
    }
    
    rect_size /= 2;
    dh *= r;
  }

  // now get it to the correct range
  float a_min_z = array.get_min();
  float a_max_z = array.get_max();
  float scale = (max_z - min_z) / (a_max_z - a_min_z);
  array *= scale;
  a_min_z *= scale;
  array += min_z - a_min_z;
}

Smoke_texture::Smoke_texture(int w, int h)
{
  TRACE("Smoke texture %d %d\n", w, h);
  vector<GLfloat> image(h*w);
  int i, j;

  assert1(w == h);
  Array_2D<float> noise_map(w, h);
  calculate_midpoint_displacement(noise_map,
                                  0.5f, // r
                                  0.0f, // min
                                  1.0f);// max
  noise_map.multiply(noise_map);

  for (i = 0 ; i < w ; ++i)
  {
    for (j = 0 ; j < h ; ++j)
    {
      // x,y are [-1,1]
      float x = ((float) i - (w/2))/(w/2);
      float y = ((float) j - (h/2))/(h/2);
      float c1 = pow((1.0f - (x*x + y*y)), 1.2f);
      float c = c1*noise_map(i, j);
//      float c = c1;
      if (c > 1.0f) c = 1.0f;
      if (c < 0.0f) c = 0.0f;
      //TRACE("(%d, %d) = %4.2f \n", i, j, c);
      //      image[0 + i*4 + w * j*4] = (unsigned) (c * 255);
      //      image[1 + i*4 + w * j*4] = (unsigned) (c * 255);
      //      image[2 + i*4 + w * j*4] = (unsigned) (c * 255);
//       image[0 + i*4 + w * j*4] = 255;
//       image[1 + i*4 + w * j*4] = 255;
//       image[2 + i*4 + w * j*4] = 255;
//       image[3 + i*4 + w * j*4] = (unsigned) (c * 255);
      image[i + w * j] = c;
//      TRACE("%d ", image[i + w * j]);
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  glGenTextures(1,&texture);
  glBindTexture(GL_TEXTURE_2D, texture);

// why doesn't this work? Just produces constant whiteness
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if 0
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_FLOAT, &image[0]);
#else
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_ALPHA,w,h,GL_ALPHA,GL_FLOAT, &image[0]);
#endif
}

Detail_texture::Detail_texture(int w, int h,
                               float r, // noisiness
                               float l_min,
                               float l_max,
                               Vector3 light_dir)
{
  TRACE("Detail texture %d %d\n", w, h);
  vector<GLfloat> image(h*w);
  int i, j;

  assert1(w == h);
  Array_2D<float> noise_map1(w, h);
//   calculate_midpoint_displacement(noise_map1,
//                                   r, // r
//                                   l_min, // min
//                                   l_max);// max
//   Array_2D<float> noise_map2(w, h);
//   calculate_midpoint_displacement(noise_map2,
//                                   r, // r
//                                   l_min, // min
//                                   l_max);// max
//   noise_map2.shift(w/2, h/2);
  
//   noise_map1.add(noise_map2);
//   noise_map1 *= 0.5f;
  
//  noise_map.multiply(noise_map);

  for (i = 0 ; i < w ; ++i)
  {
    for (j = 0 ; j < h ; ++j)
    {
//      noise_map1(i, j) = ranged_random(l_min, l_max);
      noise_map1(i, j) = ranged_random(0.0f, 1.0f);
    }
  }
  noise_map1.set_wrap(true);
  noise_map1.gaussian_filter(1, 2);

  // now find the normal
  Array_2D<float> noise_map_x(noise_map1);
  noise_map_x.gradient_x();
  Array_2D<float> noise_map_y(noise_map1);
  noise_map_y.gradient_y();

  Array_2D<float> bump_map(w, h);

  light_dir.normalise();

  for (i = 0 ; i < w ; ++i)
  {
    for (j = 0 ; j < h ; ++j)
    {
      float z = 
        sqrt(1.0f - (noise_map_x(i, j) * noise_map_x(i, j) + 
                     noise_map_y(i, j) * noise_map_y(i, j) ) );

      bump_map(i, j) = dot(light_dir, Vector3(noise_map_x(i, j),
                                              noise_map_y(i, j),
                                              z));
    }
  }
//   TRACE("range = %f, %f\n", 
//         bump_map.get_min(), 
//         bump_map.get_max());
        
  // now set the range
  bump_map.set_range(l_min, l_max);

  for (i = 0 ; i < w ; ++i)
  {
    for (j = 0 ; j < h ; ++j)
    {
//      image[i + w * j] = noise_map1(i, j);;
      image[i + w * j] = bump_map(i, j);;
    }
  }

//   i = 0; j = 0; TRACE("(%d, %d) = %f\n", i, j, image[i + w * j]);
//   i = w-1; j = 0; TRACE("(%d, %d) = %f\n", i, j, image[i + w * j]);
//   i = 0; j = h-1; TRACE("(%d, %d) = %f\n", i, j, image[i + w * j]);
//   i = w-1; j = h-1; TRACE("(%d, %d) = %f\n", i, j, image[i + w * j]);

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  glGenTextures(1,&m_texture_low);
  glBindTexture(GL_TEXTURE_2D, m_texture_low);

// why doesn't this work? Just produces constant whiteness
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#if 0
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_LUMINANCE, GL_FLOAT, &image[0]);
#else
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE,w,h,GL_LUMINANCE,GL_FLOAT, &image[0]);
#endif

  // and the high
  glGenTextures(1,&m_texture_high);
  glBindTexture(GL_TEXTURE_2D, m_texture_high);

// why doesn't this work? Just produces constant whiteness
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#if 0
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_LUMINANCE, GL_FLOAT, &image[0]);
#else
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE,w,h,GL_LUMINANCE,GL_FLOAT, &image[0]);
#endif
}



void Gl_version::initialise(const char * str)
{
  m_major = 0;
  m_minor = 0;
  m_release = 0;
  enum Working_on {MAJOR, MINOR, RELEASE};
  Working_on working_on = MAJOR;

  int len = strlen(str); // no null
  int index;
  for (index = 0 ; index < len ; ++index)
  {
    if (str[index] == ' ')
    {
      // nvidia gives us something like "1.4.0 NVIDIA...."
      TRACE_FILE_IF(1)
        TRACE("Unexpected space at index %d of %s\n", index, str);
      return;
    }
    else if (str[index] == '.')
    {
      switch (working_on)
      {
      case MAJOR: working_on = MINOR; break;
      case MINOR: working_on = RELEASE; break;
      case RELEASE: TRACE("Unexpected . at index %d", index); return;
      }
    }
    else if ( (str[index] >= '0') ||
              (str[index] <= '9') )
    {
      int val;
      // store the value of what we're working on. Then we modify
      // it. Afterwards store it back in what we're working on.
      switch (working_on)
      {
      case MAJOR: val = m_major; break;
      case MINOR: val = m_minor; break;
      case RELEASE: val = m_release; break;
      }
//      TRACE("Got %c at index %d\n", str[index], index);
      // got a digit
      int digit = (int) (str[index] - '0');
      val = 10 * val + digit;
      switch (working_on)
      {
      case MAJOR: m_major = val; break;
      case MINOR: m_minor = val; break;
      case RELEASE: m_release = val; break;
      }
    }
    else
    {
      TRACE("Got %c at index %d\n", str[index], index);
      return;
    }
  }
}

bool Gl_version::operator<(const Gl_version & rhs) const
{
  if (m_major < rhs.m_major) return true;
  else if (m_major > rhs.m_major) return false;
  else if (m_minor < rhs.m_minor) return true;
  else if (m_minor > rhs.m_minor) return false;
  else if (m_release < rhs.m_release) return true;
  else if (m_release > rhs.m_release) return false;
  else return false;
}
bool Gl_version::operator>(const Gl_version & rhs) const
{
  return rhs < *this;
}
bool Gl_version::operator==(const Gl_version & rhs) const
{
  if ( (rhs < *this) || (*this < rhs) )
    return false;
  else
    return true;
}
bool Gl_version::operator<=(const Gl_version & rhs) const
{
  return ( (*this < rhs) || (*this == rhs) );
}
bool Gl_version::operator>=(const Gl_version & rhs) const
{
  return ( (*this > rhs) || (*this == rhs) );
}

void Gl_version::show() const
{
  TRACE("OpenGL version: %d.%d.%d\n", m_major, m_minor, m_release);
}

// Always Check For Extension-Availability During Run-Time!
bool isInString(char *string, const char *search) {
  TRACE_FILE_IF(2)
    TRACE("isInString(..., <%s><%s>)\n", search, string);
  int pos=0;
  int maxpos=strlen(search)-1;
  int len=strlen(string); 
  TRACE_FILE_IF(3)
    TRACE("string len = %d, search len = %d\n", len, strlen(search));
  char *other;
  for (int i=0; i<len; i++) 
  {
    if ((i==0) || ((i>1) && string[i-1]=='\n'))
    { // New Extension Begins Here!
      other=&string[i];     
      pos=0;           // Begin New Search
      while ((string[i]!='\n') && (i < len-1) )
      {                   // Search Whole Extension-String
        if (string[i]==search[pos]) 
          pos++;   // Next Position
        if ((pos>maxpos) && string[i+1]=='\n') 
          return true; // We Have A Winner!
        i++;
        TRACE_FILE_IF(3)
          TRACE("i = %d, pos = %d\n", i, pos);
      }     
    }
  } 
  return false; // Sorry, Not Found!
}


#ifdef WIN32
PFNGLACTIVETEXTUREARBPROC _glActiveTextureARB = 0;

extern "C" {
  void APIENTRY glActiveTextureARB(GLenum val)
  {
    if (_glActiveTextureARB)
      (*_glActiveTextureARB)(val);

  }
}
#endif

// isMultitextureSupported() Checks At Run-Time If Multitexturing Is Supported
// must be called after a context has been created
bool initMultitexture(void) 
{
#ifdef WITH_GL_EXT
  char *extensions; 
  const unsigned char * str = glGetString(GL_EXTENSIONS);

  // initialise the gl version in a nice form
  gl_version.initialise((const char *) glGetString(GL_VERSION));
  TRACE_FILE_IF(2)
    gl_version.show();
#if 0
  // some testing
  if (gl_version > Gl_version(1, 2, 0))
    TRACE("Greater than 1.2.0\n");
  if (gl_version < Gl_version(1, 2, 0))
    TRACE("Less than 1.2.0\n");
  if (gl_version < Gl_version(1, 4, 0))
    TRACE("Less than 1.4.0\n");
  if (gl_version > Gl_version(1, 4, 0))
    TRACE("Greater than 1.4.0\n");
  if (gl_version == Gl_version(1, 3, 1))
    TRACE("Equal to 1.3.1\n");
  if (gl_version >= Gl_version(1, 3, 1))
    TRACE("Equal/greater to 1.3.1\n");
#endif
  // We also trace some useful info
  TRACE("GL vendor = %s, renderer = %s, version = %s\n",
        glGetString(GL_VENDOR),
        glGetString(GL_RENDERER),
        glGetString(GL_VERSION));

#ifdef WIN32
  _glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
  if (_glActiveTextureARB == 0)
  {
    TRACE_FILE_IF(2)
      TRACE("Multitexture not supported\n");
    multitextureSupported=false;
    return false;
  }
#endif

  if (!str)
  {
    TRACE("glGetString returned 0!\n");
    multitextureSupported=false;
    return false;
  }
  int len = 0;
  while (str[len] != 0)
    ++len;
  TRACE_FILE_IF(3)
    TRACE("Extensions len = %d\n", len);

  extensions=strdup((char *) str); // Fetch Extension String
  
#if 0
  for (int i=0; i<len; i++)   // Separate It By Newline Instead Of Blank
  {
    if (extensions[i]==' ') 
    {
      extensions[i]='\n';
    }
  }
#endif

  TRACE_FILE_IF(2)
    TRACE("supported GL extensions\n%s \n", extensions);
  
  //if (isInString(extensions,"GL_ARB_multitexture") // Is Multitexturing Supported?
  //  && isInString(extensions,"GL_EXT_texture_env_combine")) // Is texture_env_combining Supported?
  if (strstr(extensions,"GL_ARB_multitexture") // Is Multitexturing Supported?
      && strstr(extensions,"GL_ARB_texture_env_combine")) // Is texture_env_combining Supported?
  { 
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &maxTexelUnits);
    TRACE_FILE_IF(2)
      TRACE("Multitexture supported with %d units\n", maxTexelUnits);
    multitextureSupported=true;
    return true;
  }
  TRACE_FILE_IF(2)
    TRACE("Multitexture not supported\n");
  multitextureSupported=false;
  return false;
#else
  TRACE("Compiled without support for multi-texture\n");
  return false;
#endif
}

