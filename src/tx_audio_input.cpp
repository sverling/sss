/*
  C++ file TX_Audio_Input.cpp
  CRRCSIM Audio interface added to Slope Soaring Simulator by Mark Cassidy

  Only build for WIN32
*/

/*===========================================================================
  AUDIO.C
  
  interface avec carte son sous windows
  ============================================================================*/
#ifdef WIN32

#include<windows.h>
#include<mmsystem.h> 
#include<stdio.h>
#include"tx_audio_input.h"

int TX_Audio_Input::audio_acq(float *a,long nval)
{
  static int inited=0, nchanels;
  static WAVEFORMATEX      waveformt;
  static WAVEHDR        waveheaderIN;
  static HWAVEIN      handleIN;
  short * wReadPtr; /* en mode 16 bits*/
  UINT err;
  int i,n;
  if(!inited){
    inited=1;
        
    n =waveInGetNumDevs();
    if(!n){printf("no audio card detected\n"); return 1;}
    //waveInGetDevCaps(FIRST_DRIVER,&Caps,sizeof(Caps));
    nchanels=1;
    /*load pcm wave format*/
    waveformt.wBitsPerSample=16;
    waveformt.wFormatTag=WAVE_FORMAT_PCM;
    waveformt.nChannels=nchanels;
    waveformt.nSamplesPerSec = 44100;
    waveformt.nBlockAlign=nchanels*2;
    waveformt.nAvgBytesPerSec = waveformt.nBlockAlign*waveformt.nSamplesPerSec;
    waveformt.cbSize=0;
    /*open driver WAVE IN */
    err=waveInOpen (&handleIN ,WAVE_MAPPER/*FIRST_DRIVER*/,
                    &waveformt,0L ,0L,CALLBACK_NULL);
    if(err) return 100+err;
        
    /*{ HMIXER mixerh;
      err= mixerOpen(&mixerh,(UINT)handleIN,NULL,0,MIXER_OBJECTF_HWAVEIN);
      if(err) return;
      } */
        
    waveheaderIN.lpData=(char*)malloc(nval*4);
    waveheaderIN.dwBufferLength=nval*2;
    waveheaderIN.dwBytesRecorded=0;
    waveheaderIN.dwUser=0;
    waveheaderIN.dwFlags=0;
    err=waveInPrepareHeader(handleIN,&waveheaderIN,sizeof(waveheaderIN));
    if(err) return 7;
    err=waveInAddBuffer(handleIN,&waveheaderIN,sizeof(waveheaderIN));
    if(err) return 8;
    /*Start acq*/
    if(waveInStart(handleIN)) return 9;
  }
  if((waveheaderIN.dwFlags & WHDR_DONE)){
    wReadPtr=(short*)waveheaderIN.lpData;
    for(i=0;i<nval;i++){*a=(float)*wReadPtr; *wReadPtr=0 ;wReadPtr++;a++;}
    waveheaderIN.dwBytesRecorded=0;
    waveInReset( handleIN);
    waveInAddBuffer(handleIN,&waveheaderIN,sizeof(waveheaderIN));
    waveInStart(handleIN);
    return 0;
  }
  else return (-1);
}

int TX_Audio_Input::Get_TX_Audio(float *values,int *nvalues)
{
  float x, px, max, min, moy;
  int imp;
  int err, nval = 2000;
  int i, chanel;
  float time, dt;
  float sig [2000];
    
  err=audio_acq(sig,nval);
  if(err==0){
    max= -100000;
    min=  100000;
    for(i=0;i<nval;i++){
      x = sig[i];
      if (x>max) max=x;
      if (x<min) min=x;
    }
    moy = (max+min)/2;
    x=0;
    chanel=-1; time = 0;
    for(i=0;i<nval;i++){
      time++;
      if (time >100){
        if (chanel < 0) chanel=0; //start
        if(chanel>0) break;//end
      }
      px=x;
      x = sig[i];
      if((x>moy) && (px<moy) ) imp = 1; else imp = 0;
      if(imp){
        dt = (moy-px)/(x-px);
        if (chanel>=0){
          if(chanel>0) {
            values[chanel-1]=2*((time+dt)/44.1F-1.5F);
          }
          chanel ++;
          if(chanel>6) break;
        }
        time = -dt;
      }
    }
  }
  else if(err>0) printf("error in audio device\n");
  *nvalues=chanel-1;
  return !err;
}

#endif
