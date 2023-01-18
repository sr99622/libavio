# libavio

Has dependency on FFMPEG and SDL2

Use standard cmake procedure

sample program has hard-coded file name


```bash
sudo apt install libavcodec-dev
sudp apt install libavdevice-dev
sudo apt install libsdl2-dev

get clone --recurse-submodules https://github.com/sr99622/libavio.git
cd libavio
mkdir build
cd build
cmake -DBUILD_SAMPLE=ON ..
cmake --build . --config Release

sudo cmake --install .
```

Install FindAvio.cmake in your project CMAKE_MODULE_PATH to link


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
