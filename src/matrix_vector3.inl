/*!
  SSS - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file matrix_vector3.inl
*/
//#include "matrix_vector3.h"

#include "misc.h"
#include "log_trace.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h> // for memcpy

//########################################################################
// 
//                       Matrix3
//
//########################################################################
inline Matrix3::Matrix3() {}
inline Matrix3::~Matrix3() {}

inline void Matrix3::set_to(float val)
{
  data[0] = val;
  data[1] = val;
  data[2] = val;
  data[3] = val;
  data[4] = val;
  data[5] = val;
  data[6] = val;
  data[7] = val;
  data[8] = val;
}

inline Matrix3::Matrix3(float val) {set_to(val);}

inline Matrix3::Matrix3(const Matrix3 & matrix)
{
  memcpy(data, matrix.data, 9*sizeof(float));
}

inline Matrix3::Matrix3(float v11, float v21, float v31, // first column
                        float v12, float v22, float v32, // 2nd column
                        float v13, float v23, float v33  )
{
  operator()(0, 0) = v11;
  operator()(1, 0) = v21;
  operator()(2, 0) = v31;
  
  operator()(0, 1) = v12;
  operator()(1, 1) = v22;
  operator()(2, 1) = v32;
  
  operator()(0, 2) = v13;
  operator()(1, 2) = v23;
  operator()(2, 2) = v33;
  
}
inline Matrix3::Matrix3(const Vector3 & v1, // first column
                        const Vector3 & v2, 
                        const Vector3 & v3)
{
  operator()(0, 0) = v1[0];
  operator()(1, 0) = v1[1];
  operator()(2, 0) = v1[2];
  
  operator()(0, 1) = v2[0];
  operator()(1, 1) = v2[1];
  operator()(2, 1) = v2[2];
  
  operator()(0, 2) = v3[0];
  operator()(1, 2) = v3[1];
  operator()(2, 2) = v3[2];
}

inline void Matrix3::set_data(const float * d)
{
  memcpy(data, d, 9*sizeof(float));
}

inline Vector3 Matrix3::get_col(uint i) const
{
  const uint o = i*3; 
  return Vector3(data[o], data[o+1], data[o+2]);
}

inline Matrix3 & Matrix3::operator=(const Matrix3 & rhs)
{
  memcpy(data, rhs.data, 9*sizeof(float));
  return *this;
}

inline bool Matrix3::sensible() const
{
  for (unsigned i = 0 ; i < 9 ; ++i)
  {
    if (!is_finite(data[i]))
      return false;
  }
  return true;
}

inline void Matrix3::show(const char * str) const
{
  uint i, j;
  TRACE("%p Matrix3::this = 0x%x \n", str, this);
  for (i = 0 ; i < 3 ; i++)
  {
    for (j = 0 ; j < 3 ; j++)
    {
      TRACE("%4f ", operator()(i, j));
    }
    TRACE("\n");
  }
}

inline Matrix3 & Matrix3::operator+=(const Matrix3 & rhs)
{
  for (uint i = 9 ; i-- != 0 ;)
    data[i] += rhs.data[i];
  return *this;
}

inline Matrix3 & Matrix3::operator-=(const Matrix3 & rhs)
{
  for (uint i = 9 ; i-- != 0 ;)
    data[i] -= rhs.data[i];
  return *this;
}

inline Matrix3 & Matrix3::operator*=(const float rhs)
{
  for (uint i = 9 ; i-- != 0 ;)
    data[i] *= rhs;
  return *this;
}

inline Matrix3 & Matrix3::operator/=(const float rhs)
{
  const float inv_rhs = 1.0f/rhs;
  for (uint i = 9 ; i-- != 0 ;)
    data[i] *= inv_rhs;
  return *this;
}

inline Matrix3 Matrix3::operator+(const Matrix3 & rhs) const
{
  return Matrix3(*this) += rhs;
}

inline Matrix3 Matrix3::operator-(const Matrix3 & rhs) const
{
  return Matrix3(*this) -= rhs;
}

// global operators

inline Matrix3 operator*(const Matrix3 & lhs, const float rhs)
{
  Matrix3 result;
  
  for (uint i = 9 ; i-- != 0 ; )
    result.data[i] = rhs * lhs.data[i];
  return result;
}

inline Matrix3 operator*(const Matrix3 & lhs, const Matrix3 & rhs)
{
  static Matrix3 out; // avoid ctor/dtor
  
  for (uint oj = 3 ; oj-- != 0 ;)
  {
    for (uint oi = 3 ; oi-- != 0 ;)
    {
      out(oi, oj) =
        lhs(oi, 0)*rhs(0, oj) +
        lhs(oi, 1)*rhs(1, oj) +
        lhs(oi, 2)*rhs(2, oj);
    }
  }
  return out;
}

//########################################################################
// 
//                       Vector3
//
//########################################################################

inline void Vector3::set_to(float val)
{
  data[0] = val;
  data[1] = val;
  data[2] = val;
}

inline Vector3::Vector3(const Vector3 & matrix)
{
  memcpy(data, matrix.data, 3*sizeof(float));
}

inline Vector3::Vector3(float val) {set_to(val);}

inline Vector3 & Vector3::normalise()
{
  const float m2 = mag2();
  if (m2 > 0.0f)
  {
    const float inv_mag = 1.0f/sqrt(m2);
    data[0] = data[0] * inv_mag;
    data[1] = data[1] * inv_mag;
    data[2] = data[2] * inv_mag;
    return *this;
  }
  else
  {
    TRACE("magnitude = %f in normalise()\n", sqrt(m2));
    return *this;
  }
}

inline void Vector3::set_data(const float * d)
{
  memcpy(data, d, 3*sizeof(float));  
}

inline Vector3 & Vector3::operator=(const Vector3 & rhs)
{
  memcpy(data, rhs.data, 3*sizeof(float));
  return *this;
}

inline Vector3 & Vector3::operator+=(const Vector3 & rhs)
{
  data[0] += rhs.data[0];
  data[1] += rhs.data[1];
  data[2] += rhs.data[2];
  return *this;
}

inline Vector3 & Vector3::operator-=(const Vector3 & rhs)
{
  data[0] -= rhs.data[0];
  data[1] -= rhs.data[1];
  data[2] -= rhs.data[2];
  return *this;
}

inline Vector3 & Vector3::operator*=(const float rhs)
{
  data[0] *= rhs;
  data[1] *= rhs;
  data[2] *= rhs;
  return *this;
}

inline Vector3 & Vector3::operator/=(const float rhs)
{
  const float inv_rhs = 1.0f/rhs;
  data[0] *= inv_rhs;
  data[1] *= inv_rhs;
  data[2] *= inv_rhs;
  return *this;
}

inline Vector3 Vector3::operator+(const Vector3 & rhs) const
{
  return Vector3(*this) += rhs;
}

inline Vector3 Vector3::operator-(const Vector3 & rhs) const
{
  return Vector3(*this) -= rhs;
}

inline bool Vector3::sensible() const
{
  for (unsigned i = 3 ; i-- != 0 ;)
  {
    if (!is_finite(data[i]))
      return false;
  }
  return true;
}

inline void Vector3::show(const char * str) const
{
  uint i;
  TRACE("%s Vector3::this = %p \n", str, this);
  for (i = 0 ; i < 3 ; i++)
  {
    TRACE("%4f ", data[i]);
  }
  TRACE("\n");
}

// Helper for orthonormalise - projection of v2 onto v1
static inline Vector3 proj(const Vector3 & v1, const Vector3 & v2)
{
  return dot(v1, v2) * v1 / v1.mag2();
}

inline void Matrix3::orthonormalise()
{
  Vector3 u1(operator()(0, 0), operator()(1, 0), operator()(2, 0));
  Vector3 u2(operator()(0, 1), operator()(1, 1), operator()(2, 1));
  Vector3 u3(operator()(0, 2), operator()(1, 2), operator()(2, 2));
  
  Vector3 w1 = u1.normalise();
  
  Vector3 w2 = (u2 - proj(w1, u2)).normalise();
  Vector3 w3 = (u3 - proj(w1, u3) - proj(w2, u3)).normalise();
  
  operator()(0, 0) = w1[0];
  operator()(1, 0) = w1[1];
  operator()(2, 0) = w1[2];
  
  operator()(0, 1) = w2[0];
  operator()(1, 1) = w2[1];
  operator()(2, 1) = w2[2];
  
  operator()(0, 2) = w3[0];
  operator()(1, 2) = w3[1];
  operator()(2, 2) = w3[2];
  
  if (sensible() == false)
  {
    TRACE("orthonormalise() resulted in bad matrix\n");
    *this = Matrix3(Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1));
  }
}


// global operators

inline Vector3 operator*(const Vector3 & lhs, const float rhs)
{
  return Vector3(lhs.data[0] * rhs,
                 lhs.data[1] * rhs,
                 lhs.data[2] * rhs);
}

inline Vector3 operator/(const Vector3 & lhs, const float rhs)
{
  const float inv_rhs = 1.0f/rhs;
  return Vector3(lhs.data[0] * inv_rhs,
                 lhs.data[1] * inv_rhs,
                 lhs.data[2] * inv_rhs);
}

inline float dot(const Vector3 & lhs, const Vector3 & rhs)
{
  return (lhs.data[0] * rhs.data[0] +
          lhs.data[1] * rhs.data[1] +
          lhs.data[2] * rhs.data[2]);
}

inline Vector3 cross(const Vector3 & lhs, const Vector3 & rhs)
{
  return Vector3(lhs[1]*rhs[2] - lhs[2]*rhs[1],
                 lhs[2]*rhs[0] - lhs[0]*rhs[2],
                 lhs[0]*rhs[1] - lhs[1]*rhs[0]);
}

// matrix * vector
inline Vector3 operator*(const Matrix3 & lhs, const Vector3 & rhs)
{
  return Vector3(
    lhs(0,0) * rhs[0] +
    lhs(0,1) * rhs[1] +
    lhs(0,2) * rhs[2],
    lhs(1,0) * rhs[0] +
    lhs(1,1) * rhs[1] +
    lhs(1,2) * rhs[2],
    lhs(2,0) * rhs[0] +
    lhs(2,1) * rhs[1] +
    lhs(2,2) * rhs[2]);
}

inline Matrix3 transpose(const Matrix3 & rhs)
{
  return Matrix3(rhs(0, 0), rhs(0, 1), rhs(0, 2),
                 rhs(1, 0), rhs(1, 1), rhs(1, 2),
                 rhs(2, 0), rhs(2, 1), rhs(2, 2) );
}


// Some useful rotation Matrix3's
inline Matrix3 alpha(float alpha)
{
  Matrix3 result(0);
  float s = (float) sin_deg(alpha);
  float c = (float) cos_deg(alpha);
  
  result(0,0) = 1;
  result(1,1) = c;
  result(2,2) = c;
  result(2,1) = s;
  result(1,2) = -s;
  
  return result;
}

inline Matrix3 beta(float beta)
{
  Matrix3 result(0);
  float s = (float) sin_deg(beta);
  float c = (float) cos_deg(beta);
  
  result(1,1) = 1;
  result(2,2) = c;
  result(0,0) = c;
  result(0,2) = s;
  result(2,0) = -s;
  
  return result;
}

inline Matrix3 gamma(float gamma)
{
  Matrix3 result(0);
  float s = (float) sin_deg(gamma);
  float c = (float) cos_deg(gamma);
  
  result(2,2) = 1;
  result(0,0) = c;
  result(1,1) = c;
  result(1,0) = s;
  result(0,1) = -s;
  
  return result;
}

inline Matrix3 rotation_matrix(float ang, const Vector3 & dir)
{
  // from page 32(45) of glspec.dvi
  Matrix3 uut(dir[0]*dir[0], dir[1]*dir[0], dir[2]*dir[0],
              dir[0]*dir[1], dir[1]*dir[1], dir[2]*dir[1],
              dir[0]*dir[2], dir[1]*dir[2], dir[2]*dir[2]);
  
//    uut.set(0,0, dir[0]*dir[0]);
//    uut.set(0,1, dir[0]*dir[1]);
//    uut.set(0,2, dir[0]*dir[2]);
  
//    uut.set(1,0, dir[1]*dir[0]);
//    uut.set(1,1, dir[1]*dir[1]);
//    uut.set(1,2, dir[1]*dir[2]);
  
//    uut.set(2,0, dir[2]*dir[0]);
//    uut.set(2,1, dir[2]*dir[1]);
//    uut.set(2,2, dir[2]*dir[2]);
  
  Matrix3 s(0, dir[2], -dir[1],
            -dir[2], 0, dir[0],
            dir[1], -dir[0], 0);
  
//    s.set(0,1, -dir[2]);
//    s.set(0,2,  dir[1]);
  
//    s.set(1,0,  dir[2]);
//    s.set(1,2, -dir[0]);
  
//    s.set(2,0, -dir[1]);
//    s.set(2,1,  dir[0]);
  
  return (uut + (float) cos_deg(ang) * 
          (matrix3_identity() - uut) + (float) sin_deg(ang) * s);
}
