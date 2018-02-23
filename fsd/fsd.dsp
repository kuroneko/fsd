# Microsoft Developer Studio Project File - Name="fsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=fsd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "fsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsd.mak" CFG="fsd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "fsd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "fsd - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "../shared" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D FD_SETSIZE=1024 /YX /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386 /out:"../windows/fsd.exe"

!ELSEIF  "$(CFG)" == "fsd - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../shared" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D FD_SETSIZE=1024 /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "fsd - Win32 Release"
# Name "fsd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\authenticate.c
# End Source File
# Begin Source File

SOURCE=.\certificate.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\clinterface.cpp
# End Source File
# Begin Source File

SOURCE=.\cluser.cpp
# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\fsd.cpp
# End Source File
# Begin Source File

SOURCE=.\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\manage.cpp
# End Source File
# Begin Source File

SOURCE=.\mm.cpp
# End Source File
# Begin Source File

SOURCE=.\process.cpp
# End Source File
# Begin Source File

SOURCE=.\server.cpp
# End Source File
# Begin Source File

SOURCE=.\servinterface.cpp
# End Source File
# Begin Source File

SOURCE=.\servuser.cpp
# End Source File
# Begin Source File

SOURCE=.\support.cpp
# End Source File
# Begin Source File

SOURCE=.\sysinterface.cpp
# End Source File
# Begin Source File

SOURCE=.\sysuser.cpp
# End Source File
# Begin Source File

SOURCE=.\user.cpp
# End Source File
# Begin Source File

SOURCE=.\wprofile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\attributes.h
# End Source File
# Begin Source File

SOURCE=.\authenticate.h
# End Source File
# Begin Source File

SOURCE=.\certificate.h
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\clinterface.h
# End Source File
# Begin Source File

SOURCE=.\cluser.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\fsd.h
# End Source File
# Begin Source File

SOURCE=.\fsdpaths.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\interface.h
# End Source File
# Begin Source File

SOURCE=.\manage.h
# End Source File
# Begin Source File

SOURCE=.\mm.h
# End Source File
# Begin Source File

SOURCE=.\process.h
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\servinterface.h
# End Source File
# Begin Source File

SOURCE=.\servuser.h
# End Source File
# Begin Source File

SOURCE=.\support.h
# End Source File
# Begin Source File

SOURCE=.\sysinterface.h
# End Source File
# Begin Source File

SOURCE=.\sysuser.h
# End Source File
# Begin Source File

SOURCE=.\user.h
# End Source File
# Begin Source File

SOURCE=.\wprofile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
