/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2003 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file image_from_file.h
*/
#include "sss_glut.h"

/// Works out the image type from the name (last 3 characters)
GLubyte * read_image(const char * name,
                     int & width,
                     int & height);

GLubyte * read_rgba_image(const char * name,
                        int & width,
                        int & height);

GLubyte * read_png_image(const char * name,
                         int & width,
                         int & height);

