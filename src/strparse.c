// strparse.c
#include "strparse.h"
#include <Python.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_FIELDS 64

// Structure to hold specifier details
typedef struct {
    int type;  // 1=s, 2=d, 3=f, 4=u, 5=lf, 6=x
    char name[64];
} FormatField;

static int parse_format_string(const char *format_str, FormatField fields[], int *field_count) {
    int count = 0;
    const char *p = format_str;
    while (*p && count < MAX_FIELDS) {
        if (*p == '%') {
            p++;
            int type = 0;
            if (*p == 's') type = 1;
            else if (*p == 'd') type = 2;
            else if (*p == 'f') type = 3;
            else if (*p == 'u') type = 4;
            else if (*p == 'x') type = 6;
            else if (*p == 'l' && *(p+1) == 'f') { type = 5; p++; }
            else continue;
            p++;
            if (*p == '(') {
                p++;
                const char *start = p;
                while (*p && *p != ')') p++;
                size_t len = p - start;
                if (len >= sizeof(fields[count].name)) return -1;
                strncpy(fields[count].name, start, len);
                fields[count].name[len] = '\0';
                if (*p == ')') p++;
            } else {
                snprintf(fields[count].name, sizeof(fields[count].name), "field%d", count+1);
            }
            fields[count].type = type;
            count++;
        } else {
            p++;
        }
    }
    *field_count = count;
    return 0;
}

static int skip_literal(const char **s_ptr, const char **f_ptr) {
    const char *s = *s_ptr;
    const char *f = *f_ptr;
    while (*f && *f != '%') {
        if (isspace(*f)) {
            while (isspace(*s)) s++;
            while (isspace(*f)) f++;
        } else {
            if (*s != *f) return 0;
            s++;
            f++;
        }
    }
    *s_ptr = s;
    *f_ptr = f;
    return 1;
}

PyObject* strparse(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *input_str;
    const char *format_str;
    const char *return_type = "dict";

    static char *kwlist[] = {"input_str", "format_str", "return_type", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|s", kwlist,
                                     &input_str, &format_str, &return_type)) {
        return NULL;
    }

    FormatField fields[MAX_FIELDS];
    int field_count = 0;
    if (parse_format_string(format_str, fields, &field_count) != 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to parse format string");
        return NULL;
    }

    PyObject *result = NULL;
    if (strcmp(return_type, "dict") == 0) {
        result = PyDict_New();
    } else if (strcmp(return_type, "tuple") == 0 || strcmp(return_type, "tuple_list") == 0) {
        result = PyList_New(0);
    } else {
        PyErr_SetString(PyExc_ValueError, "Invalid return_type");
        return NULL;
    }

    const char *s = input_str;
    const char *f = format_str;

    for (int i = 0; i < field_count; i++) {
        if (!skip_literal(&s, &f)) {
            Py_XDECREF(result);
            PyErr_SetString(PyExc_ValueError, "Failed to match literal text in format string");
            return NULL;
        }

        // Skip format specifier
        if (*f == '%') {
            f++;
            if (*f == 'l' && *(f+1) == 'f') f += 2;
            else f++;
            if (*f == '(') {
                while (*f && *f != ')') f++;
                if (*f == ')') f++;
            }
        }

        while (isspace(*s)) s++;

        char buffer[256] = {0};
        int matched = 0;
        PyObject *val = NULL;
        int consumed = 0;

        switch (fields[i].type) {
            case 1:
                matched = sscanf(s, "%255s%n", buffer, &consumed);
                if (matched == 1) val = PyUnicode_FromString(buffer);
                break;
            case 2: {
                int ivalue;
                matched = sscanf(s, "%d%n", &ivalue, &consumed);
                if (matched == 1) val = PyLong_FromLong(ivalue);
                break;
            }
            case 3: {
                float fval;
                matched = sscanf(s, "%f%n", &fval, &consumed);
                if (matched == 1) val = PyFloat_FromDouble(fval);
                break;
            }
            case 4: {
                unsigned int uval;
                matched = sscanf(s, "%u%n", &uval, &consumed);
                if (matched == 1) val = PyLong_FromUnsignedLong(uval);
                break;
            }
            case 5: {
                double lfval;
                matched = sscanf(s, "%lf%n", &lfval, &consumed);
                if (matched == 1) val = PyFloat_FromDouble(lfval);
                break;
            }
            case 6: {
                int hexval;
                matched = sscanf(s, "%x%n", &hexval, &consumed);
                if (matched == 1) val = PyLong_FromLong(hexval);
                break;
            }
        }

        if (!val) {
            Py_XDECREF(result);
            PyErr_Format(PyExc_ValueError, "Failed to parse field: %s", fields[i].name);
            return NULL;
        }

        if (strcmp(return_type, "dict") == 0) {
            PyDict_SetItemString(result, fields[i].name, val);
            Py_DECREF(val);
        } else if (strcmp(return_type, "tuple_list") == 0) {
            PyObject *key = PyUnicode_FromString(fields[i].name);
            PyObject *pair = PyTuple_Pack(2, key, val);
            Py_DECREF(key);
            Py_DECREF(val);
            PyList_Append(result, pair);
            Py_DECREF(pair);
        } else {
            PyList_Append(result, val);
            Py_DECREF(val);
        }

        s += consumed;
        while (isspace(*s)) s++;
    }

    if (strcmp(return_type, "tuple") == 0) {
        PyObject *t = PyList_AsTuple(result);
        Py_DECREF(result);
        return t;
    }

    return result;
}