#define MyAppName "Wynpix"
#define MyAppVersion "1.0.1.0"
#define MyAppPublisher "hadoooooouken"
#define MyAppURL "https://github.com/hadoooooouken/wynpix"
#define MyAppExeName "wynpix.exe"
#define MyAppIconName "wynpix.ico"
#define MyAppCLSID "{{9E9681D4-2454-445F-8C28-79AAF86F9F15}"

; Target ISA of the packaged build: iscc wynpix.iss (AVX2) or iscc /DIsa=SSE2 wynpix.iss
#ifndef Isa
  #define Isa "AVX2"
#endif

#define MyAvx2OutputBaseFilename "wynpix-setup"
#define MySse2OutputBaseFilename "wynpix-setup-sse2"

#if Isa == "AVX2"
  #define MyReleaseDir "..\release"
  #define MyOutputBaseFilename MyAvx2OutputBaseFilename
#elif Isa == "SSE2"
  #define MyReleaseDir "..\release-sse2"
  #define MyOutputBaseFilename MySse2OutputBaseFilename
#else
  #error Unsupported Isa value. Use AVX2 or SSE2.
#endif

[Setup]
AppId={{BF952DD7-9BB1-484C-806E-DB1587F2E5F0}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
VersionInfoVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\INSTALL
OutputBaseFilename={#MyOutputBaseFilename}
SetupIconFile=res\icons\common\logo\app\{#MyAppIconName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
CloseApplications=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
; Compacting status
english.CompactingFiles=Optimizing file sizes on disk...
ukrainian.CompactingFiles=Оптимізація розміру файлів на диску...
german.CompactingFiles=Dateigrößen auf dem Datenträger optimieren...
spanish.CompactingFiles=Optimizar el tamaño de los archivos en el disco...
french.CompactingFiles=Optimisation de la taille des fichiers sur le disque...
japanese.CompactingFiles=ディスク上のファイルサイズを最適化しています...
turkish.CompactingFiles=Disk üzerindeki dosya boyutları optimize ediliyor...
russian.CompactingFiles=Оптимизация размера файлов на диске...

; Group names
english.GroupFileAssociations=File Associations:
ukrainian.GroupFileAssociations=Асоціації файлів:
german.GroupFileAssociations=Dateizuordnungen:
spanish.GroupFileAssociations=Asociaciones de arquivos:
french.GroupFileAssociations=Associations de fichiers :
japanese.GroupFileAssociations=ファイル関連付け:
turkish.GroupFileAssociations=Dosya İlişkilendirmeleri:
russian.GroupFileAssociations=Ассоциации файлов:

english.GroupAdditionalOptions=Additional options:
ukrainian.GroupAdditionalOptions=Додаткові параметри:
german.GroupAdditionalOptions=Zusätzliche Optionen:
spanish.GroupAdditionalOptions=Opciones adicionales:
french.GroupAdditionalOptions=Options supplémentaires :
japanese.GroupAdditionalOptions=追加オプション:
turkish.GroupAdditionalOptions=Ek seçenekler:
russian.GroupAdditionalOptions=Дополнительные параметры:

; Task description for compacting
english.CompactFilesTaskDesc=Optimize file sizes on disk (compress with LZX)
ukrainian.CompactFilesTaskDesc=Оптимізувати розмір файлів на диску (стиснення LZX)
german.CompactFilesTaskDesc=Dateigrößen auf Datenträger optimieren (LZX-Komprimierung)
spanish.CompactFilesTaskDesc=Optimizar el tamaño de los archivos en disco (compresión LZX)
french.CompactFilesTaskDesc=Optimiser la taille des fichiers sur le disque (compression LZX)
japanese.CompactFilesTaskDesc=ディスク上のファイルサイズを最適化 (LZX圧縮)
turkish.CompactFilesTaskDesc=Dosya boyutlarını diskte optimize et (LZX ile sıkıştır)
russian.CompactFilesTaskDesc=Оптимизировать размер файлов на диске (сжатие LZX)

; Task description for thumbnails
english.ThumbnailsTaskDesc=Generate thumbnails in Windows Explorer
ukrainian.ThumbnailsTaskDesc=Створювати мініатюри у Провіднику Windows
german.ThumbnailsTaskDesc=Minivorschauen im Windows-Explorer generieren
spanish.ThumbnailsTaskDesc=Generar miniaturas en el Explorador de Windows
french.ThumbnailsTaskDesc=Générer des miniatures dans l'Explorateur Windows
japanese.ThumbnailsTaskDesc=Windowsエクスプローラーでサムネイルを生成
turkish.ThumbnailsTaskDesc=Windows Gezgini'nde küçük resimler oluştur
russian.ThumbnailsTaskDesc=Создавать эскизы в Проводнике Windows

; Task description for file associations
english.AssociateFilesTaskDesc=Associate {#MyAppName} with supported image formats
ukrainian.AssociateFilesTaskDesc=Асоціювати {#MyAppName} з підтримуваними форматами зображень
german.AssociateFilesTaskDesc={#MyAppName} mit unterstützten Bildformaten verknüpfen
spanish.AssociateFilesTaskDesc=Asociar {#MyAppName} con los formatos de imagen compatibles
french.AssociateFilesTaskDesc=Associer {#MyAppName} aux formats d'image pris en charge
japanese.AssociateFilesTaskDesc={#MyAppName} をサポートされている画像形式に関連付ける
turkish.AssociateFilesTaskDesc={#MyAppName} uygulamasını desteklenen görüntü formatlarıyla ilişkilendir
russian.AssociateFilesTaskDesc=Ассоциировать {#MyAppName} с поддерживаемыми форматами изображений

; AVX2 error messages (AVX2 installer only)
#if Isa == "AVX2"
english.AVX2ErrorMsg=Installation impossible: your processor does not support AVX2.%n%nThis version requires AVX2. Please download the SSE2 version installer ({#MySse2OutputBaseFilename}.exe) instead.
ukrainian.AVX2ErrorMsg=Встановлення неможливе: ваш процесор не підтримує AVX2.%n%nЦя версія потребує AVX2. Завантажте натомість інсталятор версії SSE2 ({#MySse2OutputBaseFilename}.exe).
german.AVX2ErrorMsg=Installation unmöglich: Ihr Prozessor unterstützt kein AVX2.%n%nDiese Version benötigt AVX2. Bitte laden Sie stattdessen das Installationsprogramm der SSE2-Version ({#MySse2OutputBaseFilename}.exe) herunter.
spanish.AVX2ErrorMsg=Instalación imposible: su procesador no soporta AVX2.%n%nEsta versión requiere AVX2. Descargue en su lugar el instalador de la versión SSE2 ({#MySse2OutputBaseFilename}.exe).
french.AVX2ErrorMsg=Installation impossible : votre processeur ne supporte pas AVX2.%n%nCette version nécessite AVX2. Veuillez plutôt télécharger le programme d'installation de la version SSE2 ({#MySse2OutputBaseFilename}.exe).
japanese.AVX2ErrorMsg=インストールできません: お使いのプロセッサはAVX2をサポートしていません。%n%nこのバージョンにはAVX2が必要です。代わりにSSE2版のインストーラー ({#MySse2OutputBaseFilename}.exe) をダウンロードしてください。
turkish.AVX2ErrorMsg=Kurulum imkansız: işlemciniz AVX2'yi desteklemiyor.%n%nBu sürüm AVX2 gerektirir. Bunun yerine SSE2 sürümünün yükleyicisini ({#MySse2OutputBaseFilename}.exe) indirin.
russian.AVX2ErrorMsg=Установка невозможна: ваш процессор не поддерживает AVX2.%n%nЭта версия требует AVX2. Загрузите вместо неё установщик версии SSE2 ({#MySse2OutputBaseFilename}.exe).
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "associate"; Description: "{cm:AssociateFilesTaskDesc}"; GroupDescription: "{cm:GroupFileAssociations}"
Name: "compact"; Description: "{cm:CompactFilesTaskDesc}"; GroupDescription: "{cm:GroupAdditionalOptions}"
Name: "thumbnails"; Description: "{cm:ThumbnailsTaskDesc}"; GroupDescription: "{cm:GroupAdditionalOptions}"

[Files]
Source: "{#MyReleaseDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyReleaseDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs uninsrestartdelete; Excludes: "{#MyAppExeName},cache\*,conf\*,thumbnails\*"
Source: "res\icons\common\logo\app\{#MyAppIconName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "res\filetypes\*"; DestDir: "{app}\res\filetypes"; Flags: ignoreversion
Source: "{#MyReleaseDir}\wynpixshellex.dll"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppIconName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon; IconFilename: "{app}\{#MyAppIconName}"

[Run]
Filename: "compact.exe"; Parameters: "/c /f /s:""{app}"" /exe:lzx /i"; WorkingDir: "{app}"; StatusMsg: "{cm:CompactingFiles}"; Flags: runhidden; Tasks: compact
Filename: "compact.exe"; Parameters: "/u /f /s:""{app}\models"" /i"; WorkingDir: "{app}"; Flags: runhidden; Tasks: compact
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Registry]
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "Wynpix"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

; SupportedTypes for Applications
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpg"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpeg"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpe"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jfif"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".png"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".gif"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".webp"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".bmp"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".dib"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".tiff"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".tif"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".tga"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".svg"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".svgz"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".ico"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".avif"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".heic"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".heif"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jxl"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".psd"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".dng"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".cr2"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".cr3"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".nef"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".arw"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".orf"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".rw2"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".pef"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".raf"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".raw"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".kra"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".ora"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".cbz"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".exr"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".hdr"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".qoi"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".dds"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jxr"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".hdp"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".wdp"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".ai"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".pdf"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".djvu"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".djv"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".psb"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jp2"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".j2k"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpf"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpx"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jpc"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".jph"; ValueData: ""; Flags: uninsdeletekey; Tasks: associate


; Default value and OpenWithProgids for Extensions
Root: HKA; Subkey: "Software\Classes\.jpg"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpg\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jpg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpeg"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jpeg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpeg\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jpeg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpe"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpe\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jpg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jfif"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jfif\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jpg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.png"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.png"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.png\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.png"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.gif"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.gif"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.gif\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.gif"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.webp"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.webp"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.webp\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.webp"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.bmp"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.bmp"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.bmp\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.bmp"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dib"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.bmp"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dib\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.bmp"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tiff"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.tiff"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tiff\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.tiff"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tif"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.tif"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tif\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.tif"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tga"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.tga"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tga\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.tga"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.svg"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.svg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.svg\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.svg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.svgz"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.svg"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.svgz\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.svg"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ico"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.ico"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ico\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.ico"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.avif"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.avif"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.avif\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.avif"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.heic"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.heic"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.heic\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.heic"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.heif"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.heif"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.heif\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.heif"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jxl"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jxl"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jxl\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jxl"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.psd"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.psd"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.psd\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.psd"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dng"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dng\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cr2"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cr2\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cr3"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cr3\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.nef"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.nef\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.arw"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.arw\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.orf"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.orf\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.rw2"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.rw2\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.pef"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.pef\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.raf"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.raf\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.raw"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.raw\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.raw"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.kra"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.kra"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.kra\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.kra"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ora"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.ora"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ora\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.ora"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cbz"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.cbz"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.cbz\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.cbz"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.exr"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.exr"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.exr\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.exr"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.hdr"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.hdr"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.hdr\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.hdr"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.qoi"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.qoi"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.qoi\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.qoi"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dds"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.dds"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.dds\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.dds"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jxr"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jxr\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jxr"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.hdp"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.hdp\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jxr"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.wdp"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.wdp\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jxr"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ai"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.ai"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.ai\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.ai"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.pdf"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.pdf"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.pdf\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.pdf"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.djvu"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.djvu"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.djvu\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.djvu"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.djv"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.djvu"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.djv\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.djvu"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.psb"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.psb"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.psb\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.psb"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate

Root: HKA; Subkey: "Software\Classes\.jp2"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jp2\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jp2"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.j2k"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.j2k\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jp2"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpf"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpf\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jp2"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpx"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpx\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jp2"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpc"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jpc\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jp2"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jph"; ValueType: string; ValueName: ""; ValueData: "wynpix.AssocFile.jph"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.jph\OpenWithProgids"; ValueType: string; ValueName: "wynpix.AssocFile.jph"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associate

; Associate ProgID details
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile"; ValueType: string; ValueName: ""; ValueData: "Image File (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppIconName}"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

; Specific ProgIDs for each file type with its own icon
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ai"; ValueType: string; ValueName: ""; ValueData: "Adobe Illustrator Artwork (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ai\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\ai.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ai\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.bmp"; ValueType: string; ValueName: ""; ValueData: "BMP Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.bmp\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\bmp.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.bmp\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.exr"; ValueType: string; ValueName: ""; ValueData: "OpenEXR Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.exr\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\exr.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.exr\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.gif"; ValueType: string; ValueName: ""; ValueData: "GIF Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.gif\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\gif.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.gif\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.hdr"; ValueType: string; ValueName: ""; ValueData: "Radiance HDR Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.hdr\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\hdr.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.hdr\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.qoi"; ValueType: string; ValueName: ""; ValueData: "QOI Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.qoi\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\qoi.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.qoi\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.dds"; ValueType: string; ValueName: ""; ValueData: "DirectDraw Surface Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.dds\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\dds.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.dds\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heic"; ValueType: string; ValueName: ""; ValueData: "HEIC Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heic\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\heic.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heic\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heif"; ValueType: string; ValueName: ""; ValueData: "HEIF Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heif\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\heif.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.heif\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpeg"; ValueType: string; ValueName: ""; ValueData: "JPEG Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpeg\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jpeg.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpeg\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpg"; ValueType: string; ValueName: ""; ValueData: "JPEG Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpg\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jpg.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jpg\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxr"; ValueType: string; ValueName: ""; ValueData: "JPEG XR Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxr\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jxr.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxr\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.pdf"; ValueType: string; ValueName: ""; ValueData: "PDF Document (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.pdf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\pdf.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.pdf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.djvu"; ValueType: string; ValueName: ""; ValueData: "DjVu Document (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.djvu\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\djvu.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.djvu\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.png"; ValueType: string; ValueName: ""; ValueData: "PNG Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.png\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\png.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.png\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psb"; ValueType: string; ValueName: ""; ValueData: "Photoshop Large Document (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psb\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\psb.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psb\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psd"; ValueType: string; ValueName: ""; ValueData: "Photoshop Document (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psd\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\psd.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.psd\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.raw"; ValueType: string; ValueName: ""; ValueData: "RAW Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.raw\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\raw.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.raw\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.svg"; ValueType: string; ValueName: ""; ValueData: "SVG Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.svg\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\svg.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.svg\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tif"; ValueType: string; ValueName: ""; ValueData: "TIFF Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tif\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\tif.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tif\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tiff"; ValueType: string; ValueName: ""; ValueData: "TIFF Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tiff\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\tiff.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tiff\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.webp"; ValueType: string; ValueName: ""; ValueData: "WebP Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.webp\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\webp.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.webp\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tga"; ValueType: string; ValueName: ""; ValueData: "TGA Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tga\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\tga.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.tga\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.kra"; ValueType: string; ValueName: ""; ValueData: "Krita Document (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.kra\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\kra.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.kra\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ora"; ValueType: string; ValueName: ""; ValueData: "OpenRaster Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ora\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\ora.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ora\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.cbz"; ValueType: string; ValueName: ""; ValueData: "Comic Book ZIP (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.cbz\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\cbz.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.cbz\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.avif"; ValueType: string; ValueName: ""; ValueData: "AVIF Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.avif\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\avif.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.avif\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxl"; ValueType: string; ValueName: ""; ValueData: "JPEG XL Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxl\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jxl.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jxl\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ico"; ValueType: string; ValueName: ""; ValueData: "Icon File (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ico\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\ico.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.ico\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jp2"; ValueType: string; ValueName: ""; ValueData: "JPEG 2000 Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jp2\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jp2.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jp2\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jph"; ValueType: string; ValueName: ""; ValueData: "High-Throughput JPEG 2000 Image (Wynpix)"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jph\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\res\filetypes\jph.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\wynpix.AssocFile.jph\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

; Register in Default Programs (Windows Vista / 7 / 8 / 10 / 11)
Root: HKA; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "{#MyAppName}"; ValueData: "Software\{#MyAppName}\Capabilities"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#MyAppName}"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Fast, easy to use, and beautiful image viewer."; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpg"; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpeg"; ValueData: "wynpix.AssocFile.jpeg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpe"; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jfif"; ValueData: "wynpix.AssocFile.jpg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".png"; ValueData: "wynpix.AssocFile.png"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".gif"; ValueData: "wynpix.AssocFile.gif"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".webp"; ValueData: "wynpix.AssocFile.webp"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".bmp"; ValueData: "wynpix.AssocFile.bmp"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dib"; ValueData: "wynpix.AssocFile.bmp"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".tiff"; ValueData: "wynpix.AssocFile.tiff"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".tif"; ValueData: "wynpix.AssocFile.tif"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".tga"; ValueData: "wynpix.AssocFile.tga"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".svg"; ValueData: "wynpix.AssocFile.svg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".svgz"; ValueData: "wynpix.AssocFile.svg"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ico"; ValueData: "wynpix.AssocFile.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".avif"; ValueData: "wynpix.AssocFile.avif"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heic"; ValueData: "wynpix.AssocFile.heic"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heif"; ValueData: "wynpix.AssocFile.heif"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jxl"; ValueData: "wynpix.AssocFile.jxl"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".psd"; ValueData: "wynpix.AssocFile.psd"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dng"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".cr2"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".cr3"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".nef"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".arw"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".orf"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".rw2"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".pef"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".raf"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".raw"; ValueData: "wynpix.AssocFile.raw"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".kra"; ValueData: "wynpix.AssocFile.kra"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ora"; ValueData: "wynpix.AssocFile.ora"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".cbz"; ValueData: "wynpix.AssocFile.cbz"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".exr"; ValueData: "wynpix.AssocFile.exr"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".hdr"; ValueData: "wynpix.AssocFile.hdr"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".qoi"; ValueData: "wynpix.AssocFile.qoi"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".dds"; ValueData: "wynpix.AssocFile.dds"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jxr"; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".hdp"; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".wdp"; ValueData: "wynpix.AssocFile.jxr"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ai"; ValueData: "wynpix.AssocFile.ai"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".pdf"; ValueData: "wynpix.AssocFile.pdf"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".djvu"; ValueData: "wynpix.AssocFile.djvu"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".djv"; ValueData: "wynpix.AssocFile.djvu"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".psb"; ValueData: "wynpix.AssocFile.psb"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jp2"; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".j2k"; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpf"; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpx"; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpc"; ValueData: "wynpix.AssocFile.jp2"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jph"; ValueData: "wynpix.AssocFile.jph"; Flags: uninsdeletekey; Tasks: associate

; --- COM / Shell Extension Declarative Registration ---
Root: HKA; Subkey: "Software\Classes\CLSID\{#MyAppCLSID}"; ValueType: string; ValueName: ""; ValueData: "Wynpix Shell Extension"; Flags: uninsdeletekey; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\CLSID\{#MyAppCLSID}\InprocServer32"; ValueType: string; ValueName: ""; ValueData: "{app}\wynpixshellex.dll"; Flags: uninsdeletekey; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\CLSID\{#MyAppCLSID}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"; Flags: uninsdeletekey; Tasks: thumbnails

; System File Associations (IThumbnailProvider GUID: {E357FCCD-A995-4576-B01F-234630154E96})
; These are shared Shell keys. Ownership-aware [Code] handles uninstall;
; never add recursive uninsdelete flags here.
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.webp\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.svg\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.tiff\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.tif\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.tga\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.avif\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.heic\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.heif\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jxl\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.psd\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.dng\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.cr2\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.cr3\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.nef\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.arw\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.orf\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.rw2\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.pef\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.raf\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.raw\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.kra\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.ora\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.cbz\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.exr\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.hdr\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.qoi\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.dds\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jxr\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.hdp\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.wdp\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.ai\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.djvu\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.djv\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.psb\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jp2\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.j2k\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jpf\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jpx\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jpc\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails
Root: HKA; Subkey: "Software\Classes\SystemFileAssociations\.jph\ShellEx\{{E357FCCD-A995-4576-B01F-234630154E96}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppCLSID}"; Tasks: thumbnails

[Code]
const
#if Isa == "AVX2"
  PF_AVX2_INSTRUCTIONS_AVAILABLE = 40;
#endif
  ThumbnailProviderClassId = '{9E9681D4-2454-445F-8C28-79AAF86F9F15}';
  ThumbnailProviderDllName = 'wynpixshellex.dll';
  ThumbnailHandlerCategory = '{E357FCCD-A995-4576-B01F-234630154E96}';
  SystemFileAssociationsPath =
    'Software\Classes\SystemFileAssociations\';
  DirectFileAssociationsPath = 'Software\Classes\';
  ThumbnailProviderClassPath = 'Software\Classes\CLSID\' +
    ThumbnailProviderClassId;
  SupportedThumbnailExtensions =
    '.webp;.svg;.tiff;.tif;.tga;.avif;.heic;.heif;.jxl;.psd;.dng;.cr2;' +
    '.cr3;.nef;.arw;.orf;.rw2;.pef;.raf;.raw;.kra;.ora;.cbz;.exr;.hdr;.qoi;' +
    '.dds;.jxr;.hdp;.wdp;.ai;.djvu;.djv;.psb;.jp2;.j2k;.jpf;.jpx;.jpc;.jph';
  ObsoleteThumbnailExtensions =
    '.jpg;.jpeg;.jpe;.jfif;.png;.gif;.bmp;.dib;.ico;.pdf;.svgz';
  LegacyThumbnailProgIds =
    'wynpix.AssocFile.raw;wynpix.AssocFile.kra;' +
    'wynpix.AssocFile.ora;wynpix.AssocFile.cbz;' +
    'wynpix.AssocFile.webp;' +
    'wynpix.AssocFile.jxl;wynpix.AssocFile.avif;' +
    'wynpix.AssocFile.heic;wynpix.AssocFile.heif;' +
    'wynpix.AssocFile.exr;wynpix.AssocFile.hdr;' +
    'wynpix.AssocFile.tga;wynpix.AssocFile.jxr;' +
    'wynpix.AssocFile.qoi;wynpix.AssocFile.dds;' +
    'wynpix.AssocFile.psd;wynpix.AssocFile.psb;' +
    'wynpix.AssocFile.ai;wynpix.AssocFile.pdf;' +
    'wynpix.AssocFile.djvu;' +
    'wynpix.AssocFile.tif;wynpix.AssocFile.tiff;' +
    'wynpix.AssocFile.jp2;wynpix.AssocFile.jph;' +
    'wynpix.AssocFile.svg';

var
  ForeignThumbnailHandlerPaths: array of String;
  ForeignThumbnailHandlerProviders: array of String;

#if Isa == "AVX2"
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
  external 'IsProcessorFeaturePresent@kernel32.dll stdcall';

function InitializeSetup(): Boolean;
begin
  Result := IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
  if not Result then
    MsgBox(CustomMessage('AVX2ErrorMsg'), mbError, MB_OK);
end;
#endif

function RemoveOwnedThumbnailHandler(RootKey: HKEY;
  const AssociationPath: String): Boolean;
var
  HandlerPath: String;
  RegisteredProvider: String;
begin
  Result := True;
  HandlerPath := AssociationPath + '\ShellEx\' + ThumbnailHandlerCategory;

  if RegValueExists(RootKey, HandlerPath, '') then
  begin
    if not RegQueryStringValue(RootKey, HandlerPath, '',
      RegisteredProvider) then
    begin
      Log(Format('Failed to read thumbnail handler ownership: %s', [HandlerPath]));
      Result := False;
      exit;
    end;

    if CompareText(RegisteredProvider, ThumbnailProviderClassId) <> 0 then
      exit;

    if not RegDeleteValue(RootKey, HandlerPath, '') then
    begin
      Log(Format('Failed to remove Wynpix thumbnail handler: %s', [HandlerPath]));
      Result := False;
      exit;
    end;

    RegDeleteKeyIfEmpty(RootKey, HandlerPath);
  end;
end;

function RemoveOwnedThumbnailHandlers(RootKey: HKEY;
  const AssociationPrefix: String; Entries: String): Boolean;
var
  SeparatorPosition: Integer;
  Entry: String;
begin
  Result := True;
  while Entries <> '' do
  begin
    SeparatorPosition := Pos(';', Entries);
    if SeparatorPosition = 0 then
    begin
      Entry := Entries;
      Entries := '';
    end
    else
    begin
      Entry := Copy(Entries, 1, SeparatorPosition - 1);
      Delete(Entries, 1, SeparatorPosition);
    end;

    if (Entry <> '') and
       not RemoveOwnedThumbnailHandler(RootKey, AssociationPrefix + Entry) then
      Result := False;
  end;
end;

function HasOwnedThumbnailHandler(RootKey: HKEY;
  const AssociationPath: String): Boolean;
var
  HandlerPath: String;
  RegisteredProvider: String;
begin
  HandlerPath := AssociationPath + '\ShellEx\' + ThumbnailHandlerCategory;
  Result := False;
  if not RegValueExists(RootKey, HandlerPath, '') then
    exit;

  if not RegQueryStringValue(RootKey, HandlerPath, '', RegisteredProvider) then
    exit;

  Result := CompareText(RegisteredProvider, ThumbnailProviderClassId) = 0;
end;

function HasOwnedThumbnailHandlers(RootKey: HKEY;
  const AssociationPrefix: String; Entries: String): Boolean;
var
  SeparatorPosition: Integer;
  Entry: String;
begin
  Result := False;
  while Entries <> '' do
  begin
    SeparatorPosition := Pos(';', Entries);
    if SeparatorPosition = 0 then
    begin
      Entry := Entries;
      Entries := '';
    end
    else
    begin
      Entry := Copy(Entries, 1, SeparatorPosition - 1);
      Delete(Entries, 1, SeparatorPosition);
    end;

    if (Entry <> '') and
       HasOwnedThumbnailHandler(RootKey, AssociationPrefix + Entry) then
    begin
      Result := True;
      exit;
    end;
  end;
end;

function HasOwnedThumbnailRegistration(RootKey: HKEY): Boolean;
begin
  Result := RegKeyExists(RootKey, ThumbnailProviderClassPath) or
    HasOwnedThumbnailHandlers(RootKey, SystemFileAssociationsPath,
      SupportedThumbnailExtensions) or
    HasOwnedThumbnailHandlers(RootKey, SystemFileAssociationsPath,
      ObsoleteThumbnailExtensions) or
    HasOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
      SupportedThumbnailExtensions) or
    HasOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
      ObsoleteThumbnailExtensions) or
    HasOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
      LegacyThumbnailProgIds);
end;

procedure ClearForeignThumbnailHandlerBackups;
begin
  SetArrayLength(ForeignThumbnailHandlerPaths, 0);
  SetArrayLength(ForeignThumbnailHandlerProviders, 0);
end;

function BackupForeignThumbnailHandler(RootKey: HKEY;
  const AssociationPath: String): Boolean;
var
  BackupIndex: Integer;
  HandlerPath: String;
  RegisteredProvider: String;
begin
  Result := True;
  HandlerPath := AssociationPath + '\ShellEx\' + ThumbnailHandlerCategory;
  if not RegValueExists(RootKey, HandlerPath, '') then
    exit;

  if not RegQueryStringValue(RootKey, HandlerPath, '', RegisteredProvider) then
  begin
    Log(Format('Failed to back up thumbnail handler ownership: %s', [HandlerPath]));
    Result := False;
    exit;
  end;

  if CompareText(RegisteredProvider, ThumbnailProviderClassId) = 0 then
    exit;

  BackupIndex := GetArrayLength(ForeignThumbnailHandlerPaths);
  SetArrayLength(ForeignThumbnailHandlerPaths, BackupIndex + 1);
  SetArrayLength(ForeignThumbnailHandlerProviders, BackupIndex + 1);
  ForeignThumbnailHandlerPaths[BackupIndex] := HandlerPath;
  ForeignThumbnailHandlerProviders[BackupIndex] := RegisteredProvider;
end;

function BackupForeignThumbnailHandlers(RootKey: HKEY;
  const AssociationPrefix: String; Entries: String): Boolean;
var
  SeparatorPosition: Integer;
  Entry: String;
begin
  Result := True;
  while Entries <> '' do
  begin
    SeparatorPosition := Pos(';', Entries);
    if SeparatorPosition = 0 then
    begin
      Entry := Entries;
      Entries := '';
    end
    else
    begin
      Entry := Copy(Entries, 1, SeparatorPosition - 1);
      Delete(Entries, 1, SeparatorPosition);
    end;

    if (Entry <> '') and
       not BackupForeignThumbnailHandler(RootKey,
         AssociationPrefix + Entry) then
      Result := False;
  end;
end;

function BackupForeignThumbnailRegistrations(RootKey: HKEY): Boolean;
begin
  ClearForeignThumbnailHandlerBackups;
  Result := BackupForeignThumbnailHandlers(RootKey,
    SystemFileAssociationsPath, SupportedThumbnailExtensions);
  if not BackupForeignThumbnailHandlers(RootKey,
    SystemFileAssociationsPath, ObsoleteThumbnailExtensions) then
    Result := False;
end;

function RestoreForeignThumbnailHandlerBackups(RootKey: HKEY): Boolean;
var
  BackupIndex: Integer;
begin
  Result := True;
  if GetArrayLength(ForeignThumbnailHandlerPaths) = 0 then
    exit;

  for BackupIndex := 0 to
    GetArrayLength(ForeignThumbnailHandlerPaths) - 1 do
  begin
    if not RegValueExists(RootKey,
      ForeignThumbnailHandlerPaths[BackupIndex], '') then
    begin
      if not RegWriteStringValue(RootKey,
        ForeignThumbnailHandlerPaths[BackupIndex], '',
        ForeignThumbnailHandlerProviders[BackupIndex]) then
      begin
        Log(Format('Failed to restore foreign thumbnail handler: %s', [ForeignThumbnailHandlerPaths[BackupIndex]]));
        Result := False;
      end;
    end;
  end;

  if Result then
    ClearForeignThumbnailHandlerBackups;
end;

function RemoveThumbnailProviderClass(RootKey: HKEY): Boolean;
var
  InprocServerPath: String;
  RegisteredServer: String;
begin
  Result := True;
  if not RegKeyExists(RootKey, ThumbnailProviderClassPath) then
    exit;

  InprocServerPath := ThumbnailProviderClassPath + '\InprocServer32';
  if not RegQueryStringValue(RootKey, InprocServerPath, '',
    RegisteredServer) then
  begin
    Log(Format('Failed to verify thumbnail provider ownership: %s', [InprocServerPath]));
    Result := False;
    exit;
  end;

  if CompareText(ExtractFileName(RegisteredServer),
    ThumbnailProviderDllName) <> 0 then
  begin
    Log(Format('Refusing to remove a foreign thumbnail provider class: %s', [RegisteredServer]));
    Result := False;
    exit;
  end;

  if not RegDeleteKeyIncludingSubkeys(RootKey,
    ThumbnailProviderClassPath) then
  begin
    Log(Format('Failed to remove Wynpix thumbnail provider class: %s', [ThumbnailProviderClassPath]));
    Result := False;
  end;
end;

function CleanupThumbnailRegistrations(RootKey: HKEY;
  RemoveSupportedHandlers: Boolean): Boolean;
begin
  Result := True;

  if not RemoveOwnedThumbnailHandlers(RootKey, SystemFileAssociationsPath,
    ObsoleteThumbnailExtensions) then
    Result := False;
  if not RemoveOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
    ObsoleteThumbnailExtensions) then
    Result := False;

  { Migrate old regsvr32 routes to the installer's SystemFileAssociations. }
  if not RemoveOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
    SupportedThumbnailExtensions) then
    Result := False;
  if not RemoveOwnedThumbnailHandlers(RootKey, DirectFileAssociationsPath,
    LegacyThumbnailProgIds) then
    Result := False;

  if RemoveSupportedHandlers then
  begin
    if not RemoveOwnedThumbnailHandlers(RootKey, SystemFileAssociationsPath,
      SupportedThumbnailExtensions) then
      Result := False;
    if not RemoveThumbnailProviderClass(RootKey) then
      Result := False;
  end;
end;

procedure RaiseThumbnailCleanupFailure;
begin
  RaiseException(
    'Wynpix could not safely update its thumbnail handler registrations. ' +
    'See the setup log for details.');
end;

procedure WarnAboutMachineThumbnailProvider;
begin
  Log('A machine-wide Wynpix thumbnail provider registration remains.');
  SuppressibleMsgBox(
    'A machine-wide Wynpix thumbnail provider is still registered. ' +
    'Run Setup as administrator to update or remove it.',
    mbError, MB_OK, IDOK);
end;

procedure TryRestoreForeignThumbnailHandlers(ShowError: Boolean);
begin
  if GetArrayLength(ForeignThumbnailHandlerPaths) = 0 then
    exit;

  if not RestoreForeignThumbnailHandlerBackups(HKA64) then
  begin
    Log('Failed to restore one or more foreign thumbnail handlers.');
    if ShowError then
      SuppressibleMsgBox(
        'A thumbnail handler owned by another application could not be ' +
        'restored. See the uninstall log for details.',
        mbError, MB_OK, IDOK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  CleanupSucceeded: Boolean;
  RemoveSupportedHandlers: Boolean;
begin
  if CurStep <> ssInstall then
    exit;

  RemoveSupportedHandlers := not WizardIsTaskSelected('thumbnails');
  { HKA follows the selected install scope. Do not mutate HKCU from an admin
    install because over-the-shoulder elevation may use another account. }
  CleanupSucceeded :=
    CleanupThumbnailRegistrations(HKA64, RemoveSupportedHandlers);

  if not IsAdminInstallMode and
     HasOwnedThumbnailRegistration(HKLM64) then
    WarnAboutMachineThumbnailProvider;

  if not CleanupSucceeded then
    RaiseThumbnailCleanupFailure;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  CleanupSucceeded: Boolean;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    TryRestoreForeignThumbnailHandlers(False);
    exit;
  end;

  if CurUninstallStep <> usUninstall then
    exit;

  { Older uninstall logs contain recursive deletion entries for these shared
    ShellEx leaves. Preserve a provider that took ownership after installation. }
  if not BackupForeignThumbnailRegistrations(HKA64) then
    RaiseThumbnailCleanupFailure;

  { Match the scope that owns this uninstaller. }
  CleanupSucceeded := CleanupThumbnailRegistrations(HKA64, True);

  if not IsAdminInstallMode and
     HasOwnedThumbnailRegistration(HKLM64) then
    WarnAboutMachineThumbnailProvider;

  if not CleanupSucceeded then
    RaiseThumbnailCleanupFailure;
end;

procedure DeinitializeUninstall;
begin
  { Retry when a fatal uninstall error prevented the post-uninstall step. }
  TryRestoreForeignThumbnailHandlers(True);
end;
