#include <stdlib.h>
#include "Python.h"
#include "frameobject.h"
#include "serial.h"
#include "dump.h"

#define MODULE_DOC PyDoc_STR("C extension for fast function tracing.")

static int
trace_func(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg)
{
  if (!should_trace_module(frame)) {
      return NULL;
  }

  switch (what) {
  case PyTrace_CALL:
    handle_call(frame);
    break;
  case PyTrace_RETURN:
    handle_return(frame, arg);
    break;
  case PyTrace_EXCEPTION: // setprofile translates exceptions to calls
    handle_exception(frame, arg);
  }
  return NULL;
}

static PyListObject *filter_modules = NULL;

int should_trace_module(PyFrameObject *frame) {
  int i;
  char *filter, *module;;

  if (NULL == filter_modules) {
    return 1;
  }

  module = PyString_AsString(frame->f_code->co_filename);
  for (i = 0; i < PyList_Size(filter_modules); i++) {
    filter = PyString_AsString(PyList_GetItem(filter_modules, i));
    return strncmp(module, filter, strlen(filter)) == 0;
  }
}

static PyObject*
start(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "|O!", &PyList_Type, &filter_modules)){
    return;
  }
  if (NULL != filter_modules) {
    Py_INCREF(filter_modules);
  }
  PyEval_SetTrace((Py_tracefunc) trace_func,
		  (PyObject*) self);
  dump_main_in_thread();
  return Py_BuildValue("");
}

static PyObject*
stop(PyObject *self, PyObject *args)
{
  dump_stop();
  PyEval_SetTrace(NULL, NULL);
  return Py_BuildValue("");
}

static PyMethodDef
methods[] = {
  {"start", (PyCFunction) start, METH_VARARGS, PyDoc_STR("Start the tracer")},
  {"stop", (PyCFunction) stop, METH_VARARGS, PyDoc_STR("Stop the tracer")},
  {NULL}
};

void
inittracer(void)
{
  init_serialize();
  Py_InitModule3("pytrace.tracer", methods, MODULE_DOC);
}
