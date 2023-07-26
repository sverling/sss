/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef MATRIX_VECTOR3
#define MATRIX_VECTOR3

#include <math.h>

typedef unsigned int uint;

class Vector3;

//! A 3x3 matrix
class Matrix3
{
public:
  inline Matrix3(const Matrix3 & matrix);
  inline Matrix3(float v11, float v21, float v31, // first column
                 float v12, float v22, float v32, // 2nd column
                 float v13, float v23, float v33  );
  inline Matrix3(const Vector3 & v1, // first column
                 const Vector3 & v2, 
                 const Vector3 & v3);
  inline Matrix3();
  inline Matrix3(float val);
  inline ~Matrix3();

  inline void set_to(float val);
  inline void orthonormalise();
  
  inline bool sensible() const; // indicates if all is OK

  float & operator()(uint i, uint j) {return data[i + 3*j];}
  const float & operator()(uint i, uint j) const {return data[i + 3*j];}
  
  //! returns pointer to the first element
  inline const float * get_data() {return data;} 
  inline const float * get_data() const {return data;} 
  //! pointer to value returned from get_data
  inline void set_data(const float * d); 
  
  /// Returns a column - no range checking!
  inline Vector3 get_col(uint i) const;

  // operators
  inline Matrix3 & operator=(const Matrix3 & rhs);
  
  inline Matrix3 & operator+=(const Matrix3 & rhs);
  inline Matrix3 & operator-=(const Matrix3 & rhs);

  inline Matrix3 & operator*=(const float rhs);
  inline Matrix3 & operator/=(const float rhs);

  inline Matrix3 operator+(const Matrix3 & rhs) const;
  inline Matrix3 operator-(const Matrix3 & rhs) const;
  friend Matrix3 operator*(const Matrix3 & lhs, const float rhs);
  friend Matrix3 operator*(const float lhs, const Matrix3 & rhs);
  friend Matrix3 operator*(const Matrix3 & lhs, const Matrix3 & rhs);
  friend Matrix3 transpose(const Matrix3 & rhs);
  
  friend Vector3 operator*(const Matrix3 & lhs, const Vector3 & rhs);

  inline void show(const char * str = "") const;
  
private:
  float data[3*3];
};

//############## Vector3 ################
//! A 3x1 matrix (i.e. a vector)
class Vector3
{
public:
  inline Vector3(const Vector3 & vector);
  inline Vector3() {};
  inline Vector3(float val);
  inline Vector3(float x, float y, float z) 
    {data[0] = x; data[1] = y; data[2] = z;}
  inline ~Vector3() {};
  
  inline void set_to(float val); //!< Set all values to val
  
  inline bool sensible() const; // indicates if all is OK

  float & operator[](uint i) {return data[i];} //!< unchecked access
  const float & operator[](uint i) const {return data[i];}
  float & operator()(uint i) {return data[i];}
  const float & operator()(uint i) const {return data[i];}
  
  //! returns pointer to the first element
  inline const float * get_data() {return data;} 
  inline const float * get_data() const {return data;} 
  //! pointer to value returned from get_data
  inline void set_data(const float * d);
  
  //! calculate the square of the magnitude
  inline float mag2() const {
    return (float) (data[0]*data[0]+data[1]*data[1]+data[2]*data[2]);}
  //! calculate the magnitude
  inline float mag() const {return (float) sqrt(mag2());}
  //! Normalise, and return the result
  inline Vector3 & normalise();
  
  // operators
  inline Vector3 & operator=(const Vector3 & rhs);
  inline Vector3 & operator+=(const Vector3 & rhs);
  inline Vector3 & operator-=(const Vector3 & rhs);

  inline Vector3 & operator*=(const float rhs);
  inline Vector3 & operator/=(const float rhs);

  inline Vector3 operator-() const {return Vector3(-data[0], -data[1], -data[2]);}
  
  inline Vector3 operator+(const Vector3 & rhs) const;
  inline Vector3 operator-(const Vector3 & rhs) const;

  friend Vector3 operator*(const Vector3 & lhs, const float rhs);
  friend Vector3 operator*(const float lhs, const Vector3 & rhs);
  friend Vector3 operator/(const Vector3 & lhs, const float rhs);
  friend float dot(const Vector3 & lhs, const Vector3 & rhs);
  friend Vector3 cross(const Vector3 & lhs, const Vector3 & rhs);
  
  friend Vector3 operator*(const Matrix3 & lhs, const Vector3 & rhs);
  
  friend Matrix3 rotation_matrix(float ang, const Vector3 & dir);
  
  inline void show(const char * str = "") const;
  
private:
  float data[3];
};

// global operators

inline Matrix3 operator*(const Matrix3 & lhs, const float rhs);
inline Matrix3 operator*(const float lhs, const Matrix3 & rhs) {return rhs * lhs;}
inline Matrix3 operator*(const Matrix3 & lhs, const Matrix3 & rhs);

inline Vector3 operator*(const Vector3 & lhs, const float rhs);
inline Vector3 operator/(const Vector3 & lhs, const float rhs);
inline float dot(const Vector3 & lhs, const Vector3 & rhs);
inline Vector3 cross(const Vector3 & lhs, const Vector3 & rhs);
inline Vector3 operator*(const float lhs, const Vector3 & rhs) {return rhs * lhs;}
inline Matrix3 transpose(const Matrix3 & rhs);

// matrix * vector
inline Vector3 operator*(const Matrix3 & lhs, const Vector3 & rhs);

// Some useful rotation Matrix3's
// alpha returns a matrix that wil rotate alpha around the x axis (etc)
inline Matrix3 alpha(float alpha);
inline Matrix3 beta(float beta);
inline Matrix3 gamma(float gamma);

inline const Matrix3 & matrix3_identity()
{
  static const Matrix3 result(1, 0, 0,
                              0, 1, 0,
                              0, 0, 1);
  return result;
}

inline Matrix3 rotation_matrix(float ang, const Vector3 & dir);

#include "matrix_vector3.inl"

#endif // include file



