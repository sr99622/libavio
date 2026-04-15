@echo off

cd %HOMEPATH%
if not exist onvif-gui-win-libs\ (
    git clone https://github.com/sr99622/onvif-gui-win-libs
)

cd %HOMEPATH%\libavio
rmdir /q /s avio

if exist dist\ (
    del /q dist\*
)
if exist wheelhouse\ (
    del /q wheelhouse\*
)

call %HOMEPATH%\libavio\scripts\windows\python\install
call %HOMEPATH%\libavio\scripts\windows\env_variables
rem call %HOMEPATH%\libavio\scripts\windows\copy_libs

set list=(310 311 312 313 314)
rem set list=(313)
for %%v in %list% do (
    cd %HOMEPATH%
    %LOCALAPPDATA%\Programs\Python\Python%%v\python -m venv py%%v
    call py%%v\Scripts\activate
    python -m pip install --upgrade pip

    pip install build delvewheel
    cd libavio
    python -m build --sdist --wheel
    delvewheel repair dist\*cp%%v-cp%%v-*.whl --add-path %HOMEPATH%\onvif-gui-win-libs\ffmpeg\bin --add-path %HOMEPATH%\onvif-gui-win-libs\sdl\bin
    call deactivate
)
