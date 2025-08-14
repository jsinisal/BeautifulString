/*

This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com

*/


#ifndef STRSEARCH_INTERNAL_H
#define STRSEARCH_INTERNAL_H

#include <Python.h>


PyObject* strsearch(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* strmatch_internal(PyObject *self, const char *input_str, const char *pattern_str, const char *return_type, int *consumed_chars, int partial_mode);

#endif // STRSEARCH_INTERNAL_H