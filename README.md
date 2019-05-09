### py_sg

Version 0.12

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

### Known issues - deprecated function

```
py_sg.c: In function ‘sg_read’:
py_sg.c:118:3: warning: ‘PyObject_AsWriteBuffer’ is deprecated [-Wdeprecated-declarations]
  118 |   if (PyObject_AsWriteBuffer(bufObj, (void*)&buf, &bufLen) < 0) {
      |   ^~
In file included from /usr/include/python3.7m/Python.h:147,
                 from py_sg.c:14:
/usr/include/python3.7m/abstract.h:500:17: note: declared here
  500 | PyAPI_FUNC(int) PyObject_AsWriteBuffer(PyObject *obj,
      |                 ^~~~~~~~~~~~~~~~~~~~~~
```
