/**
 * Python SCSI generic library
 * version 0.1
 *
 * Copyright (C) 2008 by Daniel Lenski <lenski@umd.edu>
 * Time-stamp: <2008-09-19 00:18:51 dlenski>
 *
 * Released under the terms of the
 * GNU General Public License version 2 or later
 */

#include <Python.h>

#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>

static PyObject *SCSIError;
PyDoc_STRVAR(SCSIError__doc__,
"SCSI operation failed.\n\n"
"The accompanying value is a 4-tuple containing the masked_status,\n"
"driver_status, host_status, and sense buffer fields:\n\n"
"except SCSIError, e:\n"
"  masked_status, driver_status, host_status, sense = e.args");

static int
obj_to_fd(PyObject *object, int *target)
{
    int fd = PyObject_AsFileDescriptor(object);

    if (fd < 0)
      return 0;
    *target = fd;
    return 1;
}

//////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(write__doc__,
"write(sg_fd, cmd[, buf, timeout])\n\n"
"Issue a command and write data.  Returns nothing.");

static PyObject *
sg_write(PyObject *self, PyObject *args)
{
  int sg_fd, timeout=20000;
  uint8_t *cmd, *buf=NULL;
  Py_ssize_t cmdLen, bufLen=0;

  // parse and check arguments

  if (!PyArg_ParseTuple(args, "O&s#|s#i:write", obj_to_fd, &sg_fd, &cmd, &cmdLen, &buf, &bufLen, &timeout))
    return NULL;

  // submit SG_IO ioctl

  sg_io_hdr_t io;
  uint8_t sense[32];
  int r;

  memset(&io, 0, sizeof(io));

  io.interface_id = 'S';
  io.cmd_len = cmdLen;
  /* io.iovec_count = 0; */  /* memset takes care of this */
  io.mx_sb_len = sizeof(sense);
  io.dxfer_direction = SG_DXFER_TO_DEV;
  io.dxfer_len = bufLen;
  io.dxferp = buf;
  io.cmdp = cmd;
  io.sbp = sense;
  io.timeout = timeout;   /* in millisecs */
  /* io.flags = 0; */     /* take defaults: indirect IO, etc */
  /* io.pack_id = 0; */
  /* io.usr_ptr = NULL; */  

  r = ioctl(sg_fd, SG_IO, &io);

  // handle errors

  if (r < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  } else if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
    PyErr_SetObject(SCSIError,
                    Py_BuildValue("BBBs#", io.masked_status, io.host_status, io.driver_status, sense, io.sb_len_wr));
    return NULL;
  }
  
  Py_RETURN_NONE;
}

//////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(read__doc__,
"read(sg_fd, cmd, bufObj[, timeout]) -> LENGTH\n"
"read(sg_fd, cmd, bufLen[, timeout]) -> STRING\n\n"
"Issue a command and read a response.  Response is either\n"
"returned as a binary string, or written to a provided\n"
"writable buffer object.");

static PyObject *
sg_read(PyObject *self, PyObject *args)
{
  int sg_fd, timeout=20000, newbuf=0;
  uint8_t *cmd, *buf;
  Py_ssize_t cmdLen, bufLen;
  PyObject *bufObj;

  // parse and check arguments
  
  if (!PyArg_ParseTuple(args, "O&s#O|i:read", obj_to_fd, &sg_fd, &cmd, &cmdLen, &bufObj, &timeout))
    return NULL;
  if (PyObject_AsWriteBuffer(bufObj, (void*)&buf, &bufLen) < 0) {
    bufLen = PyInt_AsLong(bufObj);
    if (bufLen <= 0) {
      PyErr_SetString(PyExc_TypeError,
                      "must provide a writable buffer object, or an integer (> 0) specifying the buffer size");
      return NULL;
    }
    PyErr_Clear();
    bufObj = PyString_FromStringAndSize(NULL, bufLen); // new blank string
    if (!bufObj) return NULL;
    buf = (unsigned char*)PyString_AS_STRING(bufObj);
    newbuf = 1;
  }

  // submit SG_IO ioctl

  sg_io_hdr_t io;
  uint8_t sense[32];
  int r;

  memset(&io, 0, sizeof(io));

  io.interface_id = 'S';
  io.cmd_len = cmdLen;
  /* io.iovec_count = 0; */  /* memset takes care of this */
  io.mx_sb_len = sizeof(sense);
  io.dxfer_direction = SG_DXFER_FROM_DEV;
  io.dxfer_len = bufLen;
  io.dxferp = buf;
  io.cmdp = cmd;
  io.sbp = sense;
  io.timeout = timeout;   /* in millisecs */
  /* io.flags = 0; */     /* take defaults: indirect IO, etc */
  /* io.pack_id = 0; */
  /* io.usr_ptr = NULL; */  

  r = ioctl(sg_fd, SG_IO, &io);

  // handle errors
  
  if (r < 0) {
    if (newbuf) { Py_DECREF(bufObj); }
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  } else if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
    if (newbuf) { Py_DECREF(bufObj); }
    PyErr_SetObject(SCSIError,
                    Py_BuildValue("BBBs#", io.masked_status, io.host_status, io.driver_status, sense, io.sb_len_wr));
    return NULL;
  }

  int len = io.dxfer_len - io.resid;
  if (newbuf) {
    // trim to size of data actually received
    if (_PyString_Resize(&bufObj, len) < 0) return NULL;
    return bufObj;
  } else {
    // data is in writable buffer, just return length
    return PyInt_FromLong(len);
  }
}

//////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(module__doc__,
"This module issues commands to SCSI devices under Linux,\n"
"via the SG_IO ioctl of the scsi_generic driver.\n"
"\n"
"The device (e.g. /dev/sg0, /dev/sda, etc.) must first be\n"
"opened, and the file descriptor or filehandle obtained\n"
"is passed to the methods of this module.");

static PyMethodDef SgMethods[] = {
  {"write", sg_write, METH_VARARGS, write__doc__},
  {"read",  sg_read,  METH_VARARGS, read__doc__},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initpy_sg(void)
{
  // initialize module
  PyObject *mod = Py_InitModule3("py_sg", SgMethods, module__doc__);
  if (!mod) return;
  
  // SCSIError
  PyObject *doc = Py_BuildValue("{ss}", "__doc__", SCSIError__doc__);
  SCSIError = PyErr_NewException( "py_sg.SCSIError", NULL, doc);

  PyModule_AddObject(mod, "SCSIError", SCSIError);
}
