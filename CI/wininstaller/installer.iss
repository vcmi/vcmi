; ============================================================================
; VCMI Installer – Adding a New Translation
; ============================================================================
;
; 1. Download the base ISL file for your language:
;    - Get the appropriate .isl file from the official Inno Setup repository:
;      https://github.com/jrsoftware/issrc/tree/main/Files/Languages
;
; 2. Add VCMI custom messages:
;    - Open the downloaded .isl file and insert all VCMI-specific messages.
;    - Use English.isl (VCMI version) as a reference.
;    - Ensure translations keep placeholders (%1, %2, etc.) intact and match
;      the format of the English version exactly.
;
; 3. Update/translate the following modified messages:
;    These differ from the default Inno Setup language files and are required
;    for VCMI's custom installer functionality.
;
;    ------------------------------------------------------------------------
;    WindowsVersionNotSupported
;    ------------------------------------------------------------------------
;    Why: VCMI adds a more descriptive message for unsupported Windows versions.
;    Original:
;      WindowsVersionNotSupported=This program does not support the version of Windows your computer is running.
;    VCMI version:
;      WindowsVersionNotSupported=This program cannot run on your version of Windows. Please ensure you are using the correct Windows version.
;
;    ------------------------------------------------------------------------
;    PrivilegesRequiredOverride* messages
;    ------------------------------------------------------------------------
;    Why: VCMI customizes privilege escalation messages to clarify installation
;         for all users vs. current user and highlight administrative rights.
;    Messages to add/update:
;      PrivilegesRequiredOverrideTitle
;      PrivilegesRequiredOverrideInstruction
;      PrivilegesRequiredOverrideText1
;      PrivilegesRequiredOverrideText2
;      PrivilegesRequiredOverrideAllUsers
;      PrivilegesRequiredOverrideAllUsersRecommended
;      PrivilegesRequiredOverrideCurrentUser
;      PrivilegesRequiredOverrideCurrentUserRecommended
;
;    Example (VCMI English):
;      PrivilegesRequiredOverrideTitle=Administrator Privileges Required
;      PrivilegesRequiredOverrideInstruction=Choose how to run the installer
;      PrivilegesRequiredOverrideText1=%1 requires administrative rights to install for all users. You can also install just for your account (no administrative rights required) or for all users (administrator rights required).
;      PrivilegesRequiredOverrideText2=%1 can be installed only for your account (no administrative rights required) or for all users (administrator rights required).
;      PrivilegesRequiredOverrideAllUsers=Run as &Administrator (install for all users)
;      PrivilegesRequiredOverrideAllUsersRecommended=Run as &Administrator (recommended)
;      PrivilegesRequiredOverrideCurrentUser=Run as &Standard User (install for me only)
;      PrivilegesRequiredOverrideCurrentUserRecommended=Run as &Standard User (recommended)
;
;    ------------------------------------------------------------------------
;    ConfirmUninstall
;    ------------------------------------------------------------------------
;    Why: VCMI uses a custom uninstall wizard. The message must reflect this.
;    Original:
;      ConfirmUninstall=Are you sure you want to completely remove %1 and all of its components?
;    VCMI version:
;      ConfirmUninstall=Are you sure you want to run the %1 uninstall wizard?
;
; 4. Add the new language to the installer:
;    - In the [Languages] section of the script, register your translation:
;      Name: "YourLanguage"; MessagesFile: "{#LangPath}\YourLanguage.isl"
;
; 5. Verify consistency:
;    - Check all custom messages against the English VCMI ISL file.
;    - Test the installer to confirm all messages appear correctly.


; Manual preprocessor definitions are provided using ISCC.exe parameters.

; #define AppVersion "1.7.0"
; #define AppBuild "1122334455A"
; #define InstallerArch "x64"
; #define AllowedArch "x64compatible"
; #define VCMIFolder "VCMI"
; #define InstallerName "VCMI-Windows"
; #define SourceFilesPath "C:\_VCMI_source\bin\Release"
; #define UCRTFilesPath "C:\Program Files (x86)\Windows Kits\10\Redist\10.0.22621.0\ucrt\DLLs"
; #define LangPath "C:\_VCMI_Source\CI\wininstaller\lang"
; #define LicenseFile "C:\_VCMI_Source\license.txt"
; #define IconFile "C:\_VCMI_Source\clientapp\icons\vcmi.ico"
; #define SmallLogo "C:\_VCMI_Source\CI\wininstaller\vcmismalllogo.bmp"
; #define WizardLogo "C:\_VCMI_Source\CI\wininstaller\vcmilogo.bmp"

#define VCMIFilesFolder "My Games\vcmi"

#define AppComment "VCMI is an open-source engine for Heroes III, offering new and extended possibilities."
#define VCMITeam "VCMI Team"
#define VCMICopyright "Copyright © VCMI Community. All rights reserved."

#define VCMIHome "https://vcmi.eu/"
#define VCMIContact "https://discord.gg/chBT42V"


[Setup]
AppId={#VCMIFolder}.{#InstallerArch}
AppName={#VCMIFolder}
AppVersion={#AppVersion}.{#AppBuild}
AppVerName={#VCMIFolder}
AppPublisher={#VCMITeam}
AppPublisherURL={#VCMIHome}
AppSupportURL={#VCMIContact}
AppComments={#AppComment}
DefaultDirName={code:GetDefaultDir}
DefaultGroupName={#VCMIFolder}
UninstallDisplayIcon={app}\VCMI_launcher.exe
OutputBaseFilename={#InstallerName}
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
CloseApplications=force
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed={#AllowedArch}
LicenseFile={#LicenseFile}
SetupIconFile={#IconFile}
WizardSmallImageFile={#SmallLogo}
WizardImageFile={#WizardLogo}

; Version informations
MinVersion=6.1sp1
VersionInfoCompany={#VCMITeam}
VersionInfoDescription={#VCMIFolder} {#AppVersion} Setup (Build {#AppBuild})
VersionInfoProductName={#VCMIFolder}
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
Source: "{#SourceFilesPath}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb,*.lib,*.exp,*.ilk,*.obj,*.tlog,*.log,*.pch,*.idb,*.res,*.tmp,*.bak,*.sdf,*.ipch,*.vc.db,*.iobj,*.ipdb"; BeforeInstall: RunPreInstallTasks
Source: "{#UCRTFilesPath}\{#InstallerArch}\*"; DestDir: "{app}"; Flags: ignoreversion; Check: IsUCRTNeeded


[Icons]
Name: "{group}\{cm:ShortcutLauncher}"; Filename: "{app}\VCMI_launcher.exe"; Comment: "{cm:ShortcutLauncherComment}"
Name: "{group}\{cm:ShortcutMapEditor}"; Filename: "{app}\VCMI_mapeditor.exe"; Comment: "{cm:ShortcutMapEditorComment}"
Name: "{group}\{cm:ShortcutWebPage}"; Filename: "{#VCMIHome}"; Comment: "{cm:ShortcutWebPageComment}"
Name: "{group}\{cm:ShortcutDiscord}"; Filename: "{#VCMIContact}"; Comment: "{cm:ShortcutDiscordComment}"

Name: "{code:GetUserDesktopFolder}\{cm:ShortcutLauncher}"; Filename: "{app}\VCMI_launcher.exe"; Tasks: desktop; Comment: "{cm:ShortcutLauncherComment}"


[Tasks]
Name: "desktop"; Description: "{cm:CreateDesktopShortcuts}"; GroupDescription: "{cm:SystemIntegration}"; Check: not IsPRInstaller
Name: "startmenu"; Description: "{cm:CreateStartMenuShortcuts}"; GroupDescription: "{cm:SystemIntegration}"; Check: not IsPRInstaller
Name: "fileassociation_h3m"; Description: "{cm:AssociateH3MFiles}"; GroupDescription: "{cm:SystemIntegration}"; Flags: unchecked; Check: not IsPRInstaller
Name: "fileassociation_vcmimap"; Description: "{cm:AssociateVCMIMapFiles}"; GroupDescription: "{cm:SystemIntegration}"; Check: not IsPRInstaller

Name: "firewallrules"; Description: "{cm:AddFirewallRules}"; GroupDescription: "{cm:VCMISettings}"; Check: not IsPRInstaller and IsAdminInstallMode
Name: "h3copyfiles"; Description: "{cm:CopyH3Files}"; GroupDescription: "{cm:VCMISettings}"; Check: not IsPRInstaller and IsHeroes3Installed and IsCopyFilesNeeded

[Registry]
Root: HKCU; Subkey: "Software\{#VCMIFolder}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

Root: HKCU; Subkey: "Software\Classes\.vmap"; ValueType: string; ValueName: ""; ValueData: "VCMI.vmap"; Flags: uninsdeletevalue; Tasks: fileassociation_vcmimap
Root: HKCU; Subkey: "Software\Classes\VCMI.vmap"; ValueType: string; ValueName: ""; ValueData: "{cm:VMAPDescription}"; Flags: uninsdeletekey; Tasks: fileassociation_vcmimap
Root: HKCU; Subkey: "Software\Classes\VCMI.vmap\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\VCMI_mapeditor.exe"" ""%1"""; Tasks: fileassociation_vcmimap

Root: HKCU; Subkey: "Software\Classes\.vcmp"; ValueType: string; ValueName: ""; ValueData: "VCMI.vcmp"; Flags: uninsdeletevalue; Tasks: fileassociation_vcmimap
Root: HKCU; Subkey: "Software\Classes\VCMI.vcmp"; ValueType: string; ValueName: ""; ValueData: "{cm:VCMPDescription}"; Flags: uninsdeletekey; Tasks: fileassociation_vcmimap
Root: HKCU; Subkey: "Software\Classes\VCMI.vcmp\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\VCMI_mapeditor.exe"" ""%1"""; Tasks: fileassociation_vcmimap

Root: HKCU; Subkey: "Software\Classes\.h3m"; ValueType: string; ValueName: ""; ValueData: "VCMI.h3m"; Flags: uninsdeletevalue; Tasks: fileassociation_h3m
Root: HKCU; Subkey: "Software\Classes\VCMI.h3m"; ValueType: string; ValueName: ""; ValueData: "{cm:H3MDescription}"; Flags: uninsdeletekey; Tasks: fileassociation_h3m
Root: HKCU; Subkey: "Software\Classes\VCMI.h3m\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\VCMI_mapeditor.exe"" ""%1"""; Tasks: fileassociation_h3m


[Run]
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=vcmi_server dir=in action=allow program=""{app}\vcmi_server.exe"" enable=yes profile=public,private"; Flags: runhidden; Tasks: firewallrules; Check: IsAdmin
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=vcmi_client dir=in action=allow program=""{app}\vcmi_client.exe"" enable=yes profile=public,private"; Flags: runhidden; Tasks: firewallrules; Check: IsAdmin

Filename: "{app}\VCMI_launcher.exe"; Description: "{cm:RunVCMILauncherAfterInstall}"; Flags: nowait postinstall; Check: ShouldRunLauncher


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

  VCMIMapsFolder, VCMIDataFolder, VCMIMp3Folder: String;
  Heroes3MapsFolder, Heroes3DataFolder, Heroes3Mp3Folder: String;
  
function RegistryQueryPath(Key, ValueName: String): String;
begin
  if RegQueryStringValue(HKLM, Key, ValueName, Result) then
    Exit
  else
    Result := '';
end;


function ShouldRunLauncher(): Boolean;
begin
  Result := True;

  if Pos('SILENT', UpperCase(GetCmdTail())) > 0 then
    Result := False;

  if Pos('LAUNCH', UpperCase(GetCmdTail())) > 0 then
    Result := True;
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
  if IsARM64 then
  begin
    if ExpandConstant('{#InstallerArch}') = 'x86' then
      // For 32-bit installer on ARM64, return the 32-bit Program Files directory
      Result := ExpandConstant('{commonpf32}')
    else
      // For AMR64 installer, return the Program Files directory
      Result := ExpandConstant('{commonpf}')
  end
  else if IsWin64 then
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
    Result := GetCommonProgramFilesDir + '\{#VCMIFolder}'
  else
    // Default to User AppData for non-admin users
    Result := GlobalUserAppdataFolder + '\{#VCMIFolder}';
end;


function GetUserFolderPath(Constant: String): String;
var
  FolderPath: String;
  OriginalUserName: String;
  CurrentSessionUserName: String;
begin
  // Retrieve the current username from the session
  CurrentSessionUserName := '\' + GlobalUserName + '\';

  // Retrieve the original username
  OriginalUserName := '\' + GetUserNameString + '\';

  // Expand the specified constant
  FolderPath := ExpandConstant(Constant);

  // Replace the original username with the current session username in the path
  StringChangeEx(FolderPath, OriginalUserName, CurrentSessionUserName, True);

  // Return the modified folder path
  Result := FolderPath;
end;


procedure OnTaskCheck(Sender: TObject);
var
  idx: Integer;
begin
  // Get the index of the currently clicked task
  idx := WizardForm.TasksList.ItemIndex;

  // Check if the clicked task is the "AddFirewallRules" one
  if WizardForm.TasksList.Items[idx] = ExpandConstant('{cm:AddFirewallRules}') then
  begin
    // If it was just unchecked, show the warning
    if not WizardForm.TasksList.Checked[idx] then
    begin
      MsgBox(ExpandConstant('{cm:Warning}') + '!' + #13#10 + #13#10 + ExpandConstant('{cm:InstallForMeOnly1}') + #13#10 + ExpandConstant('{cm:InstallForMeOnly2}'), mbError, MB_OK);
    end;
  end;
end;


// Specific functions for user folders
function GetUserDocsFolder: String;
begin
  Result := GetUserFolderPath('{userdocs}');
end;


function GetUserAppdataFolder: String;
begin
  Result := GetUserFolderPath('{userappdata}');
end;


function GetUserDesktopFolder(Default: String): String;
begin
  Result := GetUserFolderPath('{userdesktop}');
end;


function IsUCRTNeeded: Boolean;
var
  FileName: String;
begin
  Result := True; // Default to copy the file

  FileName := ExtractFileName(ExpandConstant(CurrentFileName));

  // Only check system if the file name contains "api"
  if Pos('API', UpperCase(FileName)) = 1 then
  begin
    // Check existence based on architecture
    if IsWin64 then
    begin
      if ExpandConstant('{#InstallerArch}') = 'x64' then
        // For 64-bit installer on 64-bit OS, check System32
        Result := not FileExists(ExpandConstant('{win}\System32\' + FileName))
      else
        // For 32-bit installer on 64-bit OS, check SysWOW64
        Result := not FileExists(ExpandConstant('{win}\SysWOW64\' + FileName));
    end
    else
      // For 32-bit OS, always check System32
      Result := not FileExists(ExpandConstant('{win}\System32\' + FileName));
  end;
end;


function IsHeroes3Installed(): Boolean;
begin
  Result := False;

  if (Heroes3Path <> '') then
    Result := True;

end;


function IsCopyFilesNeeded(): Boolean;
begin
  // Check if any of the required folders are not valid
  Result := not (IsFolderValid(VCMIDataFolder) and IsFolderValid(VCMIMapsFolder) and IsFolderValid(VCMIMp3Folder));
  
end;


function IsPRInstaller(): Boolean;
begin
  // Skip Tasks page if this is a PR build
  Result := Pos('-PR-', ExpandConstant('{#InstallerName}')) > 0;

end;


function InitializeSetup(): Boolean;
var
  InstallPath: String;
begin
  // Check if the application is already installed
  IsUpgrade := RegQueryStringValue(HKCU, 'Software\{#VCMIFolder}', 'InstallPath', InstallPath);
 
  // Initialize the global variable during setup
  GlobalUserName := GetCurrentSessionUserName();
  GlobalUserDocsFolder := GetUserDocsFolder();
  GlobalUserAppdataFolder := GetUserAppdataFolder();

  // Define paths for VCMI
  VCMIMapsFolder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}' + '\Maps';
  VCMIDataFolder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}' + '\Data';
  VCMIMp3Folder := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}' + '\Mp3';
  
  // Check for Heroes 3 installation paths
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
  
  if (Heroes3Path <> '') then
  begin
    Heroes3MapsFolder := Heroes3Path + '\Maps';
    Heroes3DataFolder := Heroes3Path + '\Data';
    Heroes3Mp3Folder := Heroes3Path + '\Mp3';
  end;

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
begin
  // Check if the application is already installed
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
  
    // Attach an OnClick event handler to the tasks list
  WizardForm.TasksList.OnClickCheck := @OnTaskCheck;
  
    // Enable word wrap for the ReadyMemo
  WizardForm.ReadyMemo.ScrollBars := ssNone; // No scrollbars
  WizardForm.ReadyMemo.WordWrap := True;

  // Create a custom label for the footer message
  FooterLabel := TLabel.Create(WizardForm);
  FooterLabel.Parent := WizardForm;
  FooterLabel.Caption := '{#VCMIFolder} v' + '{#AppVersion}' + '.' + '{#AppBuild}';
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

  // Don't skip Target page if this is a PR build and upgrade
  if IsPRInstaller and IsUpgrade and (PageID = wpSelectDir) then
  begin
    Result := False;
    Exit;
  end;

  // Skip Tasks page if this is a PR build
  if IsPRInstaller and (PageID = wpSelectTasks) then
  begin
    Result := True;
    Exit;
  end;

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
      WizardForm.DirEdit.Text := GetCommonProgramFilesDir + '\{#VCMIFolder}'
    else
      WizardForm.DirEdit.Text := GlobalUserAppdataFolder + '\{#VCMIFolder}';
  end;

  Result := True;
end;


procedure RemoveLegacyInstaller();
var
  AppFolder: String;
  ResultCode: Integer;
begin
  AppFolder := ExpandConstant('{app}');

  // Silently remove old NSIS installation
  if FileExists(AppFolder + '\Uninstall.exe') then
  begin
    Exec(AppFolder + '\Uninstall.exe', '/S', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

    // Attempt to remove leftovers from target folder to ensure clean install
    if DirExists(AppFolder) then
      DelTree(AppFolder, True, True, False);
  end;
end;


procedure PerformHeroes3FileCopy();
var
  i: Integer;
begin
  // Loop through all tasks to find the "h3copyfiles" task
  for i := 0 to WizardForm.TasksList.Items.Count - 1 do
  begin
    // Check if the current task is "h3copyfiles"
    if WizardForm.TasksList.Items[i] = ExpandConstant('{cm:CopyH3Files}') then
    begin
      // Check if the "h3copyfiles" task is checked
      if WizardForm.TasksList.Checked[i] then
      begin
        
        if IsCopyFilesNeeded then
        begin
          // Copy folders if conditions are met
          if (IsFolderValid(Heroes3MapsFolder) and not IsFolderValid(VCMIMapsFolder)) then
            CopyFolderContents(Heroes3MapsFolder, VCMIMapsFolder, True);

          if (IsFolderValid(Heroes3DataFolder) and not IsFolderValid(VCMIDataFolder)) then
            CopyFolderContents(Heroes3DataFolder, VCMIDataFolder, True);

          if (IsFolderValid(Heroes3Mp3Folder) and not IsFolderValid(VCMIMp3Folder)) then
            CopyFolderContents(Heroes3Mp3Folder, VCMIMp3Folder, True);
        end;
      end;
      Exit; // Task found, exit the loop
    end;
  end;
end;


procedure CreateDefaultSettingsFile();
var
  ConfigDir, SettingsFile, Language, JSONContent: String;
begin
  ConfigDir := GlobalUserDocsFolder + '\' + '{#VCMIFilesFolder}' + '\config';
  SettingsFile := ConfigDir + '\settings.json';

  if not FileExists(SettingsFile) then
  begin
    Language := ActiveLanguage;
    if Language = '' then
      Language := 'english';

      JSONContent :=
        '{' + #13#10 +
        Chr(9) + '"general" : {' + #13#10 +
        Chr(9) + Chr(9) + '"language" : "' + Language + '"' + #13#10 +
        Chr(9) + '}' + #13#10 +
        '}';

    if not DirExists(ConfigDir) then
      ForceDirectories(ConfigDir);

    SaveStringToFile(SettingsFile, JSONContent, False);
  end;
end;


procedure RunPreInstallTasks();
begin
  // Remove Legacy installer when needed
  RemoveLegacyInstaller();
  // Copy H3 files when needed
  PerformHeroes3FileCopy();
  // Create default language JSON - for future use
  // CreateDefaultSettingsFile();
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
  // Repeat delete process after uninstall due logs from killed processes during uninstall
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

