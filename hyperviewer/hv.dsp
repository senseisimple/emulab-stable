# Microsoft Developer Studio Project File - Name="hv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hv - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hv.mak" CFG="hv - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hv - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(PYTHON_INCLUDE)" /I "hypviewer\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HV_EXPORTS" /D NAMESPACEHACK="using namespace std;" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib "$(PYTHON_LIB)" opengl32.lib glu32.lib /nologo /dll /debug /machine:I386 /out:"_hv.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "hv - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(PYTHON_INCLUDE)" /I "hypviewer\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HV_EXPORTS" /D NAMESPACEHACK="using namespace std;" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib "$(PYTHON_LIB)" opengl32.lib glu32.lib /nologo /dll /machine:I386 /out:"_hv.dll"

!ENDIF 

# Begin Target

# Name "hv - Win32 Debug"
# Name "hv - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\_hv.cpp
# End Source File
# Begin Source File

SOURCE=.\hvmain.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypGroupArray.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypLink.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypNode.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypNodeArray.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypQuat.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypView.cpp
# End Source File
# Begin Source File

SOURCE=.\hypviewer\src\HypViewer.cpp

!IF  "$(CFG)" == "hv - Win32 Debug"

!ELSEIF  "$(CFG)" == "hv - Win32 Release"

# ADD CPP /W3

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\hypviewer\include\HypData.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypGraph.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypGroup.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypGroupArray.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypLink.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypLinkArray.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypNode.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypNodeArray.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypPoint.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypQuat.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypTransform.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypView.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\HypViewer.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\StringArray.h
# End Source File
# Begin Source File

SOURCE=.\hypviewer\include\VkHypViewer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\hv.i

!IF  "$(CFG)" == "hv - Win32 Debug"

# Begin Custom Build
InputPath=.\hv.i
InputName=hv

BuildCmds= \
	echo In order to function correctly, please ensure the following environment variables are correctly set: \
	echo PYTHON_INCLUDE: %PYTHON_INCLUDE% \
	echo PYTHON_LIB: %PYTHON_LIB% \
	echo on \
	swig -c++ -python -DWIN32 -module $(InputName) -o _$(InputName).cpp $(InputPath) \
	

"_$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputName).py" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "hv - Win32 Release"

# Begin Custom Build
InputPath=.\hv.i
InputName=hv

BuildCmds= \
	echo In order to function correctly, please ensure the following environment variables are correctly set: \
	echo PYTHON_INCLUDE: %PYTHON_INCLUDE% \
	echo PYTHON_LIB: %PYTHON_LIB% \
	echo on \
	swig -c++ -python -DWIN32 -module $(InputName) -o _$(InputName).cpp $(InputPath) \
	

"_$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputName).py" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
