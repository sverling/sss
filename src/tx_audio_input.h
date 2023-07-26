/*
  Header file TX_Audio_Input.h
  CRRCSIM Audio interface added to Slope Soaring Simulator by Mark Cassidy
*/
#ifndef TX_AUDIO_INPUT_H
#define TX_AUDIO_INPUT_H
class TX_Audio_Input{
public:
    int Get_TX_Audio(float *values,int *nvalues);
    
private:
    static int audio_acq(float *a,long nval);
};

// stub things out for non-windows builds

#ifndef WIN32
int TX_Audio_Input::Get_TX_Audio(float *values,int *nvalues)
{
  *nvalues = 6;
  for (int i = 0 ; i < 6 ; ++i) values[0] = 0;
  return 0;
}

int TX_Audio_Input::audio_acq(float *a,long nval)
{
  return 0;
}

#endif

#endif
