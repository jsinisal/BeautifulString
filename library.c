// Part of BeautifulString python module

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdlib.h>
#include "strscan.h"
#include "strfetch.h"
#include "strmatch.h"
#include "strsearch.h"
#include "strvalidate.h"
#include "strparse.h"
#include "bstring.h"

static PyMethodDef BeautifulStringMethods[] = {
    {"strfetch", strfetch, METH_VARARGS, "Fetch substrings using slice definitions."},
    {"strvalidate", (PyCFunction)strvalidate,METH_VARARGS | METH_KEYWORDS, "Validates the given pattern based string"},
    {"strscan", (PyCFunction)strscan, METH_VARARGS | METH_KEYWORDS,"Parse input_str using format_str. Supports named fields and return_type (list, tuple, dict, tuple_list)."},
    {"strmatch", (PyCFunction)strmatch, METH_VARARGS | METH_KEYWORDS, "Extracting fields from the matching string."},
    {"strsearch", (PyCFunction)strsearch, METH_VARARGS | METH_KEYWORDS, "Search for pattern match inside a string."},
    {"strparse", (PyCFunction)strparse, METH_VARARGS | METH_KEYWORDS, "Parsing a string, High-level scanf, constraints, type-safe, returns structured output"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef BeautifulString = {
    PyModuleDef_HEAD_INIT,
    "BeautifulString",   // Module name
    "A library with custom string-like objects.",
    -1,
    BeautifulStringMethods
};


PyMODINIT_FUNC
PyInit_BeautifulString(void) {
    PyObject *m;

    // Finalize the BString type
    if (PyType_Ready(&BStringType) < 0)
        return NULL;

    // Finalize the BStringIter type
    if (PyType_Ready(&BStringIter_Type) < 0)
        return NULL;

    // Create the module
    m = PyModule_Create(&BeautifulString);
    if (m == NULL)
        return NULL;

    // Add the BString type to the module
    Py_INCREF(&BStringType);
    if (PyModule_AddObject(m, "BString", (PyObject *)&BStringType) < 0) {
        Py_DECREF(&BStringType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

/*
PyMODINIT_FUNC PyInit_BeautifulString(void) {
    return PyModule_Create(&BeautifulString);
}
*/
