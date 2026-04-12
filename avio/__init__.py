from importlib.metadata import PackageNotFoundError, version

from .avio import *
from .avio import __doc__

try:
    __version__ = version("avio")
except PackageNotFoundError:
    __version__ = "unknown"