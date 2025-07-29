//
// Created by sinj on 29/07/2025.
//
#include "stremove.h"
#include <string.h>

// Helper function that performs the actual character filtering.
static PyObject* _process_string(const char* input_str, const char* char_set, int remove_mode) {
    // Create a boolean lookup table for the character set for O(1) lookups.
    // This is very fast for the 256 possible byte values.
    int keep_char[256] = {0};

    // Populate the lookup table based on the mode.
    if (remove_mode) {
        // Default to keeping all characters.
        for (int i = 0; i < 256; ++i) keep_char[i] = 1;
        // Mark characters from the set for removal.
        for (const char* p = char_set; *p; ++p) {
            keep_char[(unsigned char)*p] = 0;
        }
    } else { // Keep mode
        // Mark characters from the set for keeping.
        for (const char* p = char_set; *p; ++p) {
            keep_char[(unsigned char)*p] = 1;
        }
    }

    size_t input_len = strlen(input_str);
    // Allocate memory for the new string. It can't be larger than the original.
    char* result_str = (char*)PyMem_Malloc(input_len + 1);
    if (!result_str) {
        return PyErr_NoMemory();
    }

    char* writer = result_str;
    for (const char* reader = input_str; *reader; ++reader) {
        if (keep_char[(unsigned char)*reader]) {
            *writer++ = *reader;
        }
    }
    *writer = '\0'; // Null-terminate the new string.

    PyObject* py_result = PyUnicode_FromString(result_str);
    PyMem_Free(result_str); // Free the temporary memory.

    return py_result;
}


PyObject* stremove(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject* data_obj;
    const char* char_set;
    const char* action = "remove"; // Default action

    static char* kwlist[] = {"data", "char_set", "action", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Os|s", kwlist,
                                     &data_obj, &char_set, &action)) {
        return NULL;
    }

    int remove_mode = (strcmp(action, "remove") == 0);
    if (!remove_mode && strcmp(action, "keep") != 0) {
        PyErr_SetString(PyExc_ValueError, "action must be 'remove' or 'keep'");
        return NULL;
    }

    // Handle a single string input
    if (PyUnicode_Check(data_obj)) {
        const char* input_str = PyUnicode_AsUTF8(data_obj);
        if (!input_str) return NULL;
        return _process_string(input_str, char_set, remove_mode);
    }

    // Handle a list of strings input
    if (PyList_Check(data_obj)) {
        Py_ssize_t list_size = PyList_Size(data_obj);
        PyObject* result_list = PyList_New(list_size);
        if (!result_list) return NULL;

        for (Py_ssize_t i = 0; i < list_size; ++i) {
            PyObject* item = PyList_GetItem(data_obj, i);
            if (!PyUnicode_Check(item)) {
                Py_DECREF(result_list);
                PyErr_SetString(PyExc_TypeError, "All items in list must be strings");
                return NULL;
            }
            const char* input_str = PyUnicode_AsUTF8(item);
            if (!input_str) {
                Py_DECREF(result_list);
                return NULL;
            }
            PyObject* processed_item = _process_string(input_str, char_set, remove_mode);
            if (!processed_item) {
                Py_DECREF(result_list);
                return NULL;
            }
            PyList_SetItem(result_list, i, processed_item);
        }
        return result_list;
    }

    PyErr_SetString(PyExc_TypeError, "data must be a string or a list of strings");
    return NULL;
}