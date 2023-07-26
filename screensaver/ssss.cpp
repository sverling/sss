// screensaver_test.cpp : Defines the entry point for the application.
//

//#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <iostream>
#include <fstream>

using namespace std;
 
enum TScrMode {smNone,smConfig,smPassword,smPreview,smSaver};
TScrMode ScrMode=smNone;


//==============================================================
// WinMain
//==============================================================
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  // decide what to do
  char *c=GetCommandLine();
  if (*c=='\"') 
  {
    c++; 
    while (*c!=0 && *c!='\"') 
      c++;
  }
  else 
  {
    while (*c!=0 && *c!=' ') 
      c++;
  }
  if (*c!=0) 
    c++;
  while (*c==' ') 
    c++;

  HWND hwnd=NULL;
  if (*c==0) 
  {
    ScrMode=smConfig; 
    hwnd=NULL;
  }
  else
  {
    if (*c=='-' || *c=='/') 
      c++;
    if (*c=='p' || *c=='P' || *c=='l' || *c=='L')
    { 
      c++; 
      while (*c==' ' || *c==':') 
        c++;
      ScrMode=smPreview;
    }
    else if (*c=='s' || *c=='S') 
    {
      ScrMode=smSaver; 
    }
    else if (*c=='c' || *c=='C') 
    {
      c++; 
      while (*c==' ' || *c==':') 
        c++;
      ScrMode=smConfig;
    }
    else if (*c=='a' || *c=='A') 
    {
      c++; 
      while (*c==' ' || *c==':') 
        c++;
      ScrMode=smPassword;
    }
  }

  // running as full screensaver?
  if (ScrMode == smSaver)
  {
    // use SSSS.cfg to tell us where sss lives
    ifstream ssss_cfg("C:/SSSS.cfg");
    streampos pos_seq = ssss_cfg.tellg();

    char c;
    if (!ssss_cfg.get(c))
    {
      MessageBox(NULL, "Unable to open C:/SSSS.cfg", "Oh Dear", MB_OK);
      return 1;
    }
    ssss_cfg.seekg(pos_seq);

    // get the first line
    char char_line[1024];
    if (!ssss_cfg.getline(char_line, 255))
    {
      MessageBox(NULL, "opened C:/SSSS.cfg but unable to read the first line!", "Oh Dear", MB_OK);
      return 1;
    }

    if (0 != chdir(char_line))
    {
      char msg[1280];
      sprintf(msg, "Unable to change directory to %s\n", char_line);
      MessageBox(NULL, msg, "Oh Dear", MB_OK);
      return 1;
    }

    // let's go

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    if( !CreateProcess( NULL, // No module name (use command line). 
        "sss.exe sss_screensaver.cfg", // Command line. 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
        0,                // No creation flags. 
        NULL,             // Use parent's environment block. 
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi )             // Pointer to PROCESS_INFORMATION structure.
    ) 
    {
      MessageBox(NULL, "Unable to run sss.exe", "Oh Dear", MB_OK);
      return 1;
    }

    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
  }
  return 0;
}



