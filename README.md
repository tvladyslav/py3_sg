# py3_sg

## Description

This is a small Python extension which sends arbitrary commands to SCSI devices,
via the Linux SCSI Generic driver, which provides the SG_IO ioctl for this purpose.

Basically, the module includes two methods, read and write, which
allow you to issue commands to SCSI devices and read and write
accompanying data. If an OS error occurs, the `OSError` exception will
be raised, while if a SCSI error occurs, the `py_sg.SCSIError` exception
will be raised.

## PyPI install

```bash
sudo python3 -m pip install py3_sg
```

## Manual install

1. Install dependencies:

    ```bash
    sudo apt install python3-dev
    ```

2. Clone this repo
3. Enter the folder
4. Try to compile:

    ```bash
    python3 setup.py build
    ```

5. If .so library is compiled, install:

    ```bash
    sudo python3 -m pip install <path/to/setup.py>
    ```

6. You are done

## Precompiled package

[https://pypi.org/project/py3-sg/](https://pypi.org/project/py3-sg/)
