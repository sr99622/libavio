pip install build
call %HOMEPATH%\libonvif\libavio\scripts\windows\env_variables
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
cd ..
