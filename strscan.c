//
// Created by Juha on 23.6.2025.
//
// Part of BeautifulString python module

#include "strscan.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static int tokenize_preserve_quotes(const char *input, char *tokens[MAX_FIELDS]) {
    int count = 0;
    const char *p = input;
    while (*p) {
        while (isspace(*p)) p++;
        if (*p == '\0') break;

        if (*p == '"') {
            p++;
            const char *start = p;
            while (*p && *p != '"') p++;
            size_t len = p - start;
            tokens[count] = (char *)malloc(len + 1);
            strncpy(tokens[count], start, len);
            tokens[count][len] = '\0';
            if (*p == '"') p++;
        } else {
            const char *start = p;
            while (*p && !isspace(*p)) p++;
            size_t len = p - start;
            tokens[count] = (char *)malloc(len + 1);
            strncpy(tokens[count], start, len);
            tokens[count][len] = '\0';
        }

        count++;
        if (count >= MAX_FIELDS) break;
    }
    return count;
}

static void free_tokens(char *tokens[], int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
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

    char strbufs[MAX_FIELDS][256];
    int intbufs[MAX_FIELDS];
    unsigned int uintbufs[MAX_FIELDS];
    float floatbufs[MAX_FIELDS];
    double doublebufs[MAX_FIELDS];
    int specifiers[MAX_FIELDS];
    char field_names[MAX_FIELDS][64];
    int count = 0;

    // --- Parse format string for specifiers and optional field names ---
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
            else if (*p == 'l' && *(p + 1) == 'f') { type = 5; p++; }

            if (type) {
                specifiers[count] = type;
                p++;

                if (*p == '(') {
                    p++;
                    const char *start = p;
                    while (*p && *p != ')') p++;
                    int len = p - start;
                    if (len <= 0 || len >= 64) {
                        PyErr_SetString(PyExc_ValueError, "Invalid field name length");
                        return NULL;
                    }
                    strncpy(field_names[count], start, len);
                    field_names[count][len] = '\0';
                    if (*p == ')') p++;
                } else {
                    snprintf(field_names[count], sizeof(field_names[count]), "field%d", count + 1);
                }
                count++;
            } else {
                p++;  // unknown specifier
            }
        } else {
            p++;
        }
    }

    if (count == 0) {
        PyErr_SetString(PyExc_ValueError, "No valid format specifiers found");
        return NULL;
    }

    if (dict_keys_obj && !PyList_Check(dict_keys_obj)) {
        PyErr_SetString(PyExc_TypeError, "dict_keys must be a list of strings");
        return NULL;
    }

    if (dict_keys_obj && PyList_Size(dict_keys_obj) != count) {
        PyErr_SetString(PyExc_ValueError, "Length of dict_keys must match number of format specifiers");
        return NULL;
    }

    char *tokens[MAX_FIELDS] = {0};
    int token_count = tokenize_preserve_quotes(input_str, tokens);
    if (token_count < count) {
        PyErr_SetString(PyExc_ValueError, "Not enough tokens in input");
        free_tokens(tokens, token_count);
        return NULL;
    }

    int is_list = strcmp(return_type, "list") == 0;
    int is_tuple = strcmp(return_type, "tuple") == 0;
    int is_dict = strcmp(return_type, "dict") == 0;
    int is_tuple_list = strcmp(return_type, "tuple_list") == 0;

    if (!is_list && !is_tuple && !is_dict && !is_tuple_list) {
        free_tokens(tokens, token_count);
        PyErr_SetString(PyExc_ValueError, "return_type must be 'list', 'tuple', 'dict', or 'tuple_list'");
        return NULL;
    }

    PyObject *result = NULL;
    PyObject *tmp_list = NULL;

    if (is_dict) {
        result = PyDict_New();
    } else if (is_tuple_list) {
        result = PyList_New(0);
    } else {
        tmp_list = PyList_New(0);
    }

    for (int i = 0; i < count; i++) {
        PyObject *value = NULL;
        int ok = 0;

        switch (specifiers[i]) {
            case 1:
                strncpy(strbufs[i], tokens[i], 255);
                strbufs[i][255] = '\0';
                value = PyUnicode_FromString(strbufs[i]);
                ok = 1;
                break;
            case 2:
                ok = sscanf(tokens[i], "%d", &intbufs[i]) == 1;
                if (ok) value = PyLong_FromLong(intbufs[i]);
                break;
            case 3:
                ok = sscanf(tokens[i], "%f", &floatbufs[i]) == 1;
                if (ok) value = PyFloat_FromDouble(floatbufs[i]);
                break;
            case 4:
                ok = sscanf(tokens[i], "%u", &uintbufs[i]) == 1;
                if (ok) value = PyLong_FromUnsignedLong(uintbufs[i]);
                break;
            case 5:
                ok = sscanf(tokens[i], "%lf", &doublebufs[i]) == 1;
                if (ok) value = PyFloat_FromDouble(doublebufs[i]);
                break;
            case 6:
                ok = sscanf(tokens[i], "%x", &intbufs[i]) == 1;
                if (ok) value = PyLong_FromLong(intbufs[i]);
                break;
        }

        if (!ok || !value) {
            free_tokens(tokens, token_count);
            Py_XDECREF(result);
            Py_XDECREF(tmp_list);
            PyErr_Format(PyExc_ValueError, "Failed to parse token: %s (field %d)", tokens[i], i);
            return NULL;
        }

        const char *field_name = NULL;
        if (dict_keys_obj) {
            PyObject *py_key = PyList_GetItem(dict_keys_obj, i);
            field_name = PyUnicode_AsUTF8(py_key);
        } else {
            field_name = field_names[i];
        }

        if (is_dict) {
            PyDict_SetItemString(result, field_name, value);
        } else if (is_tuple_list) {
            PyObject *key = PyUnicode_FromString(field_name);
            PyObject *pair = PyTuple_Pack(2, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            PyList_Append(result, pair);
            Py_DECREF(pair);
        } else {
            PyList_Append(tmp_list, value);
            Py_DECREF(value);
        }
    }

    free_tokens(tokens, token_count);

    if (is_dict || is_tuple_list) {
        return result;
    } else if (is_tuple) {
        PyObject *tuple_result = PyList_AsTuple(tmp_list);
        Py_DECREF(tmp_list);
        return tuple_result;
    } else {
        return tmp_list;
    }
}
