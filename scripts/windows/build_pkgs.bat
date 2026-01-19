set FFMPEG_INSTALL_DIR=%CD%\onvif-gui-win-libs\ffmpeg
set SDL2_INSTALL_DIR=%CD%\onvif-gui-win-libs\sdl
copy onvif-gui-win-libs\ffmpeg\bin\* avio
copy onvif-gui-win-libs\sdl\bin\* avio
pip install build
rmdir /q /s build
set SOURCE_DIR=%CD%
python -m build
for /f %%F in ('dir /b /a-d dist\*whl') do (
    pip install dist\%%F
)
