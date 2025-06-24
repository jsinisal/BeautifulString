#include "strmatch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_MATCHES 32

static void skip_spaces(const char **p) {
    while (isspace(**p)) (*p)++;
}

static int parse_named_specifier(const char **p, char *type, char *name) {
    *type = **p;
    (*p)++;
    if (**p == '(') {
        (*p)++;
        int i = 0;
        while (**p && **p != ')' && i < 63) {
            name[i++] = **p;
            (*p)++;
        }
        name[i] = '\0';
        if (**p == ')') {
            (*p)++;
            return 1;
        } else {
            return 0;  // Unterminated name
        }
    }
    name[0] = '\0';
    return 1;
}

PyObject* strmatch(PyObject *self, PyObject *args) {
    const char *input_str;
    const char *pattern_str;

    if (!PyArg_ParseTuple(args, "ss", &input_str, &pattern_str)) {
        fprintf(stderr, "[DEBUG] PyArg_ParseTuple failed\n");
        return NULL;
    }

    fprintf(stderr, "[DEBUG] Entered strmatch with pattern: \"%s\"\n", pattern_str);
    fprintf(stderr, "[DEBUG] Input string: \"%s\"\n", input_str);

    PyObject *result = NULL;
    int use_dict = 0;

    // Pre-check for named fields to decide return type
    const char *check = pattern_str;
    while ((check = strchr(check, '%')) != NULL) {
        check++;
        if (*check && strchr("sdufx", *check)) {
            if (*(check + 1) == '(') {
                use_dict = 1;
                break;
            }
        }
    }

    result = use_dict ? PyDict_New() : PyList_New(0);
    if (!result) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate result container");
        return NULL;
    }

    const char *p = pattern_str;
    const char *s = input_str;

    char temp[256];
    char type;
    char name[64];
    int int_val;
    unsigned int uint_val;
    float float_val;
    double double_val;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '\0') goto mismatch;

            if (*p == '%') {
                if (*s != '%') goto mismatch;
                p++; s++;
                continue;
            }

            skip_spaces(&s);

            if (!parse_named_specifier(&p, &type, name)) goto mismatch;

            PyObject *py_obj = NULL;

            if (type == 's') {
                if (sscanf(s, "%255s", temp) != 1) goto mismatch;
                py_obj = PyUnicode_FromString(temp);
                if (!py_obj) goto mismatch;
                s += strlen(temp);
                fprintf(stderr, "[DEBUG] Matched %%s -> \"%s\"\n", temp);
            } else if (type == 'd') {
                if (sscanf(s, "%d", &int_val) != 1) goto mismatch;
                py_obj = PyLong_FromLong(int_val);
                if (!py_obj) goto mismatch;
                while (*s && (isdigit(*s) || *s == '-' || *s == '+')) s++;
                fprintf(stderr, "[DEBUG] Matched %%d -> %d\n", int_val);
            } else if (type == 'u') {
                if (sscanf(s, "%u", &uint_val) != 1) goto mismatch;
                py_obj = PyLong_FromUnsignedLong(uint_val);
                if (!py_obj) goto mismatch;
                while (*s && isdigit(*s)) s++;
                fprintf(stderr, "[DEBUG] Matched %%u -> %u\n", uint_val);
            } else if (type == 'f') {
                if (sscanf(s, "%f", &float_val) != 1) goto mismatch;
                py_obj = PyFloat_FromDouble(float_val);
                if (!py_obj) goto mismatch;
                while (*s && (isdigit(*s) || *s == '.' || *s == '-' || *s == '+')) s++;
                fprintf(stderr, "[DEBUG] Matched %%f -> %f\n", float_val);
            } else if (type == 'x') {
                if (sscanf(s, "%x", &int_val) != 1) goto mismatch;
                py_obj = PyLong_FromLong(int_val);
                if (!py_obj) goto mismatch;
                while (isxdigit(*s)) s++;
                fprintf(stderr, "[DEBUG] Matched %%x -> %x\n", int_val);
            } else {
                fprintf(stderr, "Unknown format specifier: %%%c\n", type);
                goto mismatch;
            }

            if (use_dict && name[0]) {
                if (PyDict_SetItemString(result, name, py_obj) < 0) {
                    Py_DECREF(py_obj);
                    goto mismatch;
                }
            } else {
                if (PyList_Append(result, py_obj) < 0) {
                    Py_DECREF(py_obj);
                    goto mismatch;
                }
            }

            Py_DECREF(py_obj);
        } else {
            if (*p != *s) {
                fprintf(stderr, "Mismatch at literal: pattern '%c', input '%c'\n", *p, *s);
                goto mismatch;
            }
            p++; s++;
        }
    }

    if (*s != '\0') {
        fprintf(stderr, "Extra characters in input: '%s'\n", s);
        goto mismatch;
    }

    return result;

mismatch:
    fprintf(stderr, "[DEBUG] Entering mismatch cleanup\n");
    Py_XDECREF(result);
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_ValueError, "Pattern did not match input string");
    }
    return NULL;
}
