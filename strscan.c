//
// Created by Juha on 23.6.2025.
//
// Part of BeautifulString python module

#include "strscan.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <Python.h>

#define MAX_TOKENS 64

static const char *skip_whitespace(const char *s) {
    while (isspace(*s)) s++;
    return s;
}

static const char *match_literal(const char *input, const char *literal) {
    while (*literal && *input == *literal) {
        input++;
        literal++;
    }
    return (*literal == '\0') ? input : NULL;
}

static char *parse_quoted_string(const char **input_ptr) {
    const char *p = *input_ptr;
    if (*p != '"') return NULL;
    p++;  // skip opening quote
    char buffer[1024];
    int i = 0;
    while (*p && *p != '"' && i < 1023) {
        if (*p == '\\') {
            p++;
            if (*p == 'n') buffer[i++] = '\n';
            else if (*p == 't') buffer[i++] = '\t';
            else if (*p == '"') buffer[i++] = '"';
            else if (*p == '\\') buffer[i++] = '\\';
            else buffer[i++] = *p;  // unknown escape
        } else {
            buffer[i++] = *p;
        }
        p++;
    }
    if (*p != '"') return NULL;
    buffer[i] = '\0';
    *input_ptr = p + 1;
    return strdup(buffer);
}

PyObject* strscan(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *input_str;
    const char *format_str;
    const char *return_type = "list";
    PyObject *dict_keys_obj = NULL;

    static char *kwlist[] = {"input_str", "format_str", "return_type", "dict_keys", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|sO", kwlist,
                                     &input_str, &format_str, &return_type, &dict_keys_obj)) {
        return NULL;
    }

    int is_dict = strcmp(return_type, "dict") == 0;
    int is_list = strcmp(return_type, "list") == 0;
    int is_tuple = strcmp(return_type, "tuple") == 0;
    int is_tuple_list = strcmp(return_type, "tuple_list") == 0;

    if (!is_dict && !is_list && !is_tuple && !is_tuple_list) {
        PyErr_SetString(PyExc_ValueError, "Invalid return_type");
        return NULL;
    }

    PyObject *result = is_dict ? PyDict_New() : (is_tuple_list ? PyList_New(0) : PyList_New(0));
    const char *input = input_str;
    const char *fmt = format_str;
    int field_index = 0;
    char field_names[MAX_TOKENS][64];

    while (*fmt) {
        fmt = skip_whitespace(fmt);
        input = skip_whitespace(input);

        if (*fmt == '%') {
            fmt++;
            char type = *fmt++;
            const char *field = NULL;

            if (*fmt == '(') {
                fmt++;
                const char *start = fmt;
                while (*fmt && *fmt != ')') fmt++;
                int len = fmt - start;
                if (len <= 0 || len >= 64) {
                    PyErr_SetString(PyExc_ValueError, "Invalid field name length");
                    return NULL;
                }
                strncpy(field_names[field_index], start, len);
                field_names[field_index][len] = '\0';
                field = field_names[field_index];
                fmt++;
            } else {
                snprintf(field_names[field_index], sizeof(field_names[field_index]), "field%d", field_index + 1);
                field = field_names[field_index];
            }

            PyObject *value = NULL;
            input = skip_whitespace(input);

            if (type == 's') {
                char *token = NULL;
                if (*input == '"') {
                    token = parse_quoted_string(&input);
                } else {
                    const char *start = input;
                    while (*input && !isspace(*input)) input++;
                    size_t len = input - start;
                    token = (char *)malloc(len + 1);
                    strncpy(token, start, len);
                    token[len] = '\0';
                }
                if (!token) goto fail;
                value = PyUnicode_FromString(token);
                free(token);

            } else if (type == 'd') {
                char *end;
                long v = strtol(input, &end, 10);
                if (end == input) goto fail;
                value = PyLong_FromLong(v);
                input = end;

            } else if (type == 'u') {
                char *end;
                unsigned long v = strtoul(input, &end, 10);
                if (end == input) goto fail;
                value = PyLong_FromUnsignedLong(v);
                input = end;

            } else if (type == 'x') {
                char *end;
                unsigned long v = strtoul(input, &end, 16);
                if (end == input) goto fail;
                value = PyLong_FromUnsignedLong(v);
                input = end;

            } else if (type == 'f') {
                char *end;
                double v = strtod(input, &end);
                if (end == input) goto fail;
                value = PyFloat_FromDouble(v);
                input = end;

            } else {
                PyErr_Format(PyExc_ValueError, "Unknown format type: %%%c", type);
                goto fail;
            }

            if (is_dict) {
                PyDict_SetItemString(result, field, value);
            } else if (is_tuple_list) {
                PyObject *pair = PyTuple_Pack(2, PyUnicode_FromString(field), value);
                PyList_Append(result, pair);
                Py_DECREF(pair);
            } else {
                PyList_Append(result, value);
            }
            Py_DECREF(value);
            field_index++;

        } else {
            // match literal
            const char *start = fmt;
            while (*fmt && !isspace(*fmt) && *fmt != '%') fmt++;
            char literal[128];
            size_t len = fmt - start;
            if (len >= sizeof(literal)) len = sizeof(literal) - 1;
            strncpy(literal, start, len);
            literal[len] = '\0';

            const char *next_input = match_literal(input, literal);
            if (!next_input) goto fail;
            input = next_input;
        }
    }

    if (is_tuple) {
        PyObject *tup = PyList_AsTuple(result);
        Py_DECREF(result);
        return tup;
    }
    return result;

fail:
    Py_XDECREF(result);
    PyErr_SetString(PyExc_ValueError, "Failed to parse input string");
    return NULL;
}
