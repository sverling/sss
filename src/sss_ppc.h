#ifndef _SSS_PPC_H_
#define _SSS_PPC_H_

#include <sys/types.h>

inline u_int16_t SwapInt16(ushort arg) {
#if defined(__i386__) && defined(__GNUC__)
        __asm__("xchgb %b0, %h0" : "+q" (arg));
         return arg;
#elif defined(__ppc__) && defined(__GNUC__)
         u_int16_t result;
         __asm__("lhbrx %0,0,%1" : "=r" (result) : "r" (&arg), "m" (arg));
         return result;
#else
         u_int16_t result;
         result = ((arg << 8) & 0xFF00) | ((arg >> 8) & 0xFF);
         return result;
#endif
}

inline u_int32_t SwapInt32(u_int32_t arg) {
#if defined(__i386__) && defined(__GNUC__)
        __asm__("bswap %0" : "+r" (arg));
        return arg;
#elif defined(__ppc__) && defined(__GNUC__)
        u_int32_t result;
        __asm__("lwbrx %0,0,%1" : "=r" (result) : "r" (&arg), "m" (arg));
        return result;
#else
        u_int32_t result;
        result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);
        return result;
#endif
}

typedef union float_uint
{
    float f;
    u_int32_t i;
} fswap;

inline float SwapFloat32( float fv )
{ 
  fswap a; a.f = fv;
  a.i = SwapInt32(a.i);
  return a.f;
}
    
#if defined(__ppc__)
#define SSWAP(x) (x)=SwapInt16((x))
#define LSWAP(x) (x)=SwapInt32((x))
#define FSWAP(x) (x)=SwapFloat32((x))
#else
#define SSWAP(x) /**/
#define LSWAP(x) /**/
#define FSWAP(x) /**/
#endif

#endif /**_SSS_PPC_H_**/
