#define AppName "QuantumVerse"
#define AppVersion "7.2"
#define AppExe "quantumverse_qml.exe"
#define StageDir "F:\syyyy\installer\stage"

[Setup]
AppId="{{B7E5F3A2-4C1D-4E8F-9A2B-3D4E5F6A7B8C}}"
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=F:\syyyy\installer
OutputBaseFilename=QuantumVerse-{#AppVersion}-Setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExe}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; \
    Excludes: "*.h5;*.hdf5;*.pth;*.ckpt;*.onnx;*.bin"

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon; WorkingDir: "{app}"

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent; WorkingDir: "{app}"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
  if not FileExists('C:\Program Files\MiKTeX\miktex\bin\x64\pdflatex.exe') then
  begin
    if MsgBox('MiKTeX (pdflatex) is required for PDF generation but was not found.' + #13#10 +
              'Without it, paper generation will fail at the compile step.' + #13#10#13#10 +
              'Download from: https://miktex.org/download' + #13#10 +
              'Continue installation anyway?', mbConfirmation, MB_YESNO) = IDNO then
      Result := False;
  end;
end;
