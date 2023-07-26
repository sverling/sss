/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PLANE_H
#define PLANE_H

/*!
  \file
  This defines the stuff needed for plane processing. This
  needs 4 vectors and 4x4 matrices.
  Really something should be done about the matrix stuff being 
  in here too
*/
#include "matrix_vector3.h"

class Matrix4;

class Plane
{
public:
  Plane(float a, float b, float c, float d) 
  {
    data[0] = a; data[1] = b; data[2] = c; data[3] = d;
  }
  Plane() {}

  float & operator()(int i) {return data[i];}
  const float & operator()(int i) const {return data[i];}

  inline float distance_to_point(const Position & pos) const;
  
  inline float distance_to_point(float x, float y, float z) const;
  
  inline void show() const;
  
  friend Plane operator*(const Matrix4 & lhs, const Plane & rhs);

private:
  float data[4];
};

class Matrix4
{
public:
  Matrix4(float d00, float d01, float d02, float d03,
          float d10, float d11, float d12, float d13,
          float d20, float d21, float d22, float d23,
          float d30, float d31, float d32, float d33)
  {
    data[0][0] = d00; data[0][1] = d01; data[0][2] = d02; data[0][3] = d03;
    data[1][0] = d10; data[1][1] = d11; data[1][2] = d12; data[1][3] = d13;
    data[2][0] = d20; data[2][1] = d21; data[2][2] = d22; data[2][3] = d23;
    data[3][0] = d30; data[3][1] = d31; data[3][2] = d32; data[3][3] = d33;
  }
  float & operator()(int i, int j) {return data[i][j];}
  const float & operator()(int i, int j) const {return data[i][j];}
  friend Plane operator*(const Matrix4 & lhs, const Plane & rhs);
private:
  float data[4][4];
};

inline float Plane::distance_to_point(const Position & pos) const
{
  Vector3 normal(data[0], data[1], data[2]);
  // normal is already normalised.. I hope...!
  float dist =  dot(normal, pos);
  return dist + data[3];
}

inline float Plane::distance_to_point(float x, float y, float z) const
{
  return data[0]*x + data[1]*y + data[2]*z + data[3];
}

inline void Plane::show() const
{
  printf("Plane %p: %6.2f %6.2f %6.2f %6.2f\n",
         this,
         data[0], data[1], data[2], data[3]);
}

inline Plane operator*(const Matrix4 & lhs, const Plane & rhs)
{
  Plane result;
  
  for (uint i = 0 ; i < 4 ; i++)
  {
    result(i) = 
      lhs(i,0) * rhs(0) +
      lhs(i,1) * rhs(1) +
      lhs(i,2) * rhs(2) +
      lhs(i,3) * rhs(3);
  }
  return result;
}

#endif
