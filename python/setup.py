import os
import shutil

#from distutils.core import setup, Extension
from setuptools import setup, Extension

upstream_dir = os.path.join(os.path.dirname(__file__),"uWebSockets", "src")
if not os.path.exists(upstream_dir):
    shutil.copytree(os.path.join(os.path.dirname(__file__), "..", "uWebSockets"), os.path.join(os.path.dirname(__file__), "uWebSockets"))

upstream_src = [os.path.join(upstream_dir, f) for f in os.listdir(upstream_dir) if f.split(".")[-1] in ("cpp", "cxx", "c")]
upstream_headers = [os.path.join(upstream_dir, f) for f in os.listdir(upstream_dir) if f.split(".")[-1] in ("hpp", "hxx", "h")]

uWebSockets = Extension("uWebSockets", 
    sources=["Bindings.cpp"] + upstream_src,
    include_dirs=[upstream_dir],
    libraries=["ssl"],
    define_macros=[("UWS_THREADSAFE", 1)],
    extra_compile_args=["-std=c++11"]
)

setup(name="uWebSockets",
    version="0.14.5a4",
    description="Python Bindings for the uWebSockets library",
    url="https://github.com/elementengineering/uWebSockets-bindings/",
    author="Sam Moore",
    author_email="smoore@elementengineering.com.au",
    license="MIT",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Topic :: Software Development",
        "License :: OSI Approved :: MIT License",

        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7"
    ], 
    keywords="websockets development",
    ext_modules =[uWebSockets],
)
