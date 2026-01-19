set FFMPEG_INSTALL_DIR=%CD%\onvif-gui-win-libs\ffmpeg
set SDL2_INSTALL_DIR=%CD%\onvif-gui-win-libs\sdl
copy onvif-gui-win-libs\ffmpeg\bin\* avio
copy onvif-gui-win-libs\sdl\bin\* avio

if exist build\ (
    rmdir /s /q build
)
if exist avio.egg-info\ (
    rmdir /s /q avio.egg-info
)
pip install -v .
