/*
Copyright (c) 2023 Daniel Turecek <daniel@turecek.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "common.h"
#include "Python.h"
#include "structmember.h"
#include "filelog.h"
#include "fpdev.h"
#include "buffer.h"
#include "commonpython.h"


struct DeviceData
{
};

typedef struct {
    PyObject_HEAD
    FPDev* dev;
    FileLog* log;
} Device;

static int device_init(Device *self, PyObject *args, PyObject *kwds)
{
    self->dev = NULL;
    self->log = NULL;
    return 0;
}

static void device_dealloc(Device *self)
{
    if (self->dev){
        delete self->dev;
        self->dev = NULL;
    }

    if (self->log){
        delete self->log;
        self->log = NULL;
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* device_listDevices(Device *self, PyObject *args)
{
    (void)self;
    (void)args;
    FPDev fp;
    std::vector<FPDevInfo> devices = fp.listDevicesInfo();

    PyObject* list = PyList_New(devices.size());
    for (int i = 0; i < (int)devices.size(); i++){
        PyObject* devList = PyList_New(2);
        PyList_SetItem(devList, 0, Py_BuildValue("s", devices[i].devSerial.c_str()));
        PyList_SetItem(devList, 1, Py_BuildValue("s", devices[i].deviceID.c_str()));
        PyList_SetItem(list, i, devList);
    }

    return list;
}

static PyObject* device_open(Device *self, PyObject *args)
{
    const char* firmware;
    const char* logfile;
    const char* serial;
    if (!PyArg_ParseTuple(args, "sss", &serial, &firmware, &logfile))
        return NULL;

    if (self->dev) delete self->dev;
    if (self->log) {delete self->log; self->log = NULL;}

    if (logfile){
        self->log = new FileLog(logfile, true, false);
        self->log->setLogLevel(LOG_DBG);
    }

    self->dev = new FPDev();
    int rc = self->dev->open(serial, firmware);
    return Py_BuildValue("i", rc);
}

static PyObject* device_close(Device *self, PyObject *args)
{
    int rc = 0;
    if (self->dev){
        rc = self->dev->close();
        delete self->dev;
        self->dev = NULL;
    }

    if (self->log){
        delete self->log;
        self->log = NULL;
    }

    return Py_BuildValue("i", rc);
}


// int setWireIn(u32 address, u32 value, bool sendNow=true);
static PyObject* device_setWireIn(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    long address, value;
    int sendNow;
    if (!PyArg_ParseTuple(args, "lli", &address, &value, &sendNow))
        return NULL;

    int rc = self->dev->setWireIn(address, value, sendNow);
    return Py_BuildValue("i", rc);
}

// i64 getWireOut(u32 address, bool refreshWireOuts=true);
static PyObject* device_getWireOut(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    unsigned address;
    int refresh;
    if (!PyArg_ParseTuple(args, "Ii", &address, &refresh))
        return NULL;

    i64 rc = self->dev->getWireOut(address, refresh);
    //return Py_BuildValue("l", rc);
    return PyLong_FromLongLong(rc);
}

// int writeRegister(u32 address, u32 value);
static PyObject* device_writeRegister(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    unsigned address, value;
    if (!PyArg_ParseTuple(args, "II", &address, &value))
        return NULL;

    int rc = self->dev->writeRegister(address, value);
    return Py_BuildValue("i", rc);
}

// i64 readRegister(u32 address);
static PyObject* device_readRegister(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    unsigned address;
    if (!PyArg_ParseTuple(args, "I", &address))
        return NULL;

    i64 rc = self->dev->readRegister(address);
    return PyLong_FromLongLong(rc);
    //return Py_BuildValue("l", rc);
}


// int writePipe(u32 address, byte* data, size_t size, size_t blockSize=1024);
static PyObject* device_writePipe(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    unsigned address;
    int blockSize;
    PyObject* data;
    if (!PyArg_ParseTuple(args, "IO!i", &address, &PyList_Type, &data, &blockSize))
        return NULL;

    Py_ssize_t count = PyList_Size(data);
    if (count < 0){
        PyErr_SetString(PyExc_IOError, "Invalid data.");
        return NULL;
    }

    Buffer<byte> buff(count);
    for (int i = 0; i < count; i++)
        buff[i] = static_cast<byte>(PyInt_AsLong(PyList_GetItem(data, i)));

    int rc = self->dev->writePipe(address, buff.data(), (size_t)count, blockSize);
    return Py_BuildValue("i", rc);
}

// i64 readPipe(u32 address, byte* data, size_t size, size_t blockSize=1024);
static PyObject* device_readPipe(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    unsigned address;
    int blockSize;
    PyObject* data;
    if (!PyArg_ParseTuple(args, "IO!i", &address, &PyList_Type, &data, &blockSize))
        return NULL;

    Py_ssize_t count = PyList_Size(data);
    if (count < 0){
        PyErr_SetString(PyExc_IOError, "Invalid data.");
        return NULL;
    }

    Buffer<byte> buff(count);
    buff.zero();
    size_t size = count;

    int rc = self->dev->readPipe(address, buff.data(), size, blockSize);

    for (int i = 0; i < count; i++)
        PyList_SetItem(data, i, PyInt_FromLong(buff[i]));

    return PyLong_FromLongLong(rc);
}

// int setTimeout(double timeout);
static PyObject* device_setTimeout(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    double timeout;
    if (!PyArg_ParseTuple(args, "d", &timeout))
        return NULL;

    int rc = self->dev->setTimeout(timeout);
    return Py_BuildValue("i", rc);
}

static PyObject* device_setDeviceID(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    const char* devid;
    if (!PyArg_ParseTuple(args, "s", &devid))
        return NULL;

    self->dev->setDeviceID(devid);
    return Py_BuildValue("i", 0);
}

static PyObject* device_getDeviceID(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    std::string devid = self->dev->getDeviceID();
    return Py_BuildValue("s", devid.c_str());
}



static PyObject* device_log(Device *self, PyObject *args)
{
    if (!self->dev){
        PyErr_SetString(PyExc_IOError, "Device not opened.");
        return NULL;
    }

    int loglevel;
    int notime;
    const char* text;
    if (!PyArg_ParseTuple(args, "isi", &loglevel, &text, &notime))
        return NULL;

    if (notime)
        self->log->logNoTime((LogLevel)loglevel, text);
    else
        self->log->log((LogLevel)loglevel, text);

    return Py_BuildValue("i", 0);
}


static PyMemberDef device_members[] =
{
   { NULL }
};

static PyMethodDef device_methods[] =
{
   { "list_devices",   (PyCFunction) device_listDevices, METH_VARARGS, "List connected FrontPanel devices" },
   { "open",          (PyCFunction) device_open, METH_VARARGS, "Open device(serial,firmware,logfile)" },
   { "close",         (PyCFunction) device_close, METH_VARARGS, "close()" },
   { "set_wire_in",     (PyCFunction) device_setWireIn, METH_VARARGS, "set_wire_in(address, value, sendNow)" },
   { "get_wire_out",   (PyCFunction) device_getWireOut, METH_VARARGS, "get_wire_out(address, refreshWires)" },
   { "write_register", (PyCFunction) device_writeRegister, METH_VARARGS, "write_register(address, value)" },
   { "read_register",  (PyCFunction) device_readRegister, METH_VARARGS, "read_register(address)" },
   { "write_pipe",      (PyCFunction) device_writePipe, METH_VARARGS, "write_pipe(address,[data], blockSize=1024)" },
   { "read_pipe",       (PyCFunction) device_readPipe, METH_VARARGS, "read_pipe(address,[data], blockSize=1024)" },
   { "set_timeout",       (PyCFunction) device_setTimeout, METH_VARARGS, "set_timeout(timeout)" },
   { "set_device_id", (PyCFunction) device_setDeviceID, METH_VARARGS, "set_deviceID(deviceID)" },
   { "get_device_id", (PyCFunction) device_getDeviceID, METH_VARARGS, "get_deviceID()" },
   { "log",           (PyCFunction) device_log, METH_VARARGS, "log(loglevel, text, notime)" },
   { NULL }
};

PyTypeObject DeviceType =
{
   PyVarObject_HEAD_INIT(NULL, 0)
   //PyObject_HEAD_INIT(NULL)
   //0,                         /* ob_size */
   "Device",               /* tp_name */
   sizeof(Device),         /* tp_basicsize */
   0,                         /* tp_itemsize */
   (destructor)device_dealloc, /* tp_dealloc */
   0,                         /* tp_print */
   0,                         /* tp_getattr */
   0,                         /* tp_setattr */
   0,                         /* tp_compare */
   0,                         /* tp_repr */
   0,                         /* tp_as_number */
   0,                         /* tp_as_sequence */
   0,                         /* tp_as_mapping */
   0,                         /* tp_hash */
   0,                         /* tp_call */
   0,                         /* tp_str */
   0,                         /* tp_getattro */
   0,                         /* tp_setattro */
   0,                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
   "Device object",        /* tp_doc */
   0,                         /* tp_traverse */
   0,                         /* tp_clear */
   0,                         /* tp_richcompare */
   0,                         /* tp_weaklistoffset */
   0,                         /* tp_iter */
   0,                         /* tp_iternext */
   device_methods,         /* tp_methods */
   device_members,         /* tp_members */
   0,                         /* tp_getset */
   0,                         /* tp_base */
   0,                         /* tp_dict */
   0,                         /* tp_descr_get */
   0,                         /* tp_descr_set */
   0,                         /* tp_dictoffset */
   (initproc)device_init,  /* tp_init */
   0,                         /* tp_alloc */
   0,                         /* tp_new */
};


//################################################################################
//                      INIT MODULE
//################################################################################

static PyMethodDef module_methods[] = {
    {"list_devices", (PyCFunction)device_listDevices, METH_VARARGS, "list_devices()"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "py_fp",           /* m_name */
    "Interface to Front Panel device",    /* m_doc */
    -1,                  /* m_size */
    module_methods,    /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};

PyMODINIT_FUNC PyInit_py_fp(void)
{
    PyObject* m = PyModule_Create(&moduledef);
    if (m == NULL)
        return NULL;

    DeviceType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&DeviceType) < 0)
        return m;

    Py_INCREF(&DeviceType);
    PyModule_AddObject(m, "FPDevice", (PyObject*)&DeviceType);

    return m;
}

PyMODINIT_FUNC PyInit_py_fp_linux(void)
{
    moduledef.m_name = "py_device_linux";
    return PyInit_py_fp();
}
PyMODINIT_FUNC PyInit_py_fp_mac(void)
{
    moduledef.m_name = "py_device_mac";
    return PyInit_py_fp();
}


