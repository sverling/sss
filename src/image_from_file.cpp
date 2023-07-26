/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2003 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file image_from_file.h
*/
#include "image_from_file.h"
#include "log_trace.h"

#include <stdio.h>
#include <stdlib.h>
#if defined (__ppc__)
#include "sss_ppc.h"
#endif

extern "C" {
#include <png.h>
}

#include <string>
using namespace std;

GLubyte * read_image(const char * name,
                     int & width,
                     int & height)
{
  string file(name);
  if ( (file.find(".png") != file.npos) ||
       (file.find(".PNG") != file.npos))
  {
    TRACE_FILE_IF(2)
      TRACE("Loading PNG image: %s\n", name);
    return read_png_image(name, width, height);
  }
  else if ( (file.find(".rgb") != file.npos) ||
            (file.find(".RGB") != file.npos) ) 
  {
    TRACE_FILE_IF(2)
      TRACE("Loading SGI RGB image: %s\n", name);
    return read_rgba_image(name, width, height);
  }
  TRACE("Invalid image file format? %s\n", name);
  return 0;
}



//======================== RGB stuff ===========================
inline unsigned short int getshort(FILE * inf)
{
#if defined(__ppc__)
  unsigned short val;
  fread(&val,sizeof(unsigned short),1,inf);
  return val;
#else
  unsigned char buf[2];
  fread(buf,2,1,inf);
  return (unsigned short int) (buf[0]<<8)+(buf[1]<<0);
#endif
}

inline GLubyte getrgbchar(FILE * inf)
{
  unsigned char buf;
  fread(&buf,1,1,inf);
  return buf;
}


GLubyte * read_rgba_image(const char * name,
                          int & width,
                          int & height)
{
  GLubyte *image, *temp;
  FILE *image_in;
  unsigned char input_char;
  unsigned short int input_short;
  unsigned char header[512];
  unsigned long int loop;
  
  if ( (image_in = fopen(name, "rb")) == NULL) 
  {
    return 0;
  }
  input_short=getshort(image_in);
  if (input_short == 0x01da)
  {
    input_char=getrgbchar(image_in);
    if (input_char == 0)
    {
      input_char=getrgbchar(image_in);
      input_short=getshort(image_in);
      if (input_short == 3)
      {
        input_short=getshort(image_in);
        width=input_short;
        input_short=getshort(image_in);
        height=input_short;
        TRACE("width height: %d/%d\n", width, height);
        // if it looks like this isn't a trivial texture, display a
        // warning - loading the large textures can be slow.
        static bool done_warning = false;
        if ( (done_warning == false) && ( (width > 256) || (height > 256) ) )
        {
          TRACE("[Large texture - may take a few seconds]");
          done_warning = true;
        }

        // how many bytes per pixel
        input_short=getshort(image_in);
        int bytes_per_pixel = input_short;
        
        if ( (bytes_per_pixel == 3) ||
             (bytes_per_pixel == 4) )
        {
          image = new GLubyte[width * height * 4];
          temp  = new GLubyte[width * height];

          fread(header,sizeof(unsigned char),500,image_in);
          fread(temp, sizeof image[0], width * height, image_in);
          for (loop=0;loop<(unsigned long int)(width * height);loop++)
          {
            image[loop*4+0]=temp[loop];
          }
          fread(temp, sizeof image[0], width * height, image_in);
          for (loop=0;loop<(unsigned long int)(width * height);loop++)
          {
            image[loop*4+1]=temp[loop];
          }
          fread(temp, sizeof image[0], width * height, image_in);
          for (loop=0;loop<(unsigned long int)(width * height);loop++)
          {
            image[loop*4+2]=temp[loop];
          }
          // alpha?
          if (bytes_per_pixel == 4)
          {
            fread(temp, sizeof image[0], width * height, image_in);
            for (loop=0;loop<(unsigned long int)(width * height);loop++)
            {
              image[loop*4+3]=temp[loop];
            }
          }
          else
          {
            for (loop=0;loop<(unsigned long int)(width * height);loop++)
            {
              image[loop*4+3]=255;
            }
          }
          delete [] temp;
          fclose(image_in);
          return image; 
        }
        else
        {
          TRACE("This file isn't a 3 or 4 channel RGB file.\n");
          fclose(image_in);
          return 0;
        }
      }    
      else
      {
        TRACE("Not a useable RGB file.\n");
        fclose(image_in);
        return 0;
      }
    }
    else
    {
      TRACE("RLE encoded SGI files are not supported.\n");
      fclose(image_in);
      return 0;
    }
  }
  else
  {
    TRACE("File %s doesn't appear to be an SGI rgb file!\n",name);
    fclose(image_in);
    return 0;
  }
  return 0;
}

//======================== PNG stuff ===========================
GLubyte * read_png_image(const char * name,
                         int & width,
                         int & height)
{
  FILE *png_file;
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 width32, height32, x, y;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  GLubyte *data;
  unsigned char *row_pointer;
  unsigned int has_alpha;
  
  png_file = fopen(name, "rb");
  if (!png_file) 
  {
    TRACE("Error opening PNG file %s\n", name);
    return 0;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                   NULL, NULL, NULL);
  if (!png_ptr) 
  {
    TRACE("Error reading PNG data from file %s\n", name);
    fclose(png_file);
    return 0;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) 
  {
    TRACE("Error reading PNG info from file %s\n", name);
    fclose(png_file);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return 0;
  }

  png_init_io(png_ptr, png_file);

  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width32, &height32,
               &bit_depth, &color_type, &interlace_type,
               &compression_type, &filter_method);
  width = width32;
  height = height32;

  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
      color_type == PNG_COLOR_MASK_ALPHA) 
  {
    TRACE("Has alpha ");
    has_alpha = 1;
  }
  else 
  {
    TRACE("Has no alpha ");
    has_alpha = 0;
  }

  data = new GLubyte[width * height * (3 + has_alpha)];

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (bit_depth < 8)
    png_set_packing(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  row_pointer = data;
  for (y = 0; y < (png_uint_32) height; y++) 
  {
    png_read_row(png_ptr, row_pointer, NULL);
    row_pointer += width * (3 + has_alpha);
  }
  fclose(png_file);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  // now copy the image into something that definitely has alpha, and
  // flip it as well.
  GLubyte * image = new GLubyte[width * height * 4];
  for (y = 0 ; y < (png_uint_32) height ; ++y)
  {
    for (x = 0 ; x < (png_uint_32) width ; ++x)
    {
      image[(x + y*width)*4 + 0] = data[(x + (height - y - 1)*width)*(3+has_alpha) + 0];
      image[(x + y*width)*4 + 1] = data[(x + (height - y - 1)*width)*(3+has_alpha) + 1];
      image[(x + y*width)*4 + 2] = data[(x + (height - y - 1)*width)*(3+has_alpha) + 2];
      if (has_alpha == 1)
        image[(x + y*width)*4 + 3] = data[(x + (height - y - 1)*width)*(3+has_alpha) + 3];
      else
        image[(x + y*width)*4 + 3] = 255;
    }
  }
  
  delete [] data;
  
  return image;
}
