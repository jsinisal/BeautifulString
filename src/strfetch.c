//
// Created by Juha on 23.6.2025.
//
// Part of BeautifulString python module

#include <Python.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "strfetch.h"

static void build_slice_expr(const char *raw, char *out, size_t out_size) {
    char start[32] = "None", end[32] = "None", step[32] = "";
    const char *first = raw;
    const char *second = strchr(first, ':');
    const char *third = second ? strchr(second + 1, ':') : NULL;

    if (second) {
        if (third) {
            // Three parts
            size_t len1 = second - first;
            size_t len2 = third - second - 1;
            strncpy(start, first, len1); start[len1] = '\0';
            strncpy(end, second + 1, len2); end[len2] = '\0';
            strncpy(step, third + 1, 31); step[31] = '\0';
        } else {
            // Two parts
            size_t len1 = second - first;
            strncpy(start, first, len1); start[len1] = '\0';
            strncpy(end, second + 1, 31); end[31] = '\0';
        }
    } else {
        // Only one part (shouldn't happen in valid slice)
        strncpy(start, raw, 31); start[31] = '\0';
    }

    // Cleanup whitespace
    if (strlen(start) == 0) strcpy(start, "None");
    if (strlen(end) == 0) strcpy(end, "None");
    if (strlen(step) == 0) {
        snprintf(out, out_size, "slice(%s, %s)", start, end);
    } else {
        snprintf(out, out_size, "slice(%s, %s, %s)", start, end, step);
    }
}

PyObject* strfetch(PyObject *self, PyObject *args) {
    const char *input_str;
    const char *input_slices;

    if (!PyArg_ParseTuple(args, "ss", &input_str, &input_slices))
        return NULL;

    PyObject *py_input_str = PyUnicode_FromString(input_str);
    if (!py_input_str)
        return NULL;

    PyObject *result_list = PyList_New(0);
    if (!result_list) {
        Py_DECREF(py_input_str);
        return NULL;
    }

    const char *p = input_slices;
    while (*p) {
        while (*p && (*p == ' ' || *p == '[')) p++;
        const char *q = p;
        while (*q && *q != ']' && *q != ',') q++;

        size_t len = q - p;
        if (len >= 99) len = 98;

        char slice_str[100];
        strncpy(slice_str, p, len);
        slice_str[len] = '\0';

        char eval_expr[128];
        build_slice_expr(slice_str, eval_expr, sizeof(eval_expr));

        PyObject *main_mod = PyImport_AddModule("__main__");
        PyObject *globals_dict = PyModule_GetDict(main_mod);

        PyObject *slice_obj = PyRun_String(eval_expr, Py_eval_input, globals_dict, globals_dict);
        if (!slice_obj) {
            fprintf(stderr, "Error evaluating: %s\n", eval_expr);
            PyErr_Print();
            Py_DECREF(result_list);
            Py_DECREF(py_input_str);
            return NULL;
        }

        PyObject *substr = PyObject_GetItem(py_input_str, slice_obj);
        Py_DECREF(slice_obj);

        if (!substr) {
            fprintf(stderr, "Error slicing with: %s\n", eval_expr);
            PyErr_Print();
            Py_DECREF(result_list);
            Py_DECREF(py_input_str);
            return NULL;
        }

        PyList_Append(result_list, substr);
        Py_DECREF(substr);

        p = q;
        while (*p && (*p == ']' || *p == ',' || *p == ' ')) p++;
    }

    Py_DECREF(py_input_str);
    return result_list;
}