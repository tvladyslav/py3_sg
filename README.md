### py_sg

This is a fork of Dan Lenski's SCSI library, rewritten to be compatible with **Python3**

### How to install

1. Clone this repo;
2. Enter the folder
3. Try to compile:
```
python3 setup.py build
```
4. If .so library is compiled, install:
```
sudo python3 -m pip install <path/to/setup.py>
```
5. You are done

### Changelog

Version 0.14 - Missing PY_SSIZE_T_CLEAN macro added, fix for Python 3.10 version

Version 0.13 - **breaking** change in API - split read function into 2. Get rid of deprecated function.

Version 0.12 - migrate to Python3, one deprecated function used

Version 0.11 - original


