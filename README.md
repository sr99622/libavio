# avio

Python library for processing media streams

*Please Note*

This python module has dependencies on development libraries which
must be installed prior to installing.  This module is released as 
a source distribution and is compiled on the host.

**Debian or Ubuntu linux**, use the following command
to install the dependencies.

```
sudo apt install cmake libavcodec-dev libavdevice-dev libsdl2-dev
```

**Windows**, please use Anaconda.  The dependencies may
be installed using the following command under a conda prompt.  The 
Visual Studio C++ compiler must be installed as well.

```
conda install cmake ffmpeg sdl2
```

If successful, install avio python module

```
pip install avio
```
To uninstall
```
pip uninstall avio
```


Copyright (c) 2022, 2023  Stephen Rhodes

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
