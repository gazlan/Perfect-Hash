# Microsoft Developer Studio Project File - Name="cmph" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=cmph - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cmph.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cmph.mak" CFG="cmph - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cmph - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cmph - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cmph - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x419 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "cmph - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x419 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "cmph - Win32 Release"
# Name "cmph - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bdz.cpp
# End Source File
# Begin Source File

SOURCE=.\bdz_ph.cpp
# End Source File
# Begin Source File

SOURCE=.\bmz.cpp
# End Source File
# Begin Source File

SOURCE=.\bmz8.cpp
# End Source File
# Begin Source File

SOURCE=.\brz.cpp
# End Source File
# Begin Source File

SOURCE=.\buffer_entry.cpp
# End Source File
# Begin Source File

SOURCE=.\buffer_manager.cpp
# End Source File
# Begin Source File

SOURCE=.\chd.cpp
# End Source File
# Begin Source File

SOURCE=.\chd_ph.cpp
# End Source File
# Begin Source File

SOURCE=.\chm.cpp
# End Source File
# Begin Source File

SOURCE=.\cmph.cpp
# End Source File
# Begin Source File

SOURCE=.\cmph_structs.cpp
# End Source File
# Begin Source File

SOURCE=.\compressed_rank.cpp
# End Source File
# Begin Source File

SOURCE=.\compressed_seq.cpp
# End Source File
# Begin Source File

SOURCE=.\fch.cpp
# End Source File
# Begin Source File

SOURCE=.\fch_buckets.cpp
# End Source File
# Begin Source File

SOURCE=.\graph.cpp
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\jenkins_hash.cpp
# End Source File
# Begin Source File

SOURCE=.\linear_string_map.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\miller_rabin.cpp

!IF  "$(CFG)" == "cmph - Win32 Release"

!ELSEIF  "$(CFG)" == "cmph - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Shared\mmf.cpp
# End Source File
# Begin Source File

SOURCE=.\select.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\vqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\vstack.cpp
# End Source File
# Begin Source File

SOURCE=.\wingetopt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bdz.h
# End Source File
# Begin Source File

SOURCE=.\bdz_ph.h
# End Source File
# Begin Source File

SOURCE=.\bdz_structs.h
# End Source File
# Begin Source File

SOURCE=.\bdz_structs_ph.h
# End Source File
# Begin Source File

SOURCE=.\bitbool.h
# End Source File
# Begin Source File

SOURCE=.\bmz.h
# End Source File
# Begin Source File

SOURCE=.\bmz8.h
# End Source File
# Begin Source File

SOURCE=.\bmz8_structs.h
# End Source File
# Begin Source File

SOURCE=.\bmz_structs.h
# End Source File
# Begin Source File

SOURCE=.\brz.h
# End Source File
# Begin Source File

SOURCE=.\brz_structs.h
# End Source File
# Begin Source File

SOURCE=.\buffer_entry.h
# End Source File
# Begin Source File

SOURCE=.\buffer_manager.h
# End Source File
# Begin Source File

SOURCE=.\chd.h
# End Source File
# Begin Source File

SOURCE=.\chd_ph.h
# End Source File
# Begin Source File

SOURCE=.\chd_structs.h
# End Source File
# Begin Source File

SOURCE=.\chd_structs_ph.h
# End Source File
# Begin Source File

SOURCE=.\chm.h
# End Source File
# Begin Source File

SOURCE=.\chm_structs.h
# End Source File
# Begin Source File

SOURCE=.\cmph.h
# End Source File
# Begin Source File

SOURCE=.\cmph_structs.h
# End Source File
# Begin Source File

SOURCE=.\cmph_time.h
# End Source File
# Begin Source File

SOURCE=.\cmph_types.h
# End Source File
# Begin Source File

SOURCE=.\compressed_rank.h
# End Source File
# Begin Source File

SOURCE=.\compressed_seq.h
# End Source File
# Begin Source File

SOURCE=.\fch.h
# End Source File
# Begin Source File

SOURCE=.\fch_buckets.h
# End Source File
# Begin Source File

SOURCE=.\fch_structs.h
# End Source File
# Begin Source File

SOURCE=..\Shared\file_find.cpp
# End Source File
# Begin Source File

SOURCE=..\Shared\file_find.h
# End Source File
# Begin Source File

SOURCE=.\graph.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\hash_state.h
# End Source File
# Begin Source File

SOURCE=.\jenkins_hash.h
# End Source File
# Begin Source File

SOURCE=.\linear_string_map.h
# End Source File
# Begin Source File

SOURCE=.\miller_rabin.h
# End Source File
# Begin Source File

SOURCE=..\Shared\mmf.h
# End Source File
# Begin Source File

SOURCE=.\select.h
# End Source File
# Begin Source File

SOURCE=.\select_lookup_tables.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\vqueue.h
# End Source File
# Begin Source File

SOURCE=.\vstack.h
# End Source File
# Begin Source File

SOURCE=.\wingetopt.h
# End Source File
# End Group
# End Target
# End Project
