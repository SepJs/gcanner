; gcanner Windows Installer Script (NSIS)
; Save as installer/windows/gcanner_installer.nsi

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

!define APP_NAME "gcanner"
!define APP_VERSION "1.0.0"
!define APP_PUBLISHER "gcanner contributors"
!define APP_WEBSITE "https://github.com/SepJs/gcanner"
!define APP_EXE "gcanner_gui.exe"
!define INSTALLER_NAME "gcanner-${APP_VERSION}-windows-x64.exe"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "${INSTALLER_NAME}"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_NAME}" "Install_Dir"
RequestExecutionLevel admin
Unicode true

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "installer/windows/welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "installer/windows/unwelcome.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_LINK "Visit website"
!define MUI_FINISHPAGE_LINK_LOCATION "${APP_WEBSITE}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SEC_MAIN
    SetOutPath "$INSTDIR"

    ; Main executable
    File "bin\${APP_EXE}"

    ; DLLs and libraries
    File /r "bin\*.dll"
    File /r "bin\qt*.dll"

    ; Data files
    File /r "share\*"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Registry entries
    WriteRegStr HKLM "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "URLInfoAbout" "${APP_WEBSITE}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1

    ; Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Website.lk" "${APP_WEBSITE}" "" "" 0

    ; Desktop shortcut
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0

    ; File association
    WriteRegStr HKCR ".gcanner" "" "gcanner.report"
    WriteRegStr HKCR "gcanner.report" "" "gcanner Report"
    WriteRegStr HKCR "gcanner.report\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'
SectionEnd

Section "HardwareDatabase" SEC_HWDB
    SectionIn RO
    DetailPrint "Installing hardware database..."
    File /r "data\hardware_database.json"
SectionEnd

Section -Post
    WriteUninstaller "$INSTDIR\uninstall.exe"
    ; Register file association for .json reports
    WriteRegStr HKCR ".gcanner_report" "" "gcanner.report"
    WriteRegStr HKCR "gcanner.report" "" "gcanner Scan Report"
    WriteRegStr HKCR "gcanner.report\DefaultIcon" "" "$INSTDIR\${APP_EXE},0"
    WriteRegStr HKCR "gcanner.report\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\uninstall.exe"
    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\share"
    RMDir /r "$INSTDIR\data"

    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Website.lnk"
    RMDir "$SMPROGRAMS\${APP_NAME}"

    Delete "$DESKTOP\${APP_NAME}.lnk"

    DeleteRegKey HKLM "Software\${APP_NAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
    DeleteRegKey HKCR ".gcanner"
    DeleteRegKey HKCR "gcanner.report"
    DeleteRegKey HKCR ".gcanner_report"
    DeleteRegKey HKCR "gcanner.report"
SectionEnd

Function .onInit
    ; Check if already installed
    ReadRegStr $0 HKLM "Software\${APP_NAME}" "Install_Dir"
    StrCmp $0 "" 0 already_installed
    Goto done

    already_installed:
    MessageBox MB_YESNO|MB_ICONQUESTION "${APP_NAME} is already installed. Do you want to reinstall?" IDYES reinstall IDNO cancel
    reinstall:
    StrCpy $INSTDIR $0
    Goto done
    cancel:
    Abort

    done:
FunctionEnd

Function un.onInit
    MessageBox MB_YESNO|MB_ICONQUESTION "Are you sure you want to uninstall ${APP_NAME}?" IDYES continue IDNO cancel
    continue:
    FunctionReturn
    cancel:
    Abort
FunctionEnd

Function un.onUninstallSuccess
    HideWindow
    MessageBox MB_OK|MB_ICONINFORMATION "${APP_NAME} has been successfully uninstalled."
FunctionEnd