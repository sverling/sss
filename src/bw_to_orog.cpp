/*
  converts a greyscale rgb (SGI) file into orography.dat, suitable for
  subsequent use with mk_param

  g++ -o bw_to_orog bw_to_orog.cpp

*/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

unsigned char * read_bw(const char *name,int *w, int *h);

int main(int argc, char * argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, "Usage: %s dx zmin zmax input_file.bw\n", argv[0]);
    fprintf(stderr, "       Writes output to orography.dat\n");
    exit (-1);
  }
  
  float dx   = atof(argv[1]);
  float dy = dx;
  float zmin = atof(argv[2]);
  float zmax = atof(argv[3]);
  const char * bw_file = argv[4];
  
  unsigned char * image;

  int w, h;

  image = read_bw(bw_file, &w, &h);
  
  if (image == 0)
  {
    fprintf(stderr, "Unable to read input file\n");
    exit (-1);
  }

  FILE * orog_file = fopen("orography.dat", "w");
  if (!orog_file)
  {
    fprintf(stderr, "Unable to open %s\n", "orography.dat");
    exit(-1);
  }

  float orog_x[w];
  float orog_y[h];
  float orog_z[h][w];
  for (int i = 0 ; i < w ; ++i)
  {
    for (int j = 0 ; j < h ; ++j)
    {
      orog_x[i] = i * dx;
      orog_y[j] = j * dy;
      orog_z[j][i] = zmin + (zmax-zmin) * ((float) image[i + j*w] /255);
    }
  }

  fwrite(&w, sizeof(w), 1, orog_file);
  fwrite(&h, sizeof(h), 1, orog_file);
  fwrite(&orog_x[0], sizeof(orog_x[0]), w, orog_file);
  fwrite(&orog_y[0], sizeof(orog_y[0]), h, orog_file);
  fwrite(&orog_z[0][0], sizeof(orog_z[0][0]), w*h, orog_file);

  fclose(orog_file);

}

unsigned short getshort(FILE * inf)
{
  unsigned char buf[2];
  
  fread(buf,2,1,inf);
  return (buf[0]<<8)+(buf[1]<<0);
}

unsigned short getrgbchar(FILE * inf)
{
  unsigned char buf[1];
  
  fread(buf,1,1,inf);
  return (buf[0]);
}

unsigned char * read_bw(const char *name,int *w, int *h)
// from crrcsim
{
  unsigned char *image;
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
        *w=input_short;
        input_short=getshort(image_in);
        *h=input_short;
        image=(unsigned char *) malloc(*w * *h *sizeof(unsigned char));
        if (image == NULL)
        {
          printf("Error allocating image.\n");
          exit(0);
        }
        input_short=getshort(image_in);
        if (input_short == 1)
        {
          fread(header,sizeof(unsigned char),500,image_in);
          fread(image, sizeof image[0], *w * *h, image_in);
          
          return image; 
        }
        else
        {
          printf("This file isn't a 4 channel RGBA file.\n");
          exit(0);
        }
      }    
      else
      {
        printf("Not a useable RGB file.\n");
        exit(0);
      }
    }
    else
    {
      printf("RLE encoded SGI files are not supported.\n");
      exit(0);
    }
  }
  else
  {
    printf("File %s doesn't appear to be an SGI rgb file!\n",name);
  }
  return 0;
}

