; *** Inno Setup version 6.5.0+ Polish messages ***
; Proofreading, corrections and 5.5.7-6.5.0+ updates:
; Łukasz Abramczuk <lukasz.abramczuk at gmail.com>
; Original translations up to 5.5.7:
; Krzysztof Cynarski <krzysztof at cynarski.net>
; To download user-contributed translations of this file, go to:
;   https://jrsoftware.org/files/istrans/
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).
; last update: 2025/06/23 

[LangOptions]
LanguageName=Polski
LanguageID=$0415
LanguageCodePage=1250

[Messages]

; *** Application titles
SetupAppTitle=Instalator
SetupWindowTitle=Instalacja - %1
UninstallAppTitle=Dezinstalator
UninstallAppFullTitle=Dezinstalacja - %1

; *** Misc. common
InformationTitle=Informacja
ConfirmTitle=Potwierdź
ErrorTitle=Błąd

; *** SetupLdr messages
SetupLdrStartupMessage=Ten program zainstaluje aplikację %1. Czy chcesz kontynuować?
LdrCannotCreateTemp=Nie można utworzyć pliku tymczasowego. Instalacja przerwana
LdrCannotExecTemp=Nie można uruchomić pliku z folderu tymczasowego. Instalacja przerwana
HelpTextNote=

; *** Startup error messages
LastErrorMessage=%1.%n%nBłąd %2: %3
SetupFileMissing=W folderze instalacyjnym brakuje pliku %1.%nProszę o przywrócenie brakujących plików lub uzyskanie nowej kopii programu instalacyjnego.
SetupFileCorrupt=Pliki instalacyjne są uszkodzone. Zaleca się uzyskanie nowej kopii programu instalacyjnego.
SetupFileCorruptOrWrongVer=Pliki instalacyjne są uszkodzone lub niezgodne z tą wersją instalatora. Proszę rozwiązać problem lub uzyskać nową kopię programu instalacyjnego.
InvalidParameter=W linii komend przekazano nieprawidłowy parametr:%n%n%1
SetupAlreadyRunning=Instalator jest już uruchomiony.
WindowsVersionNotSupported=Ten program nie moze zostac uruchomiony na uzywanej wersji systemu Windows. Upewnij sie, ze uzywasz odpowiedniej architektury systemu Windows (32-bitowej lub 64-bitowej) i wlasciwej wersji tego programu.
WindowsServicePackRequired=Ta aplikacja wymaga systemu %1 z dodatkiem Service Pack %2 lub nowszym.
NotOnThisPlatform=Tej aplikacji nie można uruchomić w systemie %1.
OnlyOnThisPlatform=Ta aplikacja wymaga systemu %1.
OnlyOnTheseArchitectures=Ta aplikacja może być uruchomiona tylko w systemie Windows zaprojektowanym dla procesorów o architekturze:%n%n%1
WinVersionTooLowError=Ta aplikacja wymaga systemu %1 w wersji %2 lub nowszej.
WinVersionTooHighError=Ta aplikacja nie może być zainstalowana w systemie %1 w wersji %2 lub nowszej.
AdminPrivilegesRequired=Aby przeprowadzić instalację tej aplikacji, konto użytkownika systemu musi posiadać uprawnienia administratora.
PowerUserPrivilegesRequired=Aby przeprowadzić instalację tej aplikacji, konto użytkownika systemu musi posiadać uprawnienia administratora lub użytkownika zaawansowanego.
SetupAppRunningError=Instalator wykrył, iż aplikacja %1 jest aktualnie uruchomiona.%n%nPrzed wciśnięciem przycisku OK zamknij wszystkie procesy aplikacji. Kliknij przycisk Anuluj, aby przerwać instalację.
UninstallAppRunningError=Dezinstalator wykrył, iż aplikacja %1 jest aktualnie uruchomiona.%n%nPrzed wciśnięciem przycisku OK zamknij wszystkie procesy aplikacji. Kliknij przycisk Anuluj, aby przerwać dezinstalację.

; *** Startup questions
PrivilegesRequiredOverrideTitle=Tryb instalacji – Uprawnienia
PrivilegesRequiredOverrideInstruction=Wybierz sposób uruchomienia instalatora
PrivilegesRequiredOverrideText1=Do zainstalowania %1 dla wszystkich użytkowników wymagane są uprawnienia administratora.%nMożesz również zainstalować aplikację tylko dla swojego konta bez uprawnień administratora.
PrivilegesRequiredOverrideText2=%1 może zostać zainstalowana tylko dla twojego konta (bez uprawnień administratora) lub dla wszystkich użytkowników (wymagane są uprawnienia administratora).
PrivilegesRequiredOverrideAllUsers=Uruchom jako &Administrator (instalacja dla wszystkich użytkowników)
PrivilegesRequiredOverrideAllUsersRecommended=Uruchom jako &Administrator (zalecane)
PrivilegesRequiredOverrideCurrentUser=Uruchom jako &Użytkownik standardowy (instalacja tylko dla mnie)
PrivilegesRequiredOverrideCurrentUserRecommended=Uruchom jako &Użytkownik standardowy (zalecane)

; *** Misc. errors
ErrorCreatingDir=Instalator nie mógł utworzyć katalogu "%1"
ErrorTooManyFilesInDir=Nie można utworzyć pliku w katalogu "%1", ponieważ zawiera on zbyt wiele plików

; *** Setup common messages
ExitSetupTitle=Zakończ instalację
ExitSetupMessage=Instalacja nie została zakończona. Jeżeli przerwiesz ją teraz, aplikacja nie zostanie zainstalowana. Można ponowić instalację później poprzez uruchamianie instalatora.%n%nCzy chcesz przerwać instalację?
AboutSetupMenuItem=&O instalatorze...
AboutSetupTitle=O instalatorze
AboutSetupMessage=%1 wersja %2%n%3%n%n Strona domowa %1:%n%4
AboutSetupNote=
TranslatorNote=Wersja polska: %nŁukasz Abramczuk <lukasz.abramczuk at gmail.com>%nKrzysztof Cynarski <krzysztof at cynarski.net>

; *** Buttons
ButtonBack=< &Wstecz
ButtonNext=&Dalej >
ButtonInstall=&Instaluj
ButtonOK=OK
ButtonCancel=Anuluj
ButtonYes=&Tak
ButtonYesToAll=Tak na &wszystkie
ButtonNo=&Nie
ButtonNoToAll=N&ie na wszystkie
ButtonFinish=&Zakończ
ButtonBrowse=&Przeglądaj...
ButtonWizardBrowse=P&rzeglądaj...
ButtonNewFolder=&Utwórz nowy folder

; *** "Select Language" dialog messages
SelectLanguageTitle=Język instalacji
SelectLanguageLabel=Wybierz język używany podczas instalacji:

; *** Common wizard text
ClickNext=Kliknij przycisk Dalej, aby kontynuować, lub Anuluj, aby zakończyć instalację.
BeveledLabel=
BrowseDialogTitle=Wskaż folder
BrowseDialogLabel=Wybierz folder z poniższej listy, a następnie kliknij przycisk OK.
NewFolderName=Nowy folder

; *** "Welcome" wizard page
WelcomeLabel1=Witamy w instalatorze aplikacji [name]
WelcomeLabel2=Aplikacja [name/ver] zostanie teraz zainstalowana na komputerze.%n%nZalecane jest zamknięcie wszystkich innych uruchomionych programów przed rozpoczęciem procesu instalacji.

; *** "Password" wizard page
WizardPassword=Hasło
PasswordLabel1=Ta instalacja jest zabezpieczona hasłem.
PasswordLabel3=Podaj hasło, a następnie kliknij przycisk Dalej, aby kontynuować. W hasłach rozróżniane są wielkie i małe litery.
PasswordEditLabel=&Hasło:
IncorrectPassword=Wprowadzone hasło jest nieprawidłowe. Spróbuj ponownie.

; *** "License Agreement" wizard page
WizardLicense=Umowa Licencyjna
LicenseLabel=Przed kontynuacją należy zapoznać się z poniższą ważną informacją.
LicenseLabel3=Proszę przeczytać tekst Umowy Licencyjnej. Przed kontynuacją instalacji należy zaakceptować warunki umowy.
LicenseAccepted=&Akceptuję warunki umowy
LicenseNotAccepted=&Nie akceptuję warunków umowy

; *** "Information" wizard pages
WizardInfoBefore=Informacja
InfoBeforeLabel=Przed kontynuacją należy zapoznać się z poniższą informacją.
InfoBeforeClickLabel=Kiedy będziesz gotowy do instalacji, kliknij przycisk Dalej.
WizardInfoAfter=Informacja
InfoAfterLabel=Przed kontynuacją należy zapoznać się z poniższą informacją.
InfoAfterClickLabel=Gdy będziesz gotowy do zakończenia instalacji, kliknij przycisk Dalej.

; *** "User Information" wizard page
WizardUserInfo=Dane użytkownika
UserInfoDesc=Proszę podać swoje dane.
UserInfoName=Nazwa &użytkownika:
UserInfoOrg=&Organizacja:
UserInfoSerial=Numer &seryjny:
UserInfoNameRequired=Nazwa użytkownika jest wymagana.

; *** "Select Destination Location" wizard page
WizardSelectDir=Lokalizacja docelowa
SelectDirDesc=Gdzie ma zostać zainstalowana aplikacja [name]?
SelectDirLabel3=Instalator zainstaluje aplikację [name] do wskazanego poniżej folderu.
SelectDirBrowseLabel=Kliknij przycisk Dalej, aby kontynuować. Jeśli chcesz wskazać inny folder, kliknij przycisk Przeglądaj.
DiskSpaceGBLabel=Instalacja wymaga przynajmniej [gb] GB wolnego miejsca na dysku.
DiskSpaceMBLabel=Instalacja wymaga przynajmniej [mb] MB wolnego miejsca na dysku.
CannotInstallToNetworkDrive=Instalator nie może zainstalować aplikacji na dysku sieciowym.
CannotInstallToUNCPath=Instalator nie może zainstalować aplikacji w ścieżce UNC.
InvalidPath=Należy wprowadzić pełną ścieżkę wraz z literą dysku, np.:%n%nC:\PROGRAM%n%nlub ścieżkę sieciową (UNC) w formacie:%n%n\\serwer\udział
InvalidDrive=Wybrany dysk lub udostępniony folder sieciowy nie istnieje. Proszę wybrać inny.
DiskSpaceWarningTitle=Niewystarczająca ilość wolnego miejsca na dysku
DiskSpaceWarning=Instalator wymaga co najmniej %1 KB wolnego miejsca na dysku. Wybrany dysk posiada tylko %2 KB dostępnego miejsca.%n%nCzy mimo to chcesz kontynuować?
DirNameTooLong=Nazwa folderu lub ścieżki jest za długa.
InvalidDirName=Niepoprawna nazwa folderu.
BadDirName32=Nazwa folderu nie może zawierać żadnego z następujących znaków:%n%n%1
DirExistsTitle=Folder już istnieje
DirExists=Poniższy folder już istnieje:%n%n%1%n%nCzy mimo to chcesz zainstalować aplikację w tym folderze?
DirDoesntExistTitle=Folder nie istnieje
DirDoesntExist=Poniższy folder nie istnieje:%n%n%1%n%nCzy chcesz, aby został utworzony?

; *** "Select Components" wizard page
WizardSelectComponents=Komponenty instalacji
SelectComponentsDesc=Które komponenty mają zostać zainstalowane?
SelectComponentsLabel2=Zaznacz komponenty, które chcesz zainstalować i odznacz te, których nie chcesz zainstalować. Kliknij przycisk Dalej, aby kontynuować.
FullInstallation=Instalacja pełna
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=Instalacja podstawowa
CustomInstallation=Instalacja użytkownika
NoUninstallWarningTitle=Zainstalowane komponenty
NoUninstallWarning=Instalator wykrył, że na komputerze są już zainstalowane następujące komponenty:%n%n%1%n%nOdznaczenie któregokolwiek z nich nie spowoduje ich dezinstalacji.%n%nCzy pomimo tego chcesz kontynuować?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceGBLabel=Wybrane komponenty wymagają co najmniej [gb] GB na dysku.
ComponentsDiskSpaceMBLabel=Wybrane komponenty wymagają co najmniej [mb] MB na dysku.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Zadania dodatkowe
SelectTasksDesc=Które zadania dodatkowe mają zostać wykonane?
SelectTasksLabel2=Zaznacz dodatkowe zadania, które instalator ma wykonać podczas instalacji aplikacji [name], a następnie kliknij przycisk Dalej, aby kontynuować.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Folder Menu Start
SelectStartMenuFolderDesc=Gdzie mają zostać umieszczone skróty do aplikacji?
SelectStartMenuFolderLabel3=Instalator utworzy skróty do aplikacji we wskazanym poniżej folderze Menu Start.
SelectStartMenuFolderBrowseLabel=Kliknij przycisk Dalej, aby kontynuować. Jeśli chcesz wskazać inny folder, kliknij przycisk Przeglądaj.
MustEnterGroupName=Musisz wprowadzić nazwę folderu.
GroupNameTooLong=Nazwa folderu lub ścieżki jest za długa.
InvalidGroupName=Niepoprawna nazwa folderu.
BadGroupName=Nazwa folderu nie może zawierać żadnego z następujących znaków:%n%n%1
NoProgramGroupCheck2=&Nie twórz folderu w Menu Start

; *** "Ready to Install" wizard page
WizardReady=Gotowy do rozpoczęcia instalacji
ReadyLabel1=Instalator jest już gotowy do rozpoczęcia instalacji aplikacji [name] na komputerze.
ReadyLabel2a=Kliknij przycisk Instaluj, aby rozpocząć instalację lub Wstecz, jeśli chcesz przejrzeć lub zmienić ustawienia.
ReadyLabel2b=Kliknij przycisk Instaluj, aby kontynuować instalację.
ReadyMemoUserInfo=Dane użytkownika:
ReadyMemoDir=Lokalizacja docelowa:
ReadyMemoType=Rodzaj instalacji:
ReadyMemoComponents=Wybrane komponenty:
ReadyMemoGroup=Folder w Menu Start:
ReadyMemoTasks=Dodatkowe zadania:

; *** TDownloadWizardPage wizard page and DownloadTemporaryFile
DownloadingLabel2=Pobieranie plików...
ButtonStopDownload=&Zatrzymaj pobieranie
StopDownload=Czy na pewno chcesz zatrzymać pobieranie?
ErrorDownloadAborted=Pobieranie przerwane
ErrorDownloadFailed=Błąd pobierania: %1 %2
ErrorDownloadSizeFailed=Pobieranie informacji o rozmiarze nie powiodło się: %1 %2
ErrorProgress=Nieprawidłowy postęp: %1 z %2
ErrorFileSize=Nieprawidłowy rozmiar pliku: oczekiwano %1, otrzymano %2

; *** TExtractionWizardPage wizard page and ExtractArchive
ExtractingLabel=Wypakowywanie plików...
ButtonStopExtraction=&Zatrzymaj wypakowywanie
StopExtraction=Azy na pewno chcesz zatrzymać wypakowywanie?
ErrorExtractionAborted=Wypakowywanie przerwane
ErrorExtractionFailed=Błąd wypakowywania: %1

; *** Archive extraction failure details
ArchiveIncorrectPassword=Nieprawidłowe hasło
ArchiveIsCorrupted=Archiwum jest uszkodzone
ArchiveUnsupportedFormat=Format archiwum nie jest wspierany

; *** "Preparing to Install" wizard page
WizardPreparing=Przygotowanie do instalacji
PreparingDesc=Instalator przygotowuje instalację aplikacji [name] na komputerze.
PreviousInstallNotCompleted=Instalacja/dezinstalacja poprzedniej wersji aplikacji nie została zakończona. Aby zakończyć instalację, należy ponownie uruchomić komputer. %n%nNastępnie ponownie uruchom instalator, aby zakończyć instalację aplikacji [name].
CannotContinue=Instalator nie może kontynuować. Kliknij przycisk Anuluj, aby przerwać instalację.
ApplicationsFound=Poniższe aplikacje używają plików, które muszą zostać uaktualnione przez instalator. Zaleca się zezwolić na automatyczne zamknięcie tych aplikacji przez program instalacyjny.
ApplicationsFound2=Poniższe aplikacje używają plików, które muszą zostać uaktualnione przez instalator. Zaleca się zezwolić na automatyczne zamknięcie tych aplikacji przez program instalacyjny. Po zakończonej instalacji instalator podejmie próbę ich ponownego uruchomienia.
CloseApplications=&Automatycznie zamknij aplikacje
DontCloseApplications=&Nie zamykaj aplikacji
ErrorCloseApplications=Instalator nie był w stanie automatycznie zamknąć wymaganych aplikacji. Zalecane jest zamknięcie wszystkich aplikacji, które aktualnie używają uaktualnianych przez program instalacyjny plików.
PrepareToInstallNeedsRestart=Instalator wymaga ponownego uruchomienia komputera. Po restarcie komputera uruchom instalator ponownie, by dokończyć proces instalacji aplikacji [name].%n%nCzy chcesz teraz uruchomić komputer ponownie?

; *** "Installing" wizard page
WizardInstalling=Instalacja
InstallingLabel=Poczekaj, aż instalator zainstaluje aplikację [name] na komputerze.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=Zakończono instalację aplikacji [name]
FinishedLabelNoIcons=Instalator zakończył instalację aplikacji [name] na komputerze.
FinishedLabel=Instalator zakończył instalację aplikacji [name] na komputerze. Aplikacja może być uruchomiona poprzez użycie zainstalowanych skrótów.
ClickFinish=Kliknij przycisk Zakończ, aby zakończyć instalację.
FinishedRestartLabel=Aby zakończyć instalację aplikacji [name], instalator musi ponownie uruchomić komputer. Czy chcesz teraz uruchomić komputer ponownie?
FinishedRestartMessage=Aby zakończyć instalację aplikacji [name], instalator musi ponownie uruchomić komputer.%n%nCzy chcesz teraz uruchomić komputer ponownie?
ShowReadmeCheck=Tak, chcę przeczytać dodatkowe informacje
YesRadio=&Tak, uruchom ponownie teraz
NoRadio=&Nie, uruchomię ponownie później
; used for example as 'Run MyProg.exe'
RunEntryExec=Uruchom aplikację %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Wyświetl plik %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Instalator potrzebuje kolejnego archiwum
SelectDiskLabel2=Proszę włożyć dysk %1 i kliknąć przycisk OK.%n%nJeśli wymieniony poniżej folder nie określa położenia plików z tego dysku, proszę wprowadzić poprawną ścieżkę lub kliknąć przycisk Przeglądaj.
PathLabel=Ś&cieżka:
FileNotInDir2=Ścieżka "%2" nie zawiera pliku "%1". Proszę włożyć właściwy dysk lub wybrać inny folder.
SelectDirectoryLabel=Proszę określić lokalizację kolejnego archiwum instalatora.

; *** Installation phase messages
SetupAborted=Instalacja nie została zakończona.%n%nProszę rozwiązać problem i ponownie rozpocząć instalację.
AbortRetryIgnoreSelectAction=Wybierz operację
AbortRetryIgnoreRetry=Spróbuj &ponownie
AbortRetryIgnoreIgnore=Z&ignoruj błąd i kontynuuj
AbortRetryIgnoreCancel=Przerwij instalację
RetryCancelSelectAction=Wybierz operację
RetryCancelRetry=Spróbuj &ponownie
RetryCancelCancel=Anuluj

; *** Installation status messages
StatusClosingApplications=Zamykanie aplikacji...
StatusCreateDirs=Tworzenie folderów...
StatusExtractFiles=Dekompresja plików...
StatusDownloadFiles=Pobieranie plików...
StatusCreateIcons=Tworzenie skrótów aplikacji...
StatusCreateIniEntries=Tworzenie zapisów w plikach INI...
StatusCreateRegistryEntries=Tworzenie zapisów w rejestrze...
StatusRegisterFiles=Rejestracja plików...
StatusSavingUninstall=Zapisywanie informacji o dezinstalacji...
StatusRunProgram=Kończenie instalacji...
StatusRestartingApplications=Ponowne uruchamianie aplikacji...
StatusRollback=Cofanie zmian...

; *** Misc. errors
ErrorInternal2=Wewnętrzny błąd: %1
ErrorFunctionFailedNoCode=Błąd podczas wykonywania %1
ErrorFunctionFailed=Błąd podczas wykonywania %1; kod %2
ErrorFunctionFailedWithMessage=Błąd podczas wykonywania %1; kod %2.%n%3
ErrorExecutingProgram=Nie można uruchomić:%n%1

; *** Registry errors
ErrorRegOpenKey=Błąd podczas otwierania klucza rejestru:%n%1\%2
ErrorRegCreateKey=Błąd podczas tworzenia klucza rejestru:%n%1\%2
ErrorRegWriteKey=Błąd podczas zapisu do klucza rejestru:%n%1\%2

; *** INI errors
ErrorIniEntry=Błąd podczas tworzenia pozycji w pliku INI: "%1".

; *** File copying errors
FileAbortRetryIgnoreSkipNotRecommended=&Pomiń plik (niezalecane)
FileAbortRetryIgnoreIgnoreNotRecommended=Z&ignoruj błąd i kontynuuj (niezalecane)
SourceIsCorrupted=Plik źródłowy jest uszkodzony
SourceDoesntExist=Plik źródłowy "%1" nie istnieje
SourceVerificationFailed=Weryfikacja pliku źródłowego nie powiodła się: %1
VerificationSignatureDoesntExist=Plik podpisu "%1" nie istnieje
VerificationSignatureInvalid=Plik podpisu "%1" jest nieprawidłowy
VerificationKeyNotFound=Plik podpisu "%1" wykorzystuje nieznany klucz
VerificationFileNameIncorrect=Nazwa pliku jest nieprawidłowa
VerificationFileTagIncorrect=Etykieta pliku jest nieprawidłowa
VerificationFileSizeIncorrect=Rozmiar pliku jest nieprawidłowy
VerificationFileHashIncorrect=Suma kontrolna pliku jest nieprawidłowa
ExistingFileReadOnly2=Istniejący plik nie może zostać zastąpiony, gdyż jest oznaczony jako "Tylko do odczytu".
ExistingFileReadOnlyRetry=&Usuń atrybut "Tylko do odczytu" i spróbuj ponownie
ExistingFileReadOnlyKeepExisting=&Zachowaj istniejący plik
ErrorReadingExistingDest=Wystąpił błąd podczas próby odczytu istniejącego pliku:
FileExistsSelectAction=Wybierz czynność
FileExists2=Plik już istnieje.
FileExistsOverwriteExisting=&Nadpisz istniejący plik
FileExistsKeepExisting=&Zachowaj istniejący plik
FileExistsOverwriteOrKeepAll=&Wykonaj tę czynność dla kolejnych przypadków
ExistingFileNewerSelectAction=Wybierz czynność
ExistingFileNewer2=Istniejący plik jest nowszy niż ten, który instalator próbuje skopiować.
ExistingFileNewerOverwriteExisting=&Nadpisz istniejący plik
ExistingFileNewerKeepExisting=&Zachowaj istniejący plik (zalecane)
ExistingFileNewerOverwriteOrKeepAll=&Wykonaj tę czynność dla kolejnych przypadków
ErrorChangingAttr=Wystąpił błąd podczas próby zmiany atrybutów pliku docelowego:
ErrorCreatingTemp=Wystąpił błąd podczas próby utworzenia pliku w folderze docelowym:
ErrorReadingSource=Wystąpił błąd podczas próby odczytu pliku źródłowego:
ErrorCopying=Wystąpił błąd podczas próby kopiowania pliku:
ErrorDownloading=Wystąpił błąd podczas próby pobrania pliku:
ErrorExtracting=Wystąpił błąd podczas próby wypakowywania archiwum:
ErrorReplacingExistingFile=Wystąpił błąd podczas próby zamiany istniejącego pliku:
ErrorRestartReplace=Próba zastąpienia plików przy ponownym uruchomieniu komputera nie powiodła się.
ErrorRenamingTemp=Wystąpił błąd podczas próby zmiany nazwy pliku w folderze docelowym:
ErrorRegisterServer=Nie można zarejestrować DLL/OCX: %1
ErrorRegSvr32Failed=Funkcja RegSvr32 zakończyła się z kodem błędu %1
ErrorRegisterTypeLib=Nie mogę zarejestrować biblioteki typów: %1

; *** Uninstall display name markings
; used for example as 'My Program (32-bit)'
UninstallDisplayNameMark=%1 (%2)
; used for example as 'My Program (32-bit, All users)'
UninstallDisplayNameMarks=%1 (%2, %3)
UninstallDisplayNameMark32Bit=wersja 32-bitowa
UninstallDisplayNameMark64Bit=wersja 64-bitowa
UninstallDisplayNameMarkAllUsers=wszyscy użytkownicy
UninstallDisplayNameMarkCurrentUser=bieżący użytkownik

; *** Post-installation errors
ErrorOpeningReadme=Wystąpił błąd podczas próby otwarcia pliku z informacjami dodatkowymi.
ErrorRestartingComputer=Instalator nie mógł ponownie uruchomić tego komputera. Proszę wykonać tę czynność samodzielnie.

; *** Uninstaller messages
UninstallNotFound=Plik "%1" nie istnieje. Nie można przeprowadzić dezinstalacji.
UninstallOpenError=Plik "%1" nie mógł zostać otwarty. Nie można przeprowadzić dezinstalacji.
UninstallUnsupportedVer=Ta wersja programu dezinstalacyjnego nie rozpoznaje formatu logu dezinstalacji w pliku "%1". Nie można przeprowadzić dezinstalacji.
UninstallUnknownEntry=W logu dezinstalacji wystąpiła nieznana pozycja (%1)
ConfirmUninstall=Czy na pewno chcesz uruchomic kreatora deinstalacji %1?
UninstallOnlyOnWin64=Ta aplikacja może być odinstalowana tylko w 64-bitowej wersji systemu Windows.
OnlyAdminCanUninstall=Ta instalacja może być odinstalowana tylko przez użytkownika z uprawnieniami administratora.
UninstallStatusLabel=Poczekaj, aż aplikacja %1 zostanie usunięta z komputera.
UninstalledAll=Aplikacja %1 została usunięta z komputera.
UninstalledMost=Dezinstalacja aplikacji %1 zakończyła się.%n%nNiektóre elementy nie mogły zostać usunięte. Należy usunąć je samodzielnie.
UninstalledAndNeedsRestart=Komputer musi zostać ponownie uruchomiony, aby zakończyć proces dezinstalacji aplikacji %1.%n%nCzy chcesz teraz ponownie uruchomić komputer?
UninstallDataCorrupted=Plik "%1" jest uszkodzony. Nie można przeprowadzić dezinstalacji.

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Usunąć plik współdzielony?
ConfirmDeleteSharedFile2=System wskazuje, iż następujący plik nie jest już używany przez żaden program. Czy chcesz odinstalować ten plik współdzielony?%n%nJeśli inne programy nadal używają tego pliku, a zostanie on usunięty, mogą one przestać działać prawidłowo. W przypadku braku pewności, kliknij przycisk Nie. Pozostawienie tego pliku w systemie nie spowoduje żadnych szkód.
SharedFileNameLabel=Nazwa pliku:
SharedFileLocationLabel=Położenie:
WizardUninstalling=Stan dezinstalacji
StatusUninstalling=Dezinstalacja aplikacji %1...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=Instalacja aplikacji %1.
ShutdownBlockReasonUninstallingApp=Dezinstalacja aplikacji %1.

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 (wersja %2)
AdditionalIcons=Dodatkowe skróty:
CreateDesktopIcon=Utwórz skrót na &pulpicie
CreateQuickLaunchIcon=Utwórz skrót na pasku &szybkiego uruchamiania
ProgramOnTheWeb=Strona internetowa aplikacji %1
UninstallProgram=Dezinstalacja aplikacji %1
LaunchProgram=Uruchom aplikację %1
AssocFileExtension=&Przypisz aplikację %1 do rozszerzenia pliku %2
AssocingFileExtension=Przypisywanie aplikacji %1 do rozszerzenia pliku %2...
AutoStartProgramGroupDescription=Autostart:
AutoStartProgram=Automatycznie uruchamiaj aplikację %1
AddonHostProgramNotFound=Aplikacja %1 nie została znaleziona we wskazanym przez Ciebie folderze.%n%nCzy pomimo tego chcesz kontynuować?

; VCMI Custom Messages
SelectSetupInstallModeTitle=Wybierz tryb instalacji
SelectSetupInstallModeDesc=VCMI moze zostac zainstalowane dla wszystkich uzytkowników lub tylko dla Ciebie.
SelectSetupInstallModeSubTitle=Wybierz preferowany tryb instalacji:
InstallForAllUsers=Zainstaluj dla wszystkich uzytkowników
InstallForAllUsers1=Wymagane uprawnienia administratora
InstallForMeOnly=Zainstaluj tylko dla mnie
InstallForMeOnly1=Podczas pierwszego uruchomienia gry pojawi sie komunikat zapory
InstallForMeOnly2=Gry LAN nie beda dzialac, jesli regula zapory nie zostanie zaakceptowana
SystemIntegration=Integracja z systemem
CreateDesktopShortcuts=Utwórz skróty na pulpicie
CreateStartMenuShortcuts=Utwórz skróty w menu Start
AssociateH3MFiles=Powiaz pliki .h3m z Edytorem Map VCMI
AssociateVCMIMapFiles=Powiaz pliki .vmap i .vcmp z Edytorem Map VCMI
VCMISettings=Konfiguracja VCMI
AddFirewallRules=Dodaj reguly zapory dla VCMI
CopyH3Files=Automatycznie skopiuj wymagane pliki Heroes III do VCMI
RunVCMILauncherAfterInstall=Uruchom Launcher VCMI
ShortcutMapEditor=Edytor Map VCMI
ShortcutLauncher=Launcher VCMI
ShortcutWebPage=Strona internetowa VCMI
ShortcutDiscord=Discord VCMI
ShortcutLauncherComment=Uruchom Launcher VCMI
ShortcutMapEditorComment=Otwórz Edytor Map VCMI
ShortcutWebPageComment=Odwiedz oficjalna strone VCMI
ShortcutDiscordComment=Odwiedz oficjalny Discord VCMI
DeleteUserData=Usun dane uzytkownika
Uninstall=Odinstaluj
Warning=Ostrzezenie
VMAPDescription=Plik mapy VCMI
VCMPDescription=Plik kampanii VCMI
H3MDescription=Plik mapy Heroes 3