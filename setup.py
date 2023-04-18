import os
import subprocess
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE=Release",
            f"-DWITHOUT_LIBS=1",
            f"-Wno-dev",
        ]
        build_args = []

        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            if hasattr(self, "parallel") and self.parallel:
                build_args += [f"-j{self.parallel}"]

        build_temp = os.path.join(self.build_temp, ext.name)
        if not os.path.exists(build_temp):
            os.makedirs(build_temp)

        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(["cmake", "--build", "."] + build_args, cwd=build_temp)


setup(
    name="avio",
    version="2.0.0",
    author="Stephen Rhodes",
    author_email="sr99622@gmail.com",
    description="A python media processing module",
    long_description="",
    ext_modules=[CMakeExtension("avio")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=">=3.6",
#    packages=['avio'],
#    package_data={'avio' : ['libavcodec.so.59.37.100',
#                            'libavdevice.so.59.7.100',
#                            'libavfilter.so.8.44.100',
#                            'libavformat.so.59.27.100',
#                            'libavutil.so.57.28.100',
#                            'libpostproc.so.56.6.100',
#                            'libswresample.so.4.7.100',
#                            'libswscale.so.6.7.100',
#                            'libSDL2-2.0.so.0.2400.0']},
#    include_package_data = True,
)