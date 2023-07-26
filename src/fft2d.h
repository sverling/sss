#ifndef FFT2D_H
#define FFT2D_H

#include <assert.h>

struct Complex
{
  Complex(float r = 0.0, float i = 0.0) : real(r), imag(i) {};
  Complex & operator*=(const float rhs) { real *= rhs; imag *= rhs; return *this;}

  float real;
  float imag;
};

class Complex_array
{
public:
  Complex_array(int nx, int ny);
  Complex_array(const Complex_array & orig);
  ~Complex_array();
  
  Complex_array & operator=(const Complex_array & rhs);

  void rotate(); // "rotates" the array to shift it by nx/2 in each dir
  void flip_lr();
  void flip_ud();

  void tile(); // makes the array "tile" by replicating it (with flips)
  
  const Complex & operator()(int i, int j) const {return data[calc_index(i, j)];}
  Complex & operator()(int i, int j) {assert(i < nx && j < ny) ; return data[calc_index(i, j)];}

  int get_nx() const {return nx;}
  int get_ny() const {return ny;}

private:
  int calc_index(int i, int j) const {return i + j * nx;}

  Complex * data;
  int nx, ny;
};



/*-------------------------------------------------------------------------
   Perform a 2D FFT inplace given a complex 2D array
   The direction dir, 1 for forward, -1 for reverse
   The size of the array (nx,ny)
   Return false if there are memory problems or
      the dimensions are not powers of 2
*/
bool FFT2D(Complex_array & c,int nx,int ny,int dir);


#endif // file included
