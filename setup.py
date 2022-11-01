#!/usr/bin/env python3

from distutils.core import setup, Extension
setup(
      ext_modules=[
        Extension("py3_sg", ["py3_sg.c"])
      ],
)
