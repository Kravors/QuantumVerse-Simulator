; QuantumVerse 3.6.0 NSIS Installer script
; Build with: makensis QuantumVerse.nsi
; Requires the deploy step to have populated deploy\windows first.

!define APPNAME "QuantumVerse"
!define APPVERSION "3.6.0"
!define PUBLISHER "Humanity"
!define INSTALLDIR "$PROGRAMFILES64\QuantumVerse"

Name "${APPNAME} ${APPVERSION}"
OutFile "QuantumVerse_Setup.exe"
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
  File /r "deploy\windows\*"

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
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
