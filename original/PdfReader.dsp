# Microsoft Developer Studio Project File - Name="PdfReader" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=PdfReader - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PdfReader.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PdfReader.mak" CFG="PdfReader - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PdfReader - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "PdfReader - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PdfReader - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "PdfReader - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PdfReader - Win32 Release"
# Name "PdfReader - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\zlib\Adler32.c
# End Source File
# Begin Source File

SOURCE=.\cdate\cdate.cpp
# End Source File
# Begin Source File

SOURCE=.\chashtable\chashtable.cpp
# End Source File
# Begin Source File

SOURCE=.\cmd5\cmd5.cpp
# End Source File
# Begin Source File

SOURCE=.\cstring\cstring.cpp
# End Source File
# Begin Source File

SOURCE=.\cvector\cvector.cpp
# End Source File
# Begin Source File

SOURCE=.\zlib\DEFLATE.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFBLOCK.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFCODES.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFFAST.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFLATE.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFTREES.C
# End Source File
# Begin Source File

SOURCE=.\zlib\INFUTIL.C
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFCoordinate.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFDocumentStructure.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFFile.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFFileStructure.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFObjectRef.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\PDFPageDescription.cpp
# End Source File
# Begin Source File

SOURCE=.\zlib\TREES.C
# End Source File
# Begin Source File

SOURCE=.\zlib\zlibengn.cpp
# End Source File
# Begin Source File

SOURCE=.\zlib\ZUTIL.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cdate\cdate.h
# End Source File
# Begin Source File

SOURCE=.\chashtable\chashtable.h
# End Source File
# Begin Source File

SOURCE=.\cmd5\cmd5.h
# End Source File
# Begin Source File

SOURCE=.\cstring\cstring.h
# End Source File
# Begin Source File

SOURCE=.\cvector\cvector.h
# End Source File
# Begin Source File

SOURCE=.\zlib\DEFLATE.H
# End Source File
# Begin Source File

SOURCE=.\zlib\INFBLOCK.H
# End Source File
# Begin Source File

SOURCE=.\zlib\INFCODES.H
# End Source File
# Begin Source File

SOURCE=.\zlib\INFFAST.H
# End Source File
# Begin Source File

SOURCE=.\zlib\INFTREES.H
# End Source File
# Begin Source File

SOURCE=.\zlib\INFUTIL.H
# End Source File
# Begin Source File

SOURCE=.\PDFCoordinate.h
# End Source File
# Begin Source File

SOURCE=.\PDFDocumentStructure.h
# End Source File
# Begin Source File

SOURCE=.\PDFFile.h
# End Source File
# Begin Source File

SOURCE=.\PDFFileStructure.h
# End Source File
# Begin Source File

SOURCE=.\PDFObjectRef.h
# End Source File
# Begin Source File

SOURCE=.\PDFObjects.h
# End Source File
# Begin Source File

SOURCE=.\PDFPageDescription.h
# End Source File
# Begin Source File

SOURCE=.\zlib\ZCONF.H
# End Source File
# Begin Source File

SOURCE=.\zlib\ZLIB.H
# End Source File
# Begin Source File

SOURCE=.\zlib\zlibengn.h
# End Source File
# Begin Source File

SOURCE=.\zlib\ZUTIL.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
