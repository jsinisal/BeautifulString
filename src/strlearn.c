//
// Created by sinj on 11/07/2025.
//

// strlearn.c
#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strlearn.h"

typedef struct {
    char* value;
    size_t length;
} Token;

typedef struct {
    Token* tokens;
    size_t count;
} TokenList;

enum TokenType { TYPE_INT, TYPE_FLOAT, TYPE_STRING };

static TokenList tokenize(const char* str) {
    TokenList list = { NULL, 0 };
    size_t capacity = 8;
    list.tokens = PyMem_Malloc(capacity * sizeof(Token));

    size_t i = 0;
    while (str[i]) {
        while (str[i] && !isalnum(str[i]) && str[i] != '"') i++;
        if (!str[i]) break;

        size_t start = i;
        if (str[i] == '"') {
            i++;
            start = i;
            while (str[i] && str[i] != '"') i++;
        } else {
            while (str[i] && (isalnum(str[i]) || str[i] == '.' || str[i] == '_')) i++;
        }

        size_t len = i - start;
        if (len > 0) {
            if (list.count == capacity) {
                capacity *= 2;
                list.tokens = PyMem_Realloc(list.tokens, capacity * sizeof(Token));
            }

            list.tokens[list.count].value = PyMem_Malloc(len + 1);
            memcpy(list.tokens[list.count].value, &str[start], len);
            list.tokens[list.count].value[len] = '\0';
            list.tokens[list.count].length = len;
            list.count++;
        }

        if (str[i] == '"') i++;
    }

    return list;
}

static enum TokenType infer_type(const char* s) {
    int has_dot = 0;
    for (int i = 0; s[i]; ++i) {
        if (isdigit(s[i])) continue;
        if (s[i] == '.' && !has_dot) {
            has_dot = 1;
            continue;
        }
        return TYPE_STRING;
    }
    return has_dot ? TYPE_FLOAT : TYPE_INT;
}

PyObject* strlearn(PyObject* self, PyObject* args) {
    PyObject* input_list;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &input_list)) {
        return PyErr_Format(PyExc_TypeError, "Expected a list of strings.");
    }

    Py_ssize_t list_size = PyList_Size(input_list);
    if (list_size < 2) {
        return PyErr_Format(PyExc_ValueError, "At least two example strings are required.");
    }

    TokenList* rows = PyMem_Malloc(list_size * sizeof(TokenList));
    for (Py_ssize_t i = 0; i < list_size; ++i) {
        PyObject* item = PyList_GetItem(input_list, i);
        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "All elements must be strings.");
            goto cleanup_rows;
        }
        const char* str = PyUnicode_AsUTF8(item);
        rows[i] = tokenize(str);
    }

    size_t column_count = rows[0].count;
    enum TokenType* types = PyMem_Malloc(column_count * sizeof(enum TokenType));

    for (size_t col = 0; col < column_count; ++col) {
        types[col] = infer_type(rows[0].tokens[col].value);
        for (Py_ssize_t row = 1; row < list_size; ++row) {
            if (col >= rows[row].count) {
                types[col] = TYPE_STRING;
                break;
            }
            enum TokenType t = infer_type(rows[row].tokens[col].value);
            if (t != types[col]) {
                types[col] = TYPE_STRING;
                break;
            }
        }
    }

    PyObject* format_str = PyUnicode_FromString("");
    for (size_t i = 0; i < column_count; ++i) {
        const char* fmt = NULL;
        switch (types[i]) {
            case TYPE_INT: fmt = "%%d(field%zu)"; break;
            case TYPE_FLOAT: fmt = "%%f(field%zu)"; break;
            case TYPE_STRING: fmt = "%%s(field%zu)"; break;
        }
        PyObject* part = PyUnicode_FromFormat(fmt, i);
        PyUnicode_Append(&format_str, part);
        Py_DECREF(part);
        if (i + 1 < column_count) {
            PyUnicode_Append(&format_str, PyUnicode_FromString(" "));
        }
    }

    for (Py_ssize_t r = 0; r < list_size; ++r) {
        for (size_t t = 0; t < rows[r].count; ++t) {
            PyMem_Free(rows[r].tokens[t].value);
        }
        PyMem_Free(rows[r].tokens);
    }
    PyMem_Free(rows);
    PyMem_Free(types);

    return format_str;

cleanup_rows:
    for (Py_ssize_t r = 0; r < list_size; ++r) {
        for (size_t t = 0; t < rows[r].count; ++t) {
            PyMem_Free(rows[r].tokens[t].value);
        }
        PyMem_Free(rows[r].tokens);
    }
    PyMem_Free(rows);
    return NULL;
}
