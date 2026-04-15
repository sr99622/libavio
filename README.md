# avio

Python library for processing media streams.


<h3>Linux</h3>

Use for both x86_64 and aarch64. Start the docker container and cd to $HOME, it won't work from /.

Docker configuration

```
sudo snap install docker
sudo groupadd docker
sudo usermod -aG docker $USER
sudo reboot now
```

Start the container. Select your architecture.

   * x86_64

```
   docker run -it quay.io/pypa/manylinux_2_28_x86_64 /bin/bash
```

   * aarch64

```
   docker run -it quay.io/pypa/manylinux_2_28_aarch64 /bin/bash
```

From inside the container, run the build

```
cd $HOME
git clone https://github.com/sr99622/libavio
libavio/scripts/linux/linux_build
```

The package installers will be found in the libavio/wheelhouse directory. For a quick test:

```
. $HOME/py313/bin/activate
pip install $HOME/libavio/wheelhouse/*313*
python
>>> import avio
>>> avio.__version__
>>> avio.Player("") 
```

---


<h3>Mac</h3>

to build package on mac, build out a virtual machine, then from terminal

```
git
```

This will start the XCode tools installation. Once complete:

```
cd $HOME
git clone https://github.com/sr99622/libavio
libavio/scripts/mac/mac_build
```

to test
```
. $HOME/py313/bin/activate
pip install $HOME/libavio/fixed_wheels/*313*
python
>>> import avio
>>> avio.__version__ 
>>> avio.Player("")
```

---

<h3>Windows</h3>

to build on windows a virtual machine is recommended, run from administrator prompt. Note that if a system python is installed, it may interfer with the linking process.

```
cd %HOMEPATH%
git clone https://github.com/sr99622/libavio
libavio\scripts\windows\windows_build
```

to test

```
cd %HOMEPATH%
py313\scripts\activate
for /f "delims=" %F in ('dir /b /s libavio\wheelhouse\*313*') do pip install "%F"
python
>>> import avio
>>> avio.__version__
>>> avio.Player("")
```

---

&nbsp;

Copyright (c) 2022, 2023, 2024, 2026  Stephen Rhodes

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

# SDL

 SDL 2.0 and newer are available under the [zlib license](https://www.zlib.net/zlib_license.html) :

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

# FFMPEG

Please refer to the [ffmpeg website](https://ffmpeg.org/legal.html) for detailed licensing information.