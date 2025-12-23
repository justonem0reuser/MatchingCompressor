#define MyAppVersion "0.1.0"
[Setup]
AppName=MatchingCompressor
AppVersion={#MyAppVersion}
DefaultDirName="{commoncf}\VST3"
DefaultGroupName=MatchingCompressor
OutputBaseFilename=MatchingCompressor-installer
UsePreviousAppDir=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CreateUninstallRegKey=no
Uninstallable=no
DisableDirPage=no
DisableProgramGroupPage=yes
DisableReadyMemo=yes
[Files]
Source: "{#SourcePath}build\MatchingCompressor_artefacts\Release\VST3\MatchingCompressor.vst3\*"; DestDir: "{app}\MatchingCompressor.vst3"; Flags: recursesubdirs restartreplace ignoreversion