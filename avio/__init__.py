from importlib.metadata import PackageNotFoundError, version
import sys

if sys.platform == "win32":
    from avio import *
    from avio import __doc__
else:
    from .avio import *
    from .avio import __doc__

try:
    __version__ = version("avio")
except PackageNotFoundError:
    __version__ = "unknown"