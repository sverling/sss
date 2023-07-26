/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#ifndef SSS_MISC_H
#define SSS_MISC_H

#include "my_glut.h"
#include "log_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

// some useful angle things
#define PI 3.1415926535897932384626433832795f
#define TWO_PI 6.28318530717958647692528676655901f
#define PI_DIV_180 0.0174532925199432957692369076848861f
#define _180_DIV_PI 57.2957795130823208767981548141052f
inline float deg_to_rad(float deg) {return (float) (deg * PI_DIV_180);}
inline float rad_to_deg(float rad) {return (float) (rad * _180_DIV_PI);}

inline float sin_deg(float deg) {return (float) sin(deg_to_rad(deg));}
inline float cos_deg(float deg) {return (float) cos(deg_to_rad(deg));}
inline float asin_deg(float x) {return rad_to_deg(asin((float) x));}
inline float acos_deg(float x) {return rad_to_deg(acos((float) x));}
inline float tan_deg(float deg) {return (float) tan(deg_to_rad(deg));}
inline float atan2_deg(float x, float y) {return rad_to_deg((float) atan2(x, y));}
inline float atan_deg(float x) {return rad_to_deg((float) atan(x));}

// there is an STL version somewhere...
template<class T>
inline T sss_min(const T a, const T b) {return (a < b ? a : b);}
template<class T>
inline T sss_max(const T a, const T b) {return (a > b ? a : b);}

inline bool check_errors(char * str="")
{
  GLenum err_code = glGetError();
  if (err_code != GL_NO_ERROR)
  {
    TRACE("OpenGL Error: %s [%s]\n", gluErrorString(err_code), str);
    return true;
  }
  return false;
}

/*!
  Returns a random number between v1 and v2
*/
inline float ranged_random(float v1,float v2)
{
	return v1 + (v2-v1)*((float)rand())/((float)RAND_MAX);
}

/*! Indicates if the machine is little-endian */
bool is_little_endian();

//! Takes a 4-byte word and converts it to little endian (if necessary)
void convert_word_to_little_endian(void * orig);
//! Takes a 4-byte word and converts it from little endian to whatever
//! is this machine is (if necessary)
void convert_word_from_little_endian(void * orig);

/*
  on win32 emulate the nice fpclassify
  */
inline bool is_finite(float val)
{
  const float very_big = 1.0e8;
  if (val < very_big && val > -very_big)
    return true;
  else
    return false;
}

#endif







