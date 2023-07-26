#include <stdio.h>
#include <stdlib.h>

#include <tiffio.h>

int main(int argc, char * argv[])
{
  /* Input */
  char * tiff_file;
  TIFF * tiff;
  int rv, i, j;

  uint32 height, width;
  uint32 * raster;
  uint16 bits_per_sample;
  
  /* output */
  FILE * orog_file;
  float dx, dy, zmin, zmax;

  float * orog_x;
  float * orog_y;
  float * orog_z;
  float xmin, ymin;
  
  /* Extract args */
  if (argc != 5)
  {
    fprintf(stderr, "Usage: %s dx zmin zmax input_file.bw\n", argv[0]);
    fprintf(stderr, "       Writes output to orography.dat\n");
    exit (-1);
  }
  
  dx   = atof(argv[1]);
  dy   = dx;
  zmin = atof(argv[2]);
  zmax = atof(argv[3]);
  tiff_file = argv[4];
  
  /* Open tiff file */
  if (0 == (tiff = TIFFOpen(tiff_file, "r")))
  {
    fprintf(stderr, "Failed to read file %s\n", tiff_file);
    exit(-1);
  }

  rv = TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
  rv = TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  rv = TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
  
  printf("width = %d, height = %d, bits_per_sample = %d\n", 
         width, 
         height, 
         bits_per_sample);
  
  /* validate this info */
  if (width != height)
  {
    fprintf(stderr, "width = %d, height = %d: must be square!\n", width, height);
    exit(-1);
  }
  if (bits_per_sample != 8)
  {
    fprintf(stderr, "Bits per sample must = 8: i.e. TIFF file must be greyscale\n");
    exit(-1);
  }

  raster = (uint32 *) malloc(height * width * 8);

  if (0 == TIFFReadRGBAImage(tiff, 
                             width,
                             height,
                             raster, 
                             0))
  {
    fprintf(stderr, "Unable to extract image from TIFF\n");
    exit(-1);
  }

  printf("Read TIFF file\n");

  orog_x = (float *) malloc(sizeof(float) * width);
  orog_y = (float *) malloc(sizeof(float) * height);
  orog_z = (float *) malloc(sizeof(float) * width * height);

  xmin = -0.5 * dx * width;
  ymin = -0.5 * dy * height;
  
#define CALC_INDEX(i, j) (j*width + i)
  for (i = 0 ; i < width ; ++i)
  {
    orog_x[i] = xmin + i * dx;
    for (j = 0 ; j < height ; ++j)
    {
      orog_y[j] = ymin + j * dy;
      orog_z[CALC_INDEX(i,j)] = zmin + 
        (zmax-zmin) * ((float) TIFFGetR(raster[CALC_INDEX(i,j)])/255);
    }
  }
  
  orog_file = fopen("orography.dat", "w");
  if (!orog_file)
  {
    fprintf(stderr, "Unable to open %s\n", "orography.dat");
    exit(-1);
  }

  fwrite(&width, sizeof(width), 1, orog_file);
  fwrite(&height, sizeof(height), 1, orog_file);
  fwrite(&orog_x[0], sizeof(orog_x[0]), width, orog_file);
  fwrite(&orog_y[0], sizeof(orog_y[0]), height, orog_file);
  fwrite(&orog_z[0], sizeof(orog_z[0]), width*height, orog_file);

  fclose(orog_file);
  printf("Written orography.dat\n");

  return 0;
}



