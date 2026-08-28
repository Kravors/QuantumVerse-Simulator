; QuantumVerse 5.5.1 NSIS Installer script
; Build with: makensis QuantumVerse.nsi
; Requires the deploy step to have populated deploy\windows first.

!define APPNAME "QuantumVerse"
!define APPVERSION "5.5.1"
!define PUBLISHER "Kravors"
!define INSTALLDIR "$PROGRAMFILES64\QuantumVerse"
!define CONTACT "dhiae1122@gmail.com"

Name "${APPNAME} ${APPVERSION}"
OutFile "QuantumVerse-5.5.1-win64.exe"
InstallDir "${INSTALLDIR}"
RequestExecutionLevel admin

!include "MUI2.nsh"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "C:\Users\raffa\Desktop\QuantumVerse-5.5.1-win64\*"

  ; Start Menu + Desktop shortcuts
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\quantumverse_qml.exe"
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\quantumverse_qml.exe"

  ; Uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME} ${APPVERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${APPVERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Contact" "${CONTACT}"
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
