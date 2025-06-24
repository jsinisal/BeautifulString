#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include "strscan.h"
#include "strfetch.h"
#include "strmatch.h"


static PyMethodDef BeautifulStringMethods[] = {
    {"strfetch", strfetch, METH_VARARGS, "Fetch substrings using slice definitions."},
    {"strscan", (PyCFunction)strscan, METH_VARARGS | METH_KEYWORDS,"Parse input_str using format_str. Supports named fields and return_type (list, tuple, dict, tuple_list)."},
    {"strmatch", strmatch, METH_VARARGS, "Extracting fields from the matching string."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef BeautifulString = {
    PyModuleDef_HEAD_INIT,
    "BeautifulString",   // Module name
    NULL,
    -1,
    BeautifulStringMethods
};

PyMODINIT_FUNC PyInit_BeautifulString(void) {
    return PyModule_Create(&BeautifulString);
}
