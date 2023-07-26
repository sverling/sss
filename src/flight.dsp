# Microsoft Developer Studio Project File - Name="flight" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=flight - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flight.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flight.mak" CFG="flight - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "flight - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "flight - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "flight - Win32 Release stripped" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "flight - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W2 /GR /GX /O2 /Ob2 /D "WITH_FMOD" /D "WITH_PLIB" /D "WITH_GLUI" /D "LOD_MORPH" /D "WITH_GLE" /D "WITH_GL_EXT" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FREEGLUT_STATIC" /Fr /FD /Zm1000 /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 opengl32.lib glu32.lib glaux.lib glui.lib js.lib fnt.lib sg.lib ul.lib ssg.lib fmodex_vc.lib gle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libpng.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc.lib" /out:"../sss.exe"
# SUBTRACT LINK32 /profile /map

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W2 /GR /GX /ZI /Od /D "WITH_FMOD" /D "WITH_GLUI" /D "_DEBUG" /D "WITH_GLE" /D "WITH_GL_EXT" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FREEGLUT_STATIC" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mfc42.lib libtiff.lib glui.lib js_d.lib sl_d.lib fnt_d.lib sg_d.lib ul_d.lib ssg_d.lib fmodex_vc.lib gle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libpng_dbg.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../sss_debug.exe" /fixed:no
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "flight___Win32_Release_stripped"
# PROP BASE Intermediate_Dir "flight___Win32_Release_stripped"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Stripped"
# PROP Intermediate_Dir "Stripped"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GR /GX /O2 /Ob2 /D "WITH_PLIB" /D "WITH_GLUI" /D "WITH_GLE" /D "LOD_MORPH" /D "WITH_GL_EXT" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm1000 /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /Ob2 /D "LOD_MORPH" /D "WITH_GLE" /D "WITH_GL_EXT" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FREEGLUT_STATIC" /Fr /YX /FD /Zm1000 /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 glu32.lib glaux.lib opengl32.lib gle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glui.lib ws2_32.lib js.lib sl.lib fnt.lib sg.lib ul.lib ssg.lib /nologo /subsystem:console /profile /map /machine:I386 /nodefaultlib:"libc.lib" /out:"Release/flight.exe"
# ADD LINK32 glu32.lib glaux.lib opengl32.lib gle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libpng.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc.lib" /out:"../sss_stripped.exe"
# SUBTRACT LINK32 /profile /map

!ENDIF 

# Begin Target

# Name "flight - Win32 Release"
# Name "flight - Win32 Debug"
# Name "flight - Win32 Release stripped"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\3ds.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\aerofoil.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\audio.cpp
# End Source File
# Begin Source File

SOURCE=.\bludger.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\body.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\config_file.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\control_method.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\environment.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Explosion.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fft2d.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fuselage.cpp
# End Source File
# Begin Source File

SOURCE=.\glider.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_aero.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_aero_component.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_aero_crrcsim.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_engine.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_graphics.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_graphics_3ds.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_graphics_component.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_graphics_crrcsim.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_power.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_structure.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_structure_3ds.cpp
# End Source File
# Begin Source File

SOURCE=.\glider_structure_component.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glider_structure_crrcsim.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\graphics_3ds.cpp
# End Source File
# Begin Source File

SOURCE=.\gui.cpp
# End Source File
# Begin Source File

SOURCE=.\gyro.cpp
# End Source File
# Begin Source File

SOURCE=.\heli_controller.cpp
# End Source File
# Begin Source File

SOURCE=.\image_from_file.cpp
# End Source File
# Begin Source File

SOURCE=.\joystick.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lod.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\log_trace.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\missile.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\particle_engine.cpp
# End Source File
# Begin Source File

SOURCE=.\particle_source.cpp
# End Source File
# Begin Source File

SOURCE=.\physics.cpp
# End Source File
# Begin Source File

SOURCE=.\pilot_manager.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\propeller.cpp
# End Source File
# Begin Source File

SOURCE=.\race_manager.cpp
# End Source File
# Begin Source File

SOURCE=.\remote_sss_iface.cpp
# End Source File
# Begin Source File

SOURCE=.\remote_sss_queue.cpp
# End Source File
# Begin Source File

SOURCE=.\renderer.cpp
# End Source File
# Begin Source File

SOURCE=.\robot_pilot.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ski.cpp
# End Source File
# Begin Source File

SOURCE=.\sss.cpp
# End Source File
# Begin Source File

SOURCE=.\sss_socket.cpp
# End Source File
# Begin Source File

SOURCE=.\terrain_generator.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\text_overlay.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\texture.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\thermal.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\thermal_manager.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tracer.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tree.cpp
# End Source File
# Begin Source File

SOURCE=.\tree_collection.cpp
# End Source File
# Begin Source File

SOURCE=.\tx_audio_input.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vario.cpp
# End Source File
# Begin Source File

SOURCE=.\vertex.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wind_field_generator.cpp

!IF  "$(CFG)" == "flight - Win32 Release"

!ELSEIF  "$(CFG)" == "flight - Win32 Debug"

# SUBTRACT CPP /Fr

!ELSEIF  "$(CFG)" == "flight - Win32 Release stripped"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\3ds.h
# End Source File
# Begin Source File

SOURCE=.\aerofoil.h
# End Source File
# Begin Source File

SOURCE=.\array_2d.h
# End Source File
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\bludger.h
# End Source File
# Begin Source File

SOURCE=.\body.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\config_file.h
# End Source File
# Begin Source File

SOURCE=.\control_method.h
# End Source File
# Begin Source File

SOURCE=.\environment.h
# End Source File
# Begin Source File

SOURCE=.\Explosion.h
# End Source File
# Begin Source File

SOURCE=.\fft2d.h
# End Source File
# Begin Source File

SOURCE=.\fuselage.h
# End Source File
# Begin Source File

SOURCE=.\glider.h
# End Source File
# Begin Source File

SOURCE=.\glider_aero.h
# End Source File
# Begin Source File

SOURCE=.\glider_aero_component.h
# End Source File
# Begin Source File

SOURCE=.\glider_aero_crrcsim.h
# End Source File
# Begin Source File

SOURCE=.\glider_engine.h
# End Source File
# Begin Source File

SOURCE=.\glider_graphics.h
# End Source File
# Begin Source File

SOURCE=.\glider_graphics_3ds.h
# End Source File
# Begin Source File

SOURCE=.\glider_graphics_component.h
# End Source File
# Begin Source File

SOURCE=.\glider_graphics_crrcsim.h
# End Source File
# Begin Source File

SOURCE=.\glider_power.h
# End Source File
# Begin Source File

SOURCE=.\glider_structure.h
# End Source File
# Begin Source File

SOURCE=.\glider_structure_3ds.h
# End Source File
# Begin Source File

SOURCE=.\glider_structure_component.h
# End Source File
# Begin Source File

SOURCE=.\glider_structure_crrcsim.h
# End Source File
# Begin Source File

SOURCE=.\graphics_3ds.h
# End Source File
# Begin Source File

SOURCE=.\gui.h
# End Source File
# Begin Source File

SOURCE=.\gyro.h
# End Source File
# Begin Source File

SOURCE=.\heli_controller.h
# End Source File
# Begin Source File

SOURCE=.\image_from_file.h
# End Source File
# Begin Source File

SOURCE=.\joystick.h
# End Source File
# Begin Source File

SOURCE=.\lod.h
# End Source File
# Begin Source File

SOURCE=.\log_trace.h
# End Source File
# Begin Source File

SOURCE=.\matrix_vector3.h
# End Source File
# Begin Source File

SOURCE=.\matrix_vector3.inl
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\missile.h
# End Source File
# Begin Source File

SOURCE=.\my_glut.h
# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\particle_engine.h
# End Source File
# Begin Source File

SOURCE=.\particle_source.h
# End Source File
# Begin Source File

SOURCE=.\physics.h
# End Source File
# Begin Source File

SOURCE=.\pilot.h
# End Source File
# Begin Source File

SOURCE=.\pilot_manager.h
# End Source File
# Begin Source File

SOURCE=.\plane.h
# End Source File
# Begin Source File

SOURCE=.\propeller.h
# End Source File
# Begin Source File

SOURCE=.\race_manager.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_body_msg.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_explosion_msg.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_glider_msg.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_iface.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_missile_msg.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_msg.h
# End Source File
# Begin Source File

SOURCE=.\remote_sss_queue.h
# End Source File
# Begin Source File

SOURCE=.\renderer.h
# End Source File
# Begin Source File

SOURCE=.\robot_pilot.h
# End Source File
# Begin Source File

SOURCE=.\ski.h
# End Source File
# Begin Source File

SOURCE=.\sss.h
# End Source File
# Begin Source File

SOURCE=.\sss_assert.h
# End Source File
# Begin Source File

SOURCE=.\sss_glut.h
# End Source File
# Begin Source File

SOURCE=.\sss_random.h
# End Source File
# Begin Source File

SOURCE=.\sss_socket.h
# End Source File
# Begin Source File

SOURCE=.\terrain_generator.h
# End Source File
# Begin Source File

SOURCE=.\text_overlay.h
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\thermal.h
# End Source File
# Begin Source File

SOURCE=.\thermal_manager.h
# End Source File
# Begin Source File

SOURCE=.\tracer.h
# End Source File
# Begin Source File

SOURCE=.\tree.h
# End Source File
# Begin Source File

SOURCE=.\tree_collection.h
# End Source File
# Begin Source File

SOURCE=.\tx_audio_input.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# Begin Source File

SOURCE=.\vario.h
# End Source File
# Begin Source File

SOURCE=.\vertex.h
# End Source File
# Begin Source File

SOURCE=.\wind_field_generator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Config files"

# PROP Default_Filter ".cfg"
# Begin Source File

SOURCE=..\sss.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_big_demo.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_cherhill.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_debug.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_demo_high_detail.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_demo_med_detail.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_ds.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_hangglider.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_heli.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_learn_to_fly.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_no_3d.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_oliver.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_race.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_skybox.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_sledge_high_detail.cfg
# End Source File
# Begin Source File

SOURCE=..\sss_sledge_med_detail.cfg
# End Source File
# End Group
# Begin Group "Gliders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gliders\glider_A10.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_aileron.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_aileron_rudder.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_broomstick.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_corsair.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_allegro.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_apogee.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_biplane2.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_f3f.dat
# End Source File
# Begin Source File

SOURCE="..\gliders\glider_crrcsim_k2-f3f.dat"
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_k2.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_outrage.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_sovereign.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_spirit.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_sport.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_superzagi.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_zagi.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_zip60.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_crrcsim_zipper.dat
# End Source File
# Begin Source File

SOURCE="..\gliders\glider_deltaflex-1.dat"
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_deltaflex.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_eagle.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_f18.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_f3b.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_F86.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_fast.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_hawk.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_heli.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_heli_3ds.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_hg1.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_magpie.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_mustang.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_neoslope.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_phase6.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_rudder.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_scale.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_sierra_mk2.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_sledge.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_sledge2.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_twinjet.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_v.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_v_tail.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_wing.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_wizard.dat
# End Source File
# Begin Source File

SOURCE=..\gliders\glider_xs142pink.dat
# End Source File
# End Group
# End Target
# End Project
