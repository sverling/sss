// Work around the way that Mac OSX puts GLUT in GLUT/glut.h, instead
// of GL/glut.h (which is where it should be!).

#ifndef SSS_GLUT_H
#define SSS_GLUT_H

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif  // __APPLE__

#endif
