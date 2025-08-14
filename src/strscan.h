/*

This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com

*/


#ifndef STRSCAN_H
#define STRSCAN_H

#include <Python.h>

#define MAX_FIELDS 32

PyObject* strscan(PyObject *self, PyObject *args, PyObject *kwargs);

#endif
