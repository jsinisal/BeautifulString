/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#define PY_SSIZE_T_CLEAN
#include "beanalyzer.h"
#include "bstring.h"
#include "stremove.h"
#include "strfetch.h"
#include "strlearn.h"
#include "strmatch.h"
#include "strparse.h"
#include "strscan.h"
#include "strsearch.h"
#include "strvalidate.h"
#include <Python.h>
#include <stdlib.h>

static PyMethodDef BeautifulStringMethods[] =
{
    {"strfetch", strfetch, METH_VARARGS, "Fetch substrings using slice definitions."},
    {"strvalidate", (PyCFunction)strvalidate, METH_VARARGS | METH_KEYWORDS, "Validates the given pattern based string"},
    {"strscan", (PyCFunction)strscan, METH_VARARGS | METH_KEYWORDS,
     "Parse input_str using format_str. Supports named fields and return_type (list, tuple, dict, tuple_list)."},
    {"strmatch", (PyCFunction)strmatch, METH_VARARGS | METH_KEYWORDS, "Extracting fields from the matching string."},
    {"strsearch", (PyCFunction)strsearch, METH_VARARGS | METH_KEYWORDS, "Search for pattern match inside a string."},
    {"strparse", (PyCFunction)strparse, METH_VARARGS | METH_KEYWORDS, "Parsing a string, High-level scanf, constraints, type-safe, returns structured output"},
    {"strlearn", (PyCFunction)strlearn, METH_VARARGS | METH_KEYWORDS, "Infer a format from a list of strings. format=['list'|'c-style']"},
    {"stremove", (PyCFunction)stremove, METH_VARARGS | METH_KEYWORDS, "Remove or keep a set of characters from strings."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef BeautifulString =
{
  PyModuleDef_HEAD_INIT,
  "BeautifulString", 
  "A library with custom string-like objects.", -1, BeautifulStringMethods
};

PyMODINIT_FUNC PyInit_BeautifulString(void)
{
  PyObject *m;

  if (PyType_Ready(&BStringType) < 0)
    return NULL;
  if (PyType_Ready(&BeautifulAnalyzerType) < 0)
    return NULL;

  if (PyType_Ready(&BStringIter_Type) < 0)
    return NULL;

  m = PyModule_Create(&BeautifulString);
  if (m == NULL)
    return NULL;

  Py_INCREF(&BStringType);
  if (PyModule_AddObject(m, "BString", (PyObject *)&BStringType) < 0)
  {
    Py_DECREF(&BStringType);
    Py_DECREF(m);
    return NULL;
  }

  Py_INCREF(&BeautifulAnalyzerType);
  if (PyModule_AddObject(m, "BeautifulAnalyzer", (PyObject *)&BeautifulAnalyzerType) < 0)
  {
    Py_DECREF(&BStringType);
    Py_DECREF(&BeautifulAnalyzerType);
    Py_DECREF(m);
    return NULL;
  }
  return m;
}