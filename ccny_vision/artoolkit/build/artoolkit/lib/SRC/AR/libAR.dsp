# Microsoft Developer Studio Project File - Name="libAR" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libAR - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libAR.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libAR.mak" CFG="libAR - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libAR - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libAR - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libAR - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libAR___Win32_Release"
# PROP BASE Intermediate_Dir "libAR___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\libAR.lib"

!ELSEIF  "$(CFG)" == "libAR - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libAR___Win32_Debug"
# PROP BASE Intermediate_Dir "libAR___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\libARd.lib"

!ENDIF 

# Begin Target

# Name "libAR - Win32 Release"
# Name "libAR - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\arDetectMarker.c
# End Source File
# Begin Source File

SOURCE=.\arDetectMarker2.c
# End Source File
# Begin Source File

SOURCE=.\arGetCode.c
# End Source File
# Begin Source File

SOURCE=.\arGetMarkerInfo.c
# End Source File
# Begin Source File

SOURCE=.\arGetTransMat.c
# End Source File
# Begin Source File

SOURCE=.\arGetTransMat2.c
# End Source File
# Begin Source File

SOURCE=.\arGetTransMat3.c
# End Source File
# Begin Source File

SOURCE=.\arGetTransMatCont.c
# End Source File
# Begin Source File

SOURCE=.\arLabeling.c
# End Source File
# Begin Source File

SOURCE=.\arUtil.c
# End Source File
# Begin Source File

SOURCE=.\mAlloc.c
# End Source File
# Begin Source File

SOURCE=.\mAllocDup.c
# End Source File
# Begin Source File

SOURCE=.\mAllocInv.c
# End Source File
# Begin Source File

SOURCE=.\mAllocMul.c
# End Source File
# Begin Source File

SOURCE=.\mAllocTrans.c
# End Source File
# Begin Source File

SOURCE=.\mAllocUnit.c
# End Source File
# Begin Source File

SOURCE=.\mDet.c
# End Source File
# Begin Source File

SOURCE=.\mDisp.c
# End Source File
# Begin Source File

SOURCE=.\mDup.c
# End Source File
# Begin Source File

SOURCE=.\mFree.c
# End Source File
# Begin Source File

SOURCE=.\mInv.c
# End Source File
# Begin Source File

SOURCE=.\mMul.c
# End Source File
# Begin Source File

SOURCE=.\mPCA.c
# End Source File
# Begin Source File

SOURCE=.\mSelfInv.c
# End Source File
# Begin Source File

SOURCE=.\mTrans.c
# End Source File
# Begin Source File

SOURCE=.\mUnit.c
# End Source File
# Begin Source File

SOURCE=.\paramChangeSize.c
# End Source File
# Begin Source File

SOURCE=.\paramDecomp.c
# End Source File
# Begin Source File

SOURCE=.\paramDisp.c
# End Source File
# Begin Source File

SOURCE=.\paramDistortion.c
# End Source File
# Begin Source File

SOURCE=.\paramFile.c
# End Source File
# Begin Source File

SOURCE=.\paramGet.c
# End Source File
# Begin Source File

SOURCE=.\vAlloc.c
# End Source File
# Begin Source File

SOURCE=.\vDisp.c
# End Source File
# Begin Source File

SOURCE=.\vFree.c
# End Source File
# Begin Source File

SOURCE=.\vHouse.c
# End Source File
# Begin Source File

SOURCE=.\vInnerP.c
# End Source File
# Begin Source File

SOURCE=.\vTridiag.c
# End Source File
# End Group
# End Target
# End Project
