//
// Created by Juha on 25.6.2025.
//

#ifndef STRSEARCH_INTERNAL_H
#define STRSEARCH_INTERNAL_H

#include <Python.h>
#include <stdio.h>  // For debug logging

#ifdef __cplusplus
extern "C" {
#endif

    // Main interface for Python strsearch
    PyObject* strsearch(PyObject *self, PyObject *args, PyObject *kwargs);

    // Internal matcher used by strmatch and strsearch
    PyObject* strmatch_internal(
        PyObject *self,
        const char *input_str,
        const char *pattern_str,
        const char *return_type,
        int *consumed_chars,
        int partial_mode
    );

#ifdef __cplusplus
}
#endif

#endif // STRSEARCH_INTERNAL_H