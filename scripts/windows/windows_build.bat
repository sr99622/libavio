@echo off

if not exist "%ALLUSERSPROFILE%\chocolatey\bin\" (
    @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
)
if not exist "%ProgramFiles%\Git\" (
    choco install -y git
)
if not exist "%ProgramFiles%\CMake\" (
    choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System' --apply-install-arguments-to-dependencies
)
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\" (
    winget install Microsoft.VisualStudio.2022.BuildTools --silent --override "--wait --quiet --add ProductLang En-us --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended"
)

cd %HOMEPATH%
if not exist onvif-gui-win-libs\ (
    git clone https://github.com/sr99622/onvif-gui-win-libs
)
cd %HOMEPATH%\onvif-gui-win-libs
rem git pull

cd %HOMEPATH%\libavio

if exist dist\ (
    del /q dist\*
)

call %HOMEPATH%\libavio\scripts\windows\python\install
call %HOMEPATH%\libavio\scripts\windows\env_variables
call %HOMEPATH%\libavio\scripts\windows\copy_libs

rem set list=(310 311 312 313 314)
set list=(313)
for %%v in %list% do (
    cd %HOMEPATH%
    %LOCALAPPDATA%\Programs\Python\Python%%v\python -m venv py%%v
    call py%%v\Scripts\activate
    python.exe -m pip install --upgrade pip
    pip uninstall -y avio
    call %HOMEPATH%\libavio\scripts\windows\build_pkgs

    pip install build
    if not exist dist/ (
        mkdir dist
    )
    cd libavio
    rmdir /q /s build
    set SOURCE_DIR=%CD%
    python -m build
    for /f %%F in ('dir /b /a-d dist\*whl') do (
        pip install dist\%%F
    )
    move dist\* ..\dist

    call deactivate
)
