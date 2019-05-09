#!/usr/bin/env python

from distutils.core import setup, Extension
setup(name = "py_sg",
      version = "0.11",
      ext_modules=[
        Extension("py_sg", ["py_sg.c"])
        ],

      description = 'Python SCSI generic library',
      long_description =
'''This is a small Python extension which sends arbitrary commands to SCSI devices, via the Linux SCSI Generic driver, which provides the SG_IO ioctl for this purpose.

Basically, the module includes two methods, read and write, which
allow you to issue commands to SCSI devices and read and write
accompanying data. If an OS error occurs, the OSError exception will
be raised, while if a SCSI error occurs, the py_sg.SCSIError exception
will be raised.''',
      author = 'Dan Lenski',
      author_email = 'dlenski@gmail.com',
      url = 'http://tonquil.homeip.net/~dlenski/py_sg',
      license = 'GPLv3',
      classifiers = ['Topic :: System :: Hardware'],
)                                                                                                  
