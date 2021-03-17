# Configuration file for building Dynare Windows Installer
# Uses "NullSoft Scriptable Installer System", aka NSIS (see http://nsis.sourceforge.net)
# NSIS can be run from both Windows and Linux (see "nsis" package in Debian)

SetCompressor /SOLID lzma

Name "Dynare ${VERSION}"

OutFile "dynare-${VERSION}-win.exe"

InstallDir "c:\dynare\${VERSION}"

# Use the Modern User Interface (version 2)
!include MUI2.nsh

!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of Dynare ${VERSION}.$\n$\nDynare is distributed under the GNU General Public License (GPL) version 3.$\n$\nIf you accept the license, click Next button to continue the installation."
!insertmacro MUI_PAGE_WELCOME
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_COMPONENTSPAGE_TEXT_TOP "Choose the components you want to install.$\nIf you know whether your version of MATLAB or Octave is 64-bit or 32-bit, you can uncheck the component that you don’t need in order to save disk space."
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_LINK_LOCATION https://www.dynare.org
!define MUI_FINISHPAGE_LINK "Go to Dynare homepage"
!define MUI_FINISHPAGE_SHOWREADME $INSTDIR\README.txt
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

!define REGLOC "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dynare ${VERSION}"
!define SMLOC "$SMPROGRAMS\Dynare ${VERSION}"

!macro DETERMINE_CONTEXT
 # Determine if we are admin or not
 # This will change the start menu directory and the registry root key (HKLM or HKLU)
 UserInfo::getAccountType
 Pop $0
 StrCmp $0 "Admin" +3
 SetShellVarContext current
 Goto +2
 SetShellVarContext all
!macroend


Section "Dynare core (preprocessor and M-files)"
 SectionIn RO
!insertmacro DETERMINE_CONTEXT
 SetOutPath $INSTDIR
 File README.txt ..\NEWS.md ..\license.txt ..\VERSION

 SetOutPath $INSTDIR\matlab
 File /r ..\matlab\*.m

 SetOutPath $INSTDIR\matlab\preprocessor32
 File ..\matlab\preprocessor32\dynare_m.exe

 SetOutPath $INSTDIR\matlab\preprocessor64
 File ..\matlab\preprocessor64\dynare_m.exe

 SetOutPath $INSTDIR\matlab\modules\dseries\externals\x13\windows\32
 File deps\lib32\x13as\x13as.exe

 SetOutPath $INSTDIR\matlab\modules\dseries\externals\x13\windows\64
 File deps\lib64\x13as\x13as.exe

 SetOutPath $INSTDIR\contrib
 File /r ..\contrib\*.m

 SetOutPath $INSTDIR\scripts
 File ..\scripts\dynare.el

 WriteUninstaller $INSTDIR\uninstall.exe

 # Create start menu entries
 CreateDirectory "${SMLOC}"
 CreateShortcut "${SMLOC}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

 # Create entry in "Add/Remove programs"
 WriteRegStr SHELL_CONTEXT "${REGLOC}" "DisplayName" "Dynare ${VERSION}"
 WriteRegStr SHELL_CONTEXT "${REGLOC}" "DisplayVersion" "${VERSION}"
 WriteRegStr SHELL_CONTEXT "${REGLOC}" "InstallLocation" $INSTDIR
 WriteRegStr SHELL_CONTEXT "${REGLOC}" "UninstallString" "$INSTDIR\uninstall.exe"
 WriteRegDWORD SHELL_CONTEXT "${REGLOC}" "NoModify" 1
 WriteRegDWORD SHELL_CONTEXT "${REGLOC}" "NoRepair" 1
SectionEnd


SectionGroup "Dynare support for 64-bit MATLAB and Octave"

Section "MEX files for MATLAB 64-bit, version 7.9 to 9.3 (R2009b to R2017b)"
 SetOutPath $INSTDIR\mex\matlab\win64-7.9-9.3
 File ..\mex\matlab\win64-7.9-9.3\*.mexw64
SectionEnd

Section "MEX files for MATLAB 64-bit, version 9.4 to 9.10 (R2018a to R2021a)"
 SetOutPath $INSTDIR\mex\matlab\win64-9.4-9.10
 File ..\mex\matlab\win64-9.4-9.10\*.mexw64
SectionEnd

Section "MEX files for Octave 6.2.0 (64-bit)"
 SetOutPath $INSTDIR\mex\octave\win64
 File ..\mex\octave\win64\*
SectionEnd

Section "MinGW compiler for MATLAB 64-bit"
 SetOutPath $INSTDIR\mingw64
 File /r deps\mingw64\*
SectionEnd

SectionGroupEnd


SectionGroup "Dynare support for 32-bit MATLAB and Octave"

Section "MEX files for MATLAB 32-bit, version 7.9 to 8.6 (R2009b to R2015b)"
 SetOutPath $INSTDIR\mex\matlab\win32-7.9-8.6
 File ..\mex\matlab\win32-7.9-8.6\*.mexw32
SectionEnd

Section "MEX files for Octave 6.2.0 (32-bit)"
 SetOutPath $INSTDIR\mex\octave\win32
 File ..\mex\octave\win32\*
SectionEnd

Section "MinGW compiler for MATLAB 32-bit"
 SetOutPath $INSTDIR\mingw32
 File /r deps\mingw32\*
SectionEnd

SectionGroupEnd


Section "Documentation and examples"
 SetOutPath $INSTDIR\doc
 File ..\doc\manual\build\latex\dynare-manual.pdf ..\doc\guide.pdf ..\doc\bvar-a-la-sims.pdf ..\doc\dr.pdf ..\preprocessor\doc\macroprocessor\macroprocessor.pdf ..\preprocessor\doc\preprocessor\preprocessor.pdf ..\doc\parallel\parallel.pdf ..\doc\gsa\gsa.pdf ..\doc\dseries-and-reporting\dseriesReporting.pdf

 SetOutPath $INSTDIR\doc\dynare-manual.html
 File /r ..\doc\manual\build\html\*

 CreateShortcut "${SMLOC}\Documentation.lnk" "$INSTDIR\doc"

 SetOutPath $INSTDIR\examples
 File ..\examples\*.mod ..\examples\*.m

 CreateShortcut "${SMLOC}\Examples.lnk" "$INSTDIR\examples"
SectionEnd


Section /o "Dynare++ (standalone executable)"

 SetOutPath $INSTDIR\dynare++\32-bit
 File ..\dynare++\32-bit\dynare++.exe

 SetOutPath $INSTDIR\dynare++\64-bit
 File ..\dynare++\64-bit\dynare++.exe

 SetOutPath $INSTDIR\dynare++
 File ..\dynare++\dynare_simul\dynare_simul.m

 SetOutPath $INSTDIR\doc\dynare++
 File ..\dynare++\doc\*.pdf

 SetOutPath $INSTDIR\examples\dynare++
 File ..\examples\dynare++\example1.mod ..\examples\dynare++\README.txt
SectionEnd


Section "Uninstall"
!insertmacro DETERMINE_CONTEXT

 # First delete the uninstaller
 Delete $INSTDIR\uninstall.exe
 Delete $INSTDIR\README.txt
 Delete $INSTDIR\NEWS.md
 Delete $INSTDIR\license.txt
 Delete $INSTDIR\VERSION
 Rmdir /r $INSTDIR\matlab
 Rmdir /r $INSTDIR\contrib
 Rmdir /r $INSTDIR\mex
 Rmdir /r $INSTDIR\dynare++
 Rmdir /r $INSTDIR\doc
 Rmdir /r $INSTDIR\examples
 Rmdir /r $INSTDIR\scripts
 Rmdir /r $INSTDIR\mingw32
 Rmdir /r $INSTDIR\mingw64
 # We don't force deletion of installation directory (with /r), to avoid deleting important files
 Rmdir $INSTDIR

 # Delete start menu entries
 Rmdir /r "${SMLOC}"

 # Delete entry in "Add/Remove programs"
 DeleteRegKey SHELL_CONTEXT "${REGLOC}"
SectionEnd
