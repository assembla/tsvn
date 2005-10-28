@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set starttime=%time%
set startdir=%cd%
rem
rem build script for TortoiseSVN
rem
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call TortoiseVars.bat

set INCLUDE=%~dp0ext\svn-win32-libintl\inc;%INCLUDE%
set LIB=%~dp0ext\svn-win32-libintl\lib;%LIB%

if "%1"=="" (
  SET _RELEASE=ON
  SET _DEBUG=ON
  SET _DOCS=ON
  SET _SETUP=ON
)

:getparam
if "%1"=="release" SET _RELEASE=ON
if "%1"=="RELEASE" SET _RELEASE=ON
if "%1"=="debug" SET _DEBUG=ON
if "%1"=="DEBUG" SET _DEBUG=ON
if "%1"=="doc" SET _DOCS=ON
if "%1"=="DOC" SET _DOCS=ON
if "%1"=="setup" SET _SETUP=ON
if "%1"=="SETUP" SET _SETUP=ON
shift
if NOT "%1"=="" goto :getparam

rem OpenSSL
echo ================================================================================
echo building OpenSSL
cd ..\common\openssl
perl Configure VC-WIN32 enable-rc5 enable-mdc2 > NUL
call ms\do_masm
call nmake -f ms\ntdll.mak
@echo off

rem Subversion
echo ================================================================================
echo building Subversion
cd %startdir%\ext\Subversion
xcopy /Q /Y /I /E %startdir%\ext\berkeley-db\db4.3-win32 db4-win32
rmdir /s /q build\win32\vcnet-vcproj
del build\win32\build_*.bat
echo 0.25.4> %startdir%\ext\neon\.version
call python gen-make.py -t vcproj --with-openssl=..\..\..\Common\openssl --with-zlib=..\..\..\Common\zlib --with-neon=..\neon --with-apr=..\apr --with-apr-util=..\apr-util --with-apr-iconv=..\apr-iconv --enable-nls --enable-bdb-in-apr-util --vsnet-version=2003
copy /Y %startdir%\ext\libaprutil.vcproj %startdir%\ext\apr-util\libaprutil.vcproj
rem the expat.h.in doesn't have the version information correctly set :(
copy %startdir%\ext\apr-util\xml\expat\lib\expat.h.in %startdir%\ext\apr-util\xml\expat\lib\expat.h.in_copy
copy %startdir%\expat.h.in %startdir%\ext\apr-util\xml\expat\lib\expat.h.in /Y
rem the neon tarball contains config.hw, but the source tag doesn't
copy %startdir%\neonconfig.hw %startdir%\ext\neon\config.hw /Y
rem we want apr-util without apr-iconv!
copy %startdir%\ext\apu.hw %startdir%\ext\apr-util\include\apu.hw /Y
copy %startdir%\ext\utf.c %startdir%\ext\Subversion\subversion\libsvn_subr\utf.c /Y

cd %startdir%
if DEFINED _DEBUG (
  if EXIST bin\debug\bin rmdir /S /Q bin\debug\bin > NUL
  mkdir bin\debug\bin > NUL
  devenv %startdir%\ext\Subversion_dll\Subversion_dll.sln /useenv /rebuild debug /project "Subversion_dll"
)

if DEFINED _RELEASE (
  if EXIST bin\release\bin rmdir /S /Q bin\release\bin > NUL
  mkdir bin\release\bin > NUL
  devenv %startdir%\ext\Subversion_dll\Subversion_dll.sln /useenv /rebuild release /project "Subversion_dll"
)

copy %startdir%\ext\apr-util\xml\expat\lib\expat.h.in_copy %startdir%\ext\apr-util\xml\expat\lib\expat.h.in /Y
del %startdir%\ext\apr-util\xml\expat\lib\expat.h.in_copy

svn revert -R ext\apr-util
svn revert -R ext\Subversion

@echo off
rem TortoiseSVN
echo ================================================================================
echo copying files
cd %startdir%
if DEFINED _DEBUG (
  copy ..\Common\openssl\out32dll\*.dll bin\debug\bin /Y > NUL
  copy .\ext\svn-win32-libintl\bin\intl3_svn.dll bin\debug\bin /Y > NUL
  copy .\ext\Subversion\db4-win32\bin\libdb43d.dll bin\debug\bin /Y > NUL
  copy .\ext\apr\Debug\libapr.dll bin\Debug\bin /Y > NUL 
  copy .\ext\apr-util\Debug\libaprutil.dll bin\Debug\bin /Y > NUL 
)
if DEFINED _RELEASE (
  copy ..\Common\openssl\out32dll\*.dll bin\release\bin /Y > NUL
  copy .\ext\svn-win32-libintl\bin\intl3_svn.dll bin\release\bin /Y > NUL
  copy .\ext\Subversion\db4-win32\bin\libdb43.dll bin\release\bin /Y > NUL
  copy .\ext\apr\Release\libapr.dll bin\Release\bin /Y > NUL 
  copy .\ext\apr-util\Release\libaprutil.dll bin\Release\bin /Y > NUL 
)
echo ================================================================================
echo building TortoiseSVN
cd src
rem Build SubWCRev twice to include its own version info
copy /y version.none version.h
if NOT EXIST ..\bin\release\bin\SubWCRev.exe devenv TortoiseSVN.sln /rebuild release /project SubWCRev

:: now we can create all files with version numbers in them
cd ..
call version.bat
cd src
devenv TortoiseSVN.sln /rebuild release /project SubWCRev
if DEFINED _RELEASE (
  devenv TortoiseSVN.sln /rebuild release
  copy TortoiseSVNSetup\include\autolist.txt ..\bin\release\bin
)
if DEFINED _DEBUG (
  devenv TortoiseSVN.sln /rebuild debug
  copy TortoiseSVNSetup\include\autolist.txt ..\bin\debug\bin
)

echo ================================================================================
echo building Scintilla
cd Utils\scintilla\win32
nmake -f scintilla.mak
copy ..\bin\SciLexer.dll ..\..\..\..\bin\debug\bin /Y > NUL
copy ..\bin\SciLexer.dll ..\..\..\..\bin\release\bin /Y > NUL
del ..\bin\*.dll > NUL
del ..\bin\*.exp > NUL
del ..\bin\*.lib > NUL
del ..\bin\*.pdb > NUL
del *.obj > NUL
del *.res > NUL
del *.pdb > NUL

cd ..\..\..\..\Languages
call Make_Pot.bat
cd ..
@echo off

echo ================================================================================
cd doc
if DEFINED _DOCS (
  echo generating docs
  call GenDoc.bat > NUL
)
@echo off

echo ================================================================================
cd ..\src\TortoiseSVNSetup
@echo off
if DEFINED _SETUP (
  echo building installers

  call MakeMsi.bat
  cd ..\..\Languages
  call Make_Installer.bat
  cd ..\src\TortoiseSVNSetup
)

:end
cd %startdir%
echo started at: %starttime%
echo now it is : %time%