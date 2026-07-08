; GameReqAnalyzer NSIS Installer
; -----------------------------------------------
; Installer for GameReqAnalyzer on Windows

!define PRODUCT_NAME "GameReqAnalyzer"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "GameReqAnalyzer Team"
!define PRODUCT_WEB_SITE "https://github.com/yourusername/GameReqAnalyzer"
!define PRODUCT_DIR "GameReqAnalyzer"
!define HELP_URL "https://github.com/yourusername/GameReqAnalyzer/wiki"
!define UPDATE_URL "https://github.com/yourusername/GameFetchAnalyzer/releases"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Installer properties
Name "${PRODUCT_NAME}"
Caption "${PRODUCT_NAME} ${PRODUCT_VERSION} Setup"
OutFile "GameRegexAnalyzer-Setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_DIR}"
InstallDirRegKey HKLM "Software\${PRODUCT_DIR}" ""
ShowInstDetails show
ShowUninstDetails show
BrandingText "/$(PRODUCT_NAME)"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.md"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MPI_PAGE_FINISH

; Languages
!insertmacro MUI_LANGUAGE "English"

; Sections
Section "MainApplication" SEC01
  SetOutPath "$INSTDIR"
  File /r "build\Release\game-req-analyzer.exe"
  File /r "docs\*"
  File /r "data\*"
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  ; Create start menu shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_DIR}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_DIR}\${PRODUCT_NAME}.lnk" "$INSTDIR\game-req-analyzer.exe" "" "$INSTDIR\game-req-analyzer.exe" 0
  CreateShortCut "$SMPROGRAMS\${PRODUCT_DIR}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\game-req-analyzer.exe" "" "$INSTDIR\game-req-analyzer.exe" 0
  ; Write registry keys for uninstall
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME} ${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayIcon" "$INSTDIR\game-req-analyzer.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "InfoTip" "Application to analyze game system requirements."
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "HelpLink" "${HELP_URL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "URLInfoAbout" "${UPDATE_URL}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoRepair" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Version" ${PRODUCT_VERSION}
  WriteRegSTR HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
SectionEnd

Section "Uninstall"
  ; Remove files
  Delete "$INSTDIR\game-req-analyzer.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"
  ; Remove start menu shortcuts
  Delete "$SMPROGRAMS\${PRODUCT_DIR}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_DIR}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_DIR}"
  ; Remove desktop shortcut
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  ; Remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
SectionEnd
