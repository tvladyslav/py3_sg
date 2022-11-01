/**
 * Python SCSI generic library
 *
 * Copyright (C) 2008 by Daniel Lenski <lenski@umd.edu>
 * Time-stamp: <2008-09-19 00:18:51 dlenski>
 *
 * Migrated to Python3 by tvladyslav <ykp@protonmail.ch>
 *
 * Released under the terms of the
 * GNU General Public License version 3 or later
 */

#define PY_SSIZE_T_CLEAN

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

PyDoc_STRVAR(read_into_buf__doc__,
"sg_read_into_buf(sg_fd, cmd, bufObj[, timeout]) -> LENGTH\n\n"
"Issue a command and read a response.\n"
"Response is written to a provided writable buffer object.");

static PyObject *
sg_read_into_buf(PyObject *self, PyObject *args)
{
  int sg_fd;
  const int timeout=20000;
  uint8_t *cmd;
  int8_t *buf;
  Py_ssize_t cmdLen;
  Py_buffer bufObj;

  // parse and check arguments
  if (!PyArg_ParseTuple(args, "O&s#y*|i:read_into_buf", obj_to_fd, &sg_fd, &cmd, &cmdLen, &bufObj, &timeout))
    return NULL;

  buf = bufObj.buf;
  const Py_ssize_t bufLen = bufObj.len;

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

  PyObject *result = NULL;

  // handle errors
  if (r < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
  } else if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
    PyErr_SetObject(SCSIError,
                    Py_BuildValue("BBBs#", io.masked_status, io.host_status, io.driver_status, sense, io.sb_len_wr));
  } else {
    const int len = io.dxfer_len - io.resid;
    // data is in writable buffer, just return length
    result = PyLong_FromLong(len);
  }

  PyBuffer_Release(&bufObj);
  return result;
}

//////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(read_as_bin_str__doc__,
"read_as_bin_str(sg_fd, cmd, bufLen[, timeout]) -> STRING\n\n"
"Issue a command and read a response.\n"
"Response is returned as a binary string.");

static PyObject *
sg_read_as_bin_str(PyObject *self, PyObject *args)
{
  int sg_fd;
  const int timeout=20000;
  uint8_t *cmd;
  char *buf;
  Py_ssize_t cmdLen, bufLen;

  // parse and check arguments

  if (!PyArg_ParseTuple(args, "O&s#n|i:read_as_bin_str", obj_to_fd, &sg_fd, &cmd, &cmdLen, &bufLen, &timeout))
    return NULL;

  if (bufLen <= 0) {
    PyErr_SetString(PyExc_TypeError, "must provide an integer (> 0) specifying the buffer size");
    return NULL;
  }

  buf = (char*)calloc(bufLen, 1);

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

  PyObject* result = NULL;

  // handle errors
  if (r < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
  } else if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
    PyErr_SetObject(SCSIError,
                    Py_BuildValue("BBBs#", io.masked_status, io.host_status, io.driver_status, sense, io.sb_len_wr));
  } else {
    const int len = io.dxfer_len - io.resid;
    result = PyBytes_FromStringAndSize(buf, len);
}

  free(buf);
  return result;
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
  {"read_into_buf",  sg_read_into_buf,  METH_VARARGS, read_into_buf__doc__},
  {"read_as_bin_str",  sg_read_as_bin_str,  METH_VARARGS, read_as_bin_str__doc__},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef py3_sg_definition = {
    PyModuleDef_HEAD_INIT,
    "py3_sg",
    module__doc__,
    -1,
    SgMethods
};

PyMODINIT_FUNC
PyInit_py3_sg(void)
{
  // initialize module
  Py_Initialize();
  PyMODINIT_FUNC mod = PyModule_Create(&py3_sg_definition);
  if (!mod) return NULL;

  // SCSIError
  PyObject *doc = Py_BuildValue("{ss}", "__doc__", SCSIError__doc__);
  SCSIError = PyErr_NewException( "py3_sg.SCSIError", NULL, doc);

  PyModule_AddObject(mod, "SCSIError", SCSIError);
  return mod;
}
