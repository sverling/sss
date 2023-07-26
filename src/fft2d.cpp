#include "fft2d.h"

#include "sss_assert.h"
#include <math.h>
#include <string.h>

Complex_array::Complex_array(int nx, int ny)
  :
  nx(nx), ny(ny)
{
  data = new Complex[nx*ny];
  for (int i = 0 ; i < nx*ny ; ++i)
  {
    data[i]= 0;
  }
}

Complex_array::~Complex_array()
{
  delete [] data;
  data = 0;
}

Complex_array::Complex_array(const Complex_array & orig)
  :
  nx(orig.nx), ny(orig.ny)
{
  data = new Complex[nx*ny];
  memcpy(data, orig.data, nx*ny*sizeof(Complex));
}

Complex_array & Complex_array::operator=(const Complex_array & rhs)
{
  delete [] data;
  nx = rhs.nx;
  ny = rhs.ny;
  data = new Complex[nx*ny];
  memcpy(data, rhs.data, nx*ny*sizeof(Complex));
  return *this;
}

void Complex_array::rotate()
{
  Complex_array orig(*this);
  int nx2 = nx/2;
  int ny2 = ny/2;
  assert1(nx2*2 == nx);
  assert1(ny2*2 == ny);
  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      (*this)(i, j) = orig( (i+nx2) % nx, (j+ny2) % ny );
    }
  }
}

void Complex_array::flip_lr()
{
  Complex_array orig(*this);
  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      (*this)(i, j) = orig( (nx-1) - i, j );
    }
  }
}

void Complex_array::flip_ud()
{
  Complex_array orig(*this);
  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      (*this)(i, j) = orig( i, (ny -1) - j );
    }
  }
}


void Complex_array::tile()
{
  Complex_array bot_left(*this); // stays in same location
  Complex_array bot_right(*this);    
  bot_right.flip_lr();
  Complex_array top_left(*this);
  top_left.flip_ud();
  Complex_array top_right(top_left);
  top_right.flip_lr();
  
  delete [] data;
  int nx0 = nx;
  int ny0 = ny;
  nx *= 2;
  ny *= 2;

  data = new Complex[nx*ny];
  
  int i, j;
  for (i = 0 ; i < nx0 ; ++i)
  {
    for (j = 0 ; j < ny0 ; ++j)
    {
      (*this)(i,     j    ) = bot_left(i, j);
      (*this)(i+nx0, j    ) = bot_right(i, j);
      (*this)(i,     j+ny0) = top_left(i, j);
      (*this)(i+nx0, j+ny0) = top_right(i, j);
    }
  }
}


#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
#else
int rint(double val)
{
  if (val > 0)
    return (int) (val + 0.5);
  else
    return (int) (val - 0.5);
}
#endif

/* 
   returns true if nx is a power of two, and sets m so that 2^m = nx 
*/
bool powerof2(int nx, int * m)
{
  *m = (int) rint( log((double) nx)/log(2.0));
  if (rint(pow(2.0, (double) *m)) != nx)
  {
    return false;
  }
  return true;
}

/*-------------------------------------------------------------------------
  This computes an in-place complex-to-complex FFT
  x and y are the real and imaginary arrays of 2^m points.
  dir =  1 gives forward transform
  dir = -1 gives reverse transform
  
  Formula: forward
  N-1
  ---
  1   \          - j k 2 pi n / N
  X(n) = ---   > x(k) e                    = forward transform
  N   /                                n=0..N-1
  ---
  k=0
  
  Formula: reverse
  N-1
  ---
  \          j k 2 pi n / N
  X(n) =       > x(k) e                    = forward transform
  /                                n=0..N-1
  ---
  k=0
*/
bool FFT(int dir,int m,float *x,float *y)
{
  long nn,i,i1,j,k,i2,l,l1,l2;
  float c1,c2,tx,ty,t1,t2,u1,u2,z;
  
  /* Calculate the number of points */
  nn = 1;
  for (i=0;i<m;i++)
    nn *= 2;
  
  /* Do the bit reversal */
  i2 = nn >> 1;
  j = 0;
  for (i=0;i<nn-1;i++) 
  {
    if (i < j) 
    {
      tx = x[i];
      ty = y[i];
      x[i] = x[j];
      y[i] = y[j];
      x[j] = tx;
      y[j] = ty;
    }
    k = i2;
    while (k <= j) 
    {
      j -= k;
      k >>= 1;
    }
    j += k;
  }
  
  /* Compute the FFT */
  c1 = -1.0;
  c2 = 0.0;
  l2 = 1;
  for (l=0;l<m;l++) 
  {
    l1 = l2;
    l2 <<= 1;
    u1 = 1.0;
    u2 = 0.0;
    for (j=0;j<l1;j++) 
    {
      for (i=j;i<nn;i+=l2) 
      {
        i1 = i + l1;
        t1 = u1 * x[i1] - u2 * y[i1];
        t2 = u1 * y[i1] + u2 * x[i1];
        x[i1] = x[i] - t1;
        y[i1] = y[i] - t2;
        x[i] += t1;
        y[i] += t2;
      }
      z =  u1 * c1 - u2 * c2;
      u2 = u1 * c2 + u2 * c1;
      u1 = z;
    }
    c2 = (float) sqrt((1.0 - c1) / 2.0);
    if (dir == 1)
      c2 = -c2;
    c1 = (float) sqrt((1.0 + c1) / 2.0);
  }
  
  /* Scaling for forward transform */
  if (dir == 1) 
  {
    for (i=0;i<nn;i++) 
    {
      x[i] /= nn;
      y[i] /= nn;
    }
  }
  
  return true;
}

/*-------------------------------------------------------------------------
  Perform a 2D FFT inplace given a complex 2D array
  The direction dir, 1 for forward, -1 for reverse
  The size of the array (nx,ny)
  Return false if there are memory problems or
  the dimensions are not powers of 2
*/
bool FFT2D(Complex_array & c,int nx,int ny,int dir)
{
  int i,j;
  int m;
  float *real,*imag;
  
  /* Transform the rows */
  real = new float[nx];
  imag = new float[nx];
    
  if (!powerof2(nx,&m))
  {
    delete [] real;
    delete [] imag;
    return false;
  }
  
  for (j=0;j<ny;j++) 
  {
    for (i=0;i<nx;i++) 
    {
      real[i] = c(i, j).real;
      imag[i] = c(i, j).imag;
    }
    FFT(dir,m,real,imag);
    for (i=0;i<nx;i++) 
    {
      c(i, j).real = real[i];
      c(i, j).imag = imag[i];
    }
  }
  
  delete [] real;
  delete [] imag;
  
  /* Transform the columns */
  real = new float[ny];
  imag = new float[ny];
  
  if (!powerof2(ny,&m))
  {
    delete [] real;
    delete [] imag;
    return false;
  }
  
  for (i=0;i<nx;i++) 
  {
    for (j=0;j<ny;j++) 
    {
      real[j] = c(i, j).real;
      imag[j] = c(i, j).imag;
    }
    FFT(dir,m,real,imag);
    for (j=0;j<ny;j++) 
    {
      c(i, j).real = real[j];
      c(i, j).imag = imag[j];
    }
  }
  delete [] real;
  delete [] imag;
  
  return true;
}

