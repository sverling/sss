/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef TYPES_H
#define TYPES_H
#ifndef unix
#pragma warning (disable : 4786 4800 4161)    //to avoid stupid warnings from mfc 'cause stl
#endif

#include "matrix_vector3.h"

typedef Vector3 Vector;
typedef Matrix3 Matrix;

// The following refer to the global coordinate system...
typedef Vector Position;
typedef Vector Velocity;
typedef Matrix Orientation;
typedef Vector Rotation; // apart from this, which is in the body's reference frame

/// Describes a hard point, with directional friction.
struct Hard_point
{
  Hard_point(const Position & pos,
             const Vector & min_friction_dir,
             float mu_max,
             float mu_min,
             float hardness,
             const Velocity & vel = Velocity(0.0f)) :
    pos(pos), 
    min_friction_dir(min_friction_dir), 
    mu_max(mu_max), mu_min(mu_min), hardness(hardness),
    m_vel(vel) {}; 

  Hard_point(const Position & pos) :
    pos(pos), min_friction_dir(Vector(1, 0, 0)),
    mu_max(0.8f), mu_min(0.8f), hardness(1.0f), m_vel(0.0f) {};

  Hard_point() {};
  Position pos;
  Vector min_friction_dir; /// Direction for mu_min (local coords)
  float mu_max; // friction coefficient in the min_friction_dir
  float mu_min; // friction coefficient perp. to min_friction_dir
  float hardness; // hardness of the point (very qualitative) - e.g. 1
  Velocity m_vel; // point velocity in local coords
};

class Color
{
public:
  Color(float r, float g, float b, float a) {
    vec[0] = r;
    vec[1] = g;
    vec[2] = b;
    vec[3] = a;
  }
  Color() {vec[0] = 1;vec[1] = 1;vec[2] = 1;vec[3] = 1;}
  Color operator=(const Color & col) {
    vec[0] = col.vec[0];
    vec[1] = col.vec[1];
    vec[2] = col.vec[2];
    vec[3] = col.vec[3];
    return *this;
  }    
  const float * get_vec() {return vec;}
  float vec[4];
};

#define RED    Color(1,0,0,1)
#define GREEN  Color(0,1,0,1)
#define BLUE   Color(0,0,1,1)
#define YELLOW Color(1,1,0,1)
#define WHITE  Color(1,1,1,1)


#endif // file included





