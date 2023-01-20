# libavio

Has dependency on FFMPEG, Python and SDL2
GUI sample has dependency on Qt5


```bash
sudo apt install libavcodec-dev
sudp apt install libavdevice-dev
sudo apt install libsdl2-dev
sudo apt install qtbase5-dev

get clone --recurse-submodules https://github.com/sr99622/libavio.git
cd libavio
mkdir build
cd build
cmake -DBUILD_SAMPLES=ON ..
cmake --build . --config Release
sudo cmake --install .
```

Install FindAvio.cmake in your project CMAKE_MODULE_PATH to link

To install avio python module
```
python3 setup.py install
```
To uninstall
```
pip uninstall avio
```


Copyright (c) 2022  Stephen Rhodes

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
