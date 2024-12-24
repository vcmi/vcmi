; VCMI Installer Script

; How to Add a New Translation to the Installer

; 1. Download your language's ISL file from the Inno Setup repository: 
;    https://github.com/jrsoftware/issrc/tree/main/Files/Languages
; 2. Add the necessary VCMI custom messages (refer to the end of English.isl for examples).
; 3. If the Uninstall Wizard is used, modify the ConfirmUninstall message to align with 
;    the English version. Ensure it is updated accordingly.
; 4. Add the corresponding entry to the [Languages] section.
 
; Manual preprocessor definitions are provided using ISCC.exe parameters instead.

; #define AppVersion "1.6.0"
; #define AppBuild "2272707"
; #define InstallerArch "x64"
; 
; #define SourceFilesPath "C:\_VCMI_Source_v2\_files"
; #define LangPath "C:\_VCMI_Source_v2\CI\wininstaller\lang"
; #define LicenseFile "C:\_VCMI_Source_v2\license.txt"
; #define IconFile "C:\_VCMI_Source_v2\clientapp\icons\vcmi.ico"
; #define SmallLogo "C:\_VCMI_Source_v2\CI\wininstaller\vcmismalllogo.bmp"
; #define WizardLogo "C:\_VCMI_Source_v2\CI\wininstaller\vcmilogo.bmp"

#define VCMIFilesFolder "My Games\vcmi"
#define InstallerName "VCMI_Installer"

#define AppComment "VCMI is an open-source engine for Heroes III, offering new and extended possibilities."
#define VCMITeam "VCMI Team"
#define VCMICopyright "Copyright © VCMI Community. All rights reserved."

#define VCMIHome "https://vcmi.eu/"
#define VCMIContact "https://discord.gg/chBT42V"

[Setup]
AppId=VCMI
AppName=VCMI
AppVersion={#AppVersion}.{#AppBuild}
AppVerName=VCMI
AppPublisher={#VCMITeam}
AppPublisherURL={#VCMIHome}
AppSupportURL={#VCMIContact}
AppComments={#AppComment}
DefaultDirName={code:GetDefaultDir}
DefaultGroupName=VCMI
UninstallDisplayIcon={app}\VCMI_launcher.exe
OutputBaseFilename={#InstallerName}_{#InstallerArch}_{#AppVersion}.{#AppBuild}
PrivilegesRequiredOverridesAllowed=commandline dialog
ShowLanguageDialog=yes
DisableWelcomePage=no
DisableProgramGroupPage=yes
ChangesAssociations=yes
UsePreviousLanguage=yes
DirExistsWarning=no
UsePreviousAppDir=yes
UsePreviousTasks=yes
UsePreviousGroup=yes
DisableStartupPrompt=yes
UsedUserAreasWarning=no
WindowResizable=no
CloseApplicationsFilter=*.exe
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed={#InstallerArch}compatible
LicenseFile={#LicenseFile}
SetupIconFile={#IconFile}
WizardSmallImageFile={#SmallLogo}
WizardImageFile={#WizardLogo}

; Version informations
MinVersion=6.1sp1
VersionInfoCompany={#VCMITeam}
VersionInfoDescription=VCMI {#AppVersion} Setup (Build {#AppBuild})
VersionInfoProductName=VCMI
VersionInfoCopyright={#VCMICopyright}
VersionInfoVersion={#AppVersion} 
VersionInfoOriginalFileName={#InstallerName}.exe

[Languages]
Name: "english"; MessagesFile: "{#LangPath}\English.isl"
Name: "czech"; MessagesFile: "{#LangPath}\Czech.isl"
Name: "chinese"; MessagesFile: "{#LangPath}\ChineseSimplified.isl"
Name: "finnish"; MessagesFile: "{#LangPath}\Finnish.isl"
Name: "french"; MessagesFile: "{#LangPath}\French.isl"
Name: "german"; MessagesFile: "{#LangPath}\German.isl"
Name: "hungarian"; MessagesFile: "{#LangPath}\Hungarian.isl"
Name: "italian"; MessagesFile: "{#LangPath}\Italian.isl"
Name: "korean"; MessagesFile: "{#LangPath}\Korean.isl"
Name: "polish"; MessagesFile: "{#LangPath}\Polish.isl"
Name: "portuguese"; MessagesFile: "{#LangPath}\BrazilianPortuguese.isl"
Name: "russian"; MessagesFile: "{#LangPath}\Russian.isl"
Name: "spanish"; MessagesFile: "{#LangPath}\Spanish.isl"
Name: "swedish"; MessagesFile: "{#LangPath}\Swedish.isl"
Name: "turkish"; MessagesFile: "{#LangPath}\Turkish.isl"
Name: "ukrainian"; MessagesFile: "{#LangPath}\Ukrainian.isl"
Name: "vietnamese"; MessagesFile: "{#LangPath}\Vietnamese.isl"

[Files]
Source: "{#SourceFilesPath}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; BeforeInstall: PerformHeroes3FileCopy

[Icons]
Name: "{group}\{cm:ShortcutLauncher}"; Filename: "{app}\VCMI_launcher.exe"; Comment: "{cm:ShortcutLauncherComment}"
Name: "{group}\{cm:ShortcutMapEditor}"; Filename: "{app}\VCMI_mapeditor.exe"; Comment: "{cm:ShortcutMapEditorComment}"
Name: "{group}\{cm:ShortcutWebPage}"; Filename: "{#VCMIHome}"; Comment: "{cm:ShortcutWebPageComment}"
Name: "{group}\{cm:ShortcutDiscord}"; Filename: "{#VCMIContact}"; Comment: "{cm:ShortcutDiscordComment}"

Name: "{code:GetUserDesktopFolder}\{cm:ShortcutLauncher}"; Filename: "{app}\VCMI_launcher.exe"; Tasks: desktop; Comment: "{cm:ShortcutLauncherComment}"

[Tasks]
Name: "fileassociation_h3m"; Description: "{cm:AssociateH3MFiles}"; GroupDescription: "{cm:FileAssociations}"; Flags: unchecked
Name: "fileassociation_vmap"; Description: "{cm:AssociateVMapFiles}"; GroupDescription: "{cm:FileAssociations}"

Name: "desktop"; Description: "{cm:CreateDesktopShortcuts}"; GroupDescription: "{cm:ShortcutsOptions}"
Name: "startmenu"; Description: "{cm:CreateStartMenuShortcuts}"; GroupDescription: "{cm:ShortcutsOptions}"
Name: "firewallrules"; Description: "{cm:AddFirewallRules}"; GroupDescription: "{cm:FirewallOptions}"; Check: IsAdminInstallMode


[Registry]
Root: HKCU; Subkey: "Software\VCMI"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

Root: HKCU; Subkey: "Software\Classes\.vmap"; ValueType: string; ValueName: ""; ValueData: "VCMI.vmap"; Flags: uninsdeletevalue; Tasks: fileassociation_vmap
Root: HKCU; Subkey: "Software\Classes\VCMI.vmap"; ValueType: string; ValueName: ""; ValueData: "{cm:VMAPDescription}"; Flags: uninsdeletekey; Tasks: fileassociation_vmap
Root: HKCU; Subkey: "Software\Classes\VCMI.vmap\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\VCMI_mapeditor.exe"" ""%1"""; Tasks: fileassociation_vmap

Root: HKCU; Subkey: "Software\Classes\.h3m"; ValueType: string; ValueName: ""; ValueData: "VCMI.h3m"; Flags: uninsdeletevalue; Tasks: fileassociation_h3m
Root: HKCU; Subkey: "Software\Classes\VCMI.h3m"; ValueType: string; ValueName: ""; ValueData: "{cm:H3MDescription}"; Flags: uninsdeletekey; Tasks: fileassociation_h3m
Root: HKCU; Subkey: "Software\Classes\VCMI.h3m\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\VCMI_mapeditor.exe"" ""%1"""; Tasks: fileassociation_h3m


[Run]
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=vcmi_server dir=in action=allow program=""{app}\vcmi_server.exe"" enable=yes profile=public,private"; Flags: runhidden; Tasks: firewallrules; Check: IsAdmin
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=vcmi_client dir=in action=allow program=""{app}\vcmi_client.exe"" enable=yes profile=public,private"; Flags: runhidden; Tasks: firewallrules; Check: IsAdmin

Filename: "{app}\VCMI_launcher.exe"; Description: "{cm:RunVCMILauncherAfterInstall}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Kill VCMI processes
Filename: "taskkill.exe"; Parameters: "/F /IM VCMI_client.exe"; Flags: runhidden; RunOnceId: "KillVCMIClient"
Filename: "taskkill.exe"; Parameters: "/F /IM VCMI_server.exe"; Flags: runhidden; RunOnceId: "KillVCMIServer"
Filename: "taskkill.exe"; Parameters: "/F /IM VCMI_launcher.exe"; Flags: runhidden; RunOnceId: "KillVCMILauncher"
Filename: "taskkill.exe"; Parameters: "/F /IM VCMI_mapeditor.exe"; Flags: runhidden; RunOnceId: "KillVCMIMapEditor"

; Remove firewall rules
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=vcmi_server"; Flags: runhidden; Check: IsAdmin; RunOnceId: "RemoveFirewallVCMIServer"
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=vcmi_client"; Flags: runhidden; Check: IsAdmin; RunOnceId: "RemoveFirewallVCMIClient"


[Code]
var
  InstallModePage: TInputOptionWizardPage;
  FooterLabel: TLabel;
  IsUpgrade: Boolean;
  Heroes3Path: String;
  GlobalUserName: String;
  GlobalUserDocsFolder: String;
  GlobalUserAppdataFolder: String;

  
function RegistryQueryPath(Key, ValueName: String): String;
begin
  if RegQueryStringValue(HKLM, Key, ValueName, Result) then
    Exit
  else
    Result := '';
end;


function FolderSize(FolderPath: String): Int64;
var
  FindRec: TFindRec;
begin
  Result := 0;
  if FindFirst(FolderPath + '\*', FindRec) then
  begin
    try
      repeat
        if (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
          Result := Result + FindRec.SizeLow
        else if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
          Result := Result + FolderSize(FolderPath + '\' + FindRec.Name);
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
end;


function IsFolderValid(FolderPath: String): Boolean;
begin
  Result := DirExists(FolderPath) and (FolderSize(FolderPath) > 1024 * 1024);
end;


procedure CopyFolderContents(SourceDir, DestDir: String; Overwrite: Boolean);
var
  FindRec: TFindRec;
  SourceFile, DestFile: String;
begin
  // Ensure the destination directory exists
  if not DirExists(DestDir) then
    if not ForceDirectories(DestDir) then
    begin
      //MsgBox('Failed to create destination directory: ' + DestDir, mbError, MB_OK);
      Exit;
    end;

  // Start file copying
  if FindFirst(SourceDir + '\*.*', FindRec) then
  begin
    try
      repeat
        SourceFile := SourceDir + '\' + FindRec.Name;
        DestFile := DestDir + '\' + FindRec.Name;

        if (FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
        begin
          if Overwrite or not FileExists(DestFile) then
          begin
            if not FileCopy(SourceFile, DestFile, False) then
              //MsgBox('Failed to copy file: ' + SourceFile + ' to ' + DestFile, mbError, MB_OK);
          end;
        end
        else if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
        begin
          // Copy subdirectories recursively
          CopyFolderContents(SourceFile, DestFile, Overwrite);
        end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end
  //else
  //  MsgBox('No files found in directory: ' + SourceDir, mbError, MB_OK);
end;


// A huge workaround to get non-admin profile name on elevated installer as admin
function WTSQuerySessionInformation(hServer: THandle; SessionId: Cardinal; WTSInfoClass: Integer; var pBuffer: DWord; var BytesReturned: DWord): Boolean;
  external 'WTSQuerySessionInformationW@wtsapi32.dll stdcall';

procedure WTSFreeMemory(pMemory: DWord);
  external 'WTSFreeMemory@wtsapi32.dll stdcall';

procedure RtlMoveMemoryAsString(Dest: string; Source: DWord; Len: Integer);
  external 'RtlMoveMemory@kernel32.dll stdcall';

const
  WTS_CURRENT_SERVER_HANDLE = 0;
  WTS_CURRENT_SESSION = -1;
  WTSUserName = 5;

function GetCurrentSessionUserName: string;
var
  Buffer: DWord;
  BytesReturned: DWord;
  QueryResult: Boolean;
begin
  // Initialize Result to an empty string
  Result := '';

  // Query the username for the current session
  QueryResult := WTSQuerySessionInformation(
    WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSUserName, Buffer, BytesReturned);

  if not QueryResult then
  begin
    // Error if the query fails
    Exit;
  end;

  try
    // Set the length of the result string (BytesReturned includes null terminator)
    SetLength(Result, (BytesReturned div 2) - 1); // Divide by 2 for Unicode and exclude null terminator

    // Copy the buffer contents into the result string
    RtlMoveMemoryAsString(Result, Buffer, BytesReturned - 2); // Exclude null terminator
  finally
    // Free the allocated memory
    WTSFreeMemory(Buffer);
  end;

end;


function GetCommonProgramFilesDir: String;
begin
  // Check if the installer is running on a 64-bit system
  if DirExists(ExpandConstant('{win}\syswow64')) then
  begin
    if ExpandConstant('{#InstallerArch}') = 'x64' then
      // For 64-bit installer, return the 64-bit Program Files directory
      Result := ExpandConstant('{commonpf64}')
    else
      // For 32-bit installer on 64-bit system, return the 32-bit Program Files directory
      Result := ExpandConstant('{commonpf32}');
  end
  else
    // On 32-bit systems, always return the 32-bit Program Files directory
    Result := ExpandConstant('{commonpf32}');
end;


function GetDefaultDir(Default: String): String;
begin
  if IsAdmin then
    // Default to Program Files for admins
    Result := GetCommonProgramFilesDir + '\VCMI'
  else
    // Default to User AppData for non-admin users
    Result := GlobalUserAppdataFolder + '\VCMI';
end;


function GetUserDocsFolder: String;
var
  UserDocsPath: String;
  OriginalUserName: String;
  CurrentSessionUserName: String;
begin
  // Retrieve the current username from the session
  CurrentSessionUserName := '\' + GlobalUserName + '\';

  // Retrieve the original username
  OriginalUserName := '\' + GetUserNameString + '\';

  // Expand the {userdocs} constant
  UserDocsPath := ExpandConstant('{userdocs}');

  // Replace the original username with the current session username in the user docs path
  StringChangeEx(UserDocsPath, OriginalUserName, CurrentSessionUserName, True);
  
  // Return the modified user docs folder path
  Result := UserDocsPath;
end;


function GetUserAppdataFolder: String;
var
  UserAppdataPath: String;
  OriginalUserName: String;
  CurrentSessionUserName: String;
begin
  // Retrieve the current username from the session
  CurrentSessionUserName := '\' + GlobalUserName + '\';

  // Retrieve the original username
  OriginalUserName := '\' + GetUserNameString + '\';

  // Expand the {userappdata} constant
  UserAppdataPath := ExpandConstant('{userappdata}');

  // Replace the original username with the current session username in the user Appdata path
  StringChangeEx(UserAppdataPath, OriginalUserName, CurrentSessionUserName, True);
  
  // Return the modified user docs folder path
  Result := UserAppdataPath;
end;


function GetUserDesktopFolder(Default: String): String;
var
  UserDesktopPath: String;
  OriginalUserName: String;
  CurrentSessionUserName: String;
begin
  // Retrieve the current username from the session
  CurrentSessionUserName := '\' + GlobalUserName + '\';

  // Retrieve the original username
  OriginalUserName := '\' + GetUserNameString + '\';

  // Expand the {userdesktop} constant
  UserDesktopPath := ExpandConstant('{userdesktop}');

  // Replace the original username with the current session username
  StringChangeEx(UserDesktopPath, OriginalUserName, CurrentSessionUserName, True);

  // Return the modified user desktop folder path
  Result := UserDesktopPath;
end;


function InitializeSetup(): Boolean;
begin
  // Initialize the global variable during setup
  GlobalUserName := GetCurrentSessionUserName();
  GlobalUserDocsFolder := GetUserDocsFolder();
  GlobalUserAppdataFolder := GetUserAppdataFolder();
  
  Result := True;
end;


function InitializeUninstall(): Boolean;
begin
  // Initialize the global variable during uninstall
  GlobalUserName := GetCurrentSessionUserName();
  GlobalUserDocsFolder := GetUserDocsFolder();
  GlobalUserAppdataFolder := GetUserAppdataFolder();
  
  Result := True;
end;


procedure InitializeWizard();
var
  InstallPath, VCMIFolder, MapsFolder, DataFolder, Mp3Folder: String;
begin
  // Check if the application is already installed
  IsUpgrade := RegQueryStringValue(HKCU, 'Software\VCMI', 'InstallPath', InstallPath);
  if not IsUpgrade then
  begin
    // Create the install mode selection page only if it's not an upgrade
    InstallModePage := CreateInputOptionPage(
      wpWelcome,
      ExpandConstant('{cm:SelectSetupInstallModeTitle}'),
      ExpandConstant('{cm:SelectSetupInstallModeDesc}'),
      ExpandConstant('{cm:SelectSetupInstallModeSubTitle}'),
      True, False
    );
    
    // Option 0
    InstallModePage.Add(ExpandConstant(#13#10 + '  {cm:InstallForAllUsers}' + #13#10 + '   • {cm:InstallForAllUsers1}' + #13#10 + #13#10));
    // Option 1
    InstallModePage.Add(ExpandConstant(#13#10 + '  {cm:InstallForMeOnly}' + #13#10  +  '   • {cm:InstallForMeOnly1}' + #13#10 + '   • {cm:InstallForMeOnly2}' + #13#10));

    if IsAdmin then
    begin
      // Default to "All Users"
      InstallModePage.SelectedValueIndex := 0;
    end
    else
    begin
      // Default to "Me Only"
      InstallModePage.SelectedValueIndex := 1;

      // Disable the first option ("Install for All Users") for non-admins
      InstallModePage.CheckListBox.ItemEnabled[0] := False;

      // Force a redraw of the CheckListBox to fix appearance
      InstallModePage.CheckListBox.Invalidate();
    end;
  end;
 
  // Check for Heroes 3 installation paths
  Heroes3Path := '';
  Heroes3Path := RegistryQueryPath('SOFTWARE\GOG.com\Games\1207658787', 'path');
  if Heroes3Path = '' then
    Heroes3Path := RegistryQueryPath('SOFTWARE\WOW6432Node\GOG.com\Games\1207658787', 'path');
  if Heroes3Path = '' then
    Heroes3Path := RegistryQueryPath('SOFTWARE\New World Computing\Heroes of Might and Magic® III\1.0', 'AppPath');
  if Heroes3Path = '' then
    Heroes3Path := RegistryQueryPath('SOFTWARE\WOW6432Node\New World Computing\Heroes of Might and Magic® III\1.0', 'AppPath');
  if Heroes3Path = '' then
    Heroes3Path := RegistryQueryPath('SOFTWARE\New World Computing\Heroes of Might and Magic III\1.0', 'AppPath');
  if Heroes3Path = '' then
    Heroes3Path := RegistryQueryPath('SOFTWARE\WOW6432Node\New World Computing\Heroes of Might and Magic III\1.0', 'AppPath');
    
  // Paths in VCMI directory
  VCMIFolder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}';
  MapsFolder := VCMIFolder + '\Maps';
  DataFolder := VCMIFolder + '\Data';
  Mp3Folder := VCMIFolder + '\Mp3';

  // Create a custom label for the footer message
  FooterLabel := TLabel.Create(WizardForm);
  FooterLabel.Parent := WizardForm;
  FooterLabel.Caption := 'VCMI v' + '{#AppVersion}' + '.' + '{#AppBuild}';
  // Padding from the left edge
  FooterLabel.Left := 10;
  // Adjust to leave space for multiple lines
  FooterLabel.Top := WizardForm.ClientHeight - 30;
  // Adjust for padding
  FooterLabel.Width := WizardForm.ClientWidth - 20;
  // Adjust height to accommodate multiple lines
  FooterLabel.Height := 40; 
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False; // Default is not to skip the page

  if IsUpgrade then
  begin
    if (PageID = wpLicense) or (PageID = wpSelectTasks) or (PageID = wpReady) then
    begin
      Result := True; // Skip these pages during upgrade
      Exit;
    end;
  end;
end;


procedure CurPageChanged(CurPageID: Integer);
begin
  // Ensure the footer message is visible on every page
  FooterLabel.Visible := True;
    
end;


function NextButtonClick(CurPageID: Integer): Boolean;
begin
  // Skip the custom page on upgrade
  if IsUpgrade and Assigned(InstallModePage) and (CurPageID = InstallModePage.ID) then
  begin
    Result := True;
    Exit;
  end;

  // Handle logic for the custom page if it exists
  if Assigned(InstallModePage) and (CurPageID = InstallModePage.ID) then
  begin
    if (InstallModePage.SelectedValueIndex = 0) and not IsAdmin then
    begin
      Result := False;
      Exit;
    end;

    if InstallModePage.SelectedValueIndex = 0 then
      WizardForm.DirEdit.Text := GetCommonProgramFilesDir + '\VCMI'
    else
      WizardForm.DirEdit.Text := GlobalUserAppdataFolder + '\VCMI';
  end;

  Result := True;
end;


procedure PerformHeroes3FileCopy();
var
  VCMIFolder, VCMIMapsFolder, VCMIDataFolder, VCMIMp3Folder: String;
  Heroes3MapsFolder, Heroes3DataFolder, Heroes3Mp3Folder: String;
begin
  // Define paths for VCMI and Heroes 3 directories
  VCMIFolder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}';
  VCMIMapsFolder := VCMIFolder + '\Maps';
  VCMIDataFolder := VCMIFolder + '\Data';
  VCMIMp3Folder := VCMIFolder + '\Mp3';

  Heroes3MapsFolder := Heroes3Path + '\Maps';
  Heroes3DataFolder := Heroes3Path + '\Data';
  Heroes3Mp3Folder := Heroes3Path + '\Mp3';

  // Copy folders if conditions are met
  if (DirExists(Heroes3MapsFolder) and ((not DirExists(VCMIMapsFolder)) or (FolderSize(VCMIMapsFolder) < 1024 * 1024))) then
    CopyFolderContents(Heroes3MapsFolder, VCMIMapsFolder, True);

  if (DirExists(Heroes3DataFolder) and ((not DirExists(VCMIDataFolder)) or (FolderSize(VCMIDataFolder) < 1024 * 1024))) then
    CopyFolderContents(Heroes3DataFolder, VCMIDataFolder, True);

  if (DirExists(Heroes3Mp3Folder) and ((not DirExists(VCMIMp3Folder)) or (FolderSize(VCMIMp3Folder) < 1024 * 1024))) then
    CopyFolderContents(Heroes3Mp3Folder, VCMIMp3Folder, True);
end;


/// Uninstall ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

var
  DeleteUserDataCheckbox: TNewCheckBox;
  DeleteUserDataLabel: TLabel;


function DeleteFolderContents(const FolderPath: String): Boolean;
var
  FindResult: TFindRec;
  SubPath: String;
begin
  Result := True;

  if FindFirst(FolderPath + '\*', FindResult) then
  begin
    try
      repeat
        if (FindResult.Name <> '.') and (FindResult.Name <> '..') then
        begin
          SubPath := FolderPath + '\' + FindResult.Name;

          if (FindResult.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0 then
          begin
            if not DeleteFolderContents(SubPath) then
            begin
              Result := False;
              Exit;
            end;
            if not RemoveDir(SubPath) then
            begin
              Result := False;
              Exit;
            end;
          end
          else
          begin
            if not DeleteFile(SubPath) then
            begin
              Result := False;
              Exit;
            end;
          end;
        end;
      until not FindNext(FindResult);
    finally
      FindClose(FindResult);
    end;
  end;
end;


procedure PerformFileDeletion;
var
  UserDataFolder: String;
begin
  if (DeleteUserDataCheckbox <> nil) and DeleteUserDataCheckbox.Checked then
  begin
    UserDataFolder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}';

    if DirExists(UserDataFolder) then
    begin
      if DeleteFolderContents(UserDataFolder) then
      begin
        if not RemoveDir(UserDataFolder) then
        begin
          // Log or handle failed root directory removal if necessary
        end;
      end;
    end;
  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
    PerformFileDeletion;
  if CurUninstallStep = usPostUninstall then
    PerformFileDeletion;
end;


procedure UninsNextButtonOnClick(Sender: TObject);
begin
  with UninstallProgressForm.InnerNotebook do
  begin
    ActivePage := Pages[ActivePage.PageIndex + 1];
    if ActivePage.PageIndex = PageCount - 1 then
    begin
      TButton(Sender).Hide;
      UninstallProgressForm.Close;
    end;
  end;
end;


procedure UninsCancelButtonOnClick(Sender: TObject);
begin
  // Optionally handle user cancellation
end;


procedure InitializeUninstallProgressForm();
var
  Page: TNewNotebookPage;
  UninsNextButton: TButton;
begin
  with UninstallProgressForm do
  begin
    // -- Create the "Uninstall" button
    UninsNextButton := TButton.Create(UninstallProgressForm);
    with UninsNextButton do
    begin
      Parent := UninstallProgressForm;
      Top := CancelButton.Top;
      Width := CancelButton.Width;
      Height := CancelButton.Height;
      Left := CancelButton.Left - Width - ScaleX(10);
      Caption := ExpandConstant('{cm:Uninstall}');
      OnClick := @UninsNextButtonOnClick;
      TabOrder := 1; // Ensure this button is first in the tab order
      Default := True; // Make it the default button (triggered by Enter key)
    end;

    // -- Configure the Cancel button so it aborts the form
    CancelButton.Enabled := True;
    CancelButton.ModalResult := mrAbort; 
    CancelButton.OnClick := @UninsCancelButtonOnClick;

    // -- Create a custom page (as the first page in the notebook)
    Page := TNewNotebookPage.Create(InnerNotebook);
    with Page do
    begin
      Parent := InnerNotebook;
      Notebook := InnerNotebook;
      PageIndex := 0; // first page
    end;

    // -- Create our "Delete user data" checkbox on that custom page
    DeleteUserDataCheckbox := TNewCheckBox.Create(UninstallProgressForm);
    with DeleteUserDataCheckbox do
    begin
      Parent := Page;
      Top := ScaleX(20);
      Left := ScaleX(20);
      Width := ScaleX(400);
      Checked := False;
      Caption := ExpandConstant('{cm:DeleteUserData}');
      TabOrder := 0; // Tab focus goes to this control after the Uninstall button
    end;

    // -- Add a label for the additional text
    DeleteUserDataLabel := TLabel.Create(UninstallProgressForm);
    with DeleteUserDataLabel do
    begin
      Parent := Page;
      Top := DeleteUserDataCheckbox.Top + ScaleY(20); // Position below the checkbox
      Left := DeleteUserDataCheckbox.Left + ScaleX(20); // Indent slightly to align with the text
      Width := ScaleX(400);
      Caption := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}';
    end;

    // -- Activate the first page
    InnerNotebook.ActivePage := Page;

    // -- Make InstallingPage the last page
    InstallingPage.PageIndex := InnerNotebook.PageCount - 1;

    // -- Show the form modally; if user clicks Cancel, ShowModal = mrAbort -> Abort uninstallation
    if ShowModal = mrAbort then
      Abort;
  end;
end;