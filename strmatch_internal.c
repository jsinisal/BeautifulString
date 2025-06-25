#include "strmatch_internal.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

// Helper struct
typedef struct {
    const char *name;
    PyObject *value;
} NamedField;

// Forward declarations
static int parse_format_token(const char **pattern_ptr, char *type_code, char *field_name);
static PyObject* build_result_object(PyObject **values, NamedField *fields, int field_count, int val_index, const char *return_type);

PyObject* strmatch_internal(PyObject *self, const char *input_str, const char *pattern_str, const char *return_type, int *consumed_chars, int partial_mode) {
    const char *input = input_str;
    const char *pattern = pattern_str;
    PyObject *values[32];
    NamedField fields[32];
    int field_count = 0;
    int val_index = 0;
    int matched_any_field = 0;

    while (*pattern && *input) {
        if (*pattern == '%') {
            pattern++;
            char type_code = 0;
            char field_name[64] = {0};
            if (!parse_format_token(&pattern, &type_code, field_name)) {
                PyErr_SetString(PyExc_ValueError, "Invalid format token");
                return NULL;
            }

            const char *start = input;
            PyObject *val = NULL;
            if (type_code == 's') {
                if (*input == '"' || *input == '\'') {
                    char quote = *input++;
                    PyObject *str_obj = PyUnicode_New(0, 127);
                    Py_ssize_t len = 0;
                    while (*input && *input != quote) {
                        char c = *input++;
                        if (c == '\\' && *input) {
                            char esc = *input++;
                            switch (esc) {
                                case 'n': c = '\n'; break;
                                case 't': c = '\t'; break;
                                case 'r': c = '\r'; break;
                                case '\\': c = '\\'; break;
                                case '\"': c = '\"'; break;
                                case '\'': c = '\''; break;
                                default: c = esc; break;
                            }
                        }
                        PyUnicode_AppendAndDel(&str_obj, PyUnicode_FromStringAndSize(&c, 1));
                        len++;
                    }
                    if (*input == quote) input++;
                    val = str_obj;
                } else {
                    while (*input && !isspace(*input)) input++;
                    val = PyUnicode_FromStringAndSize(start, input - start);
                }
            } else if (type_code == 'd') {
                char *end;
                long num = strtol(input, &end, 10);
                if (end == input) {
                    PyErr_SetString(PyExc_ValueError, "Failed to parse int");
                    return NULL;
                }
                val = PyLong_FromLong(num);
                input = end;
            } else if (type_code == 'u') {
                char *end;
                unsigned long num = strtoul(input, &end, 10);
                if (end == input) {
                    PyErr_SetString(PyExc_ValueError, "Failed to parse unsigned int");
                    return NULL;
                }
                val = PyLong_FromUnsignedLong(num);
                input = end;
            } else if (type_code == 'x') {
                char *end;
                unsigned long num = strtoul(input, &end, 16);
                if (end == input) {
                    PyErr_SetString(PyExc_ValueError, "Failed to parse hex int");
                    return NULL;
                }
                val = PyLong_FromUnsignedLong(num);
                input = end;
            } else if (type_code == 'f') {
                char *end;
                double num = strtod(input, &end);
                if (end == input) {
                    PyErr_SetString(PyExc_ValueError, "Failed to parse float");
                    return NULL;
                }
                val = PyFloat_FromDouble(num);
                input = end;
            } else {
                PyErr_SetString(PyExc_ValueError, "Unsupported format type");
                return NULL;
            }

            values[val_index] = val;
            if (field_name[0]) {
                fields[field_count].name = strdup(field_name);
                fields[field_count].value = val;
                field_count++;
            }
            val_index++;
            matched_any_field = 1;
        } else {
            if (*pattern != *input) {
                if (partial_mode) break;
                PyErr_SetString(PyExc_ValueError, "Literal mismatch");
                return NULL;
            }
            pattern++;
            input++;
        }
    }

    if (!matched_any_field) {
        for (int i = 0; i < val_index; i++) {
            Py_XDECREF(values[i]);
        }
        for (int i = 0; i < field_count; i++) {
            free((void*)fields[i].name);
        }
        return NULL;
    }

    if (consumed_chars) {
        *consumed_chars = input - input_str;
    }

    printf("[DEBUG] field_count: %d, val_index: %d, return_type: %s\n", field_count, val_index, return_type ? return_type : "NULL");

    return build_result_object(values, fields, field_count, val_index, return_type);
}

static int parse_format_token(const char **pattern_ptr, char *type_code, char *field_name) {
    const char *p = *pattern_ptr;
    if (!*p) return 0;
    *type_code = *p++;
    if (*p == '(') {
        p++;
        int i = 0;
        while (*p && *p != ')' && i < 63) {
            field_name[i++] = *p++;
        }
        field_name[i] = '\0';
        if (*p != ')') return 0;
        p++;
    }
    *pattern_ptr = p;
    return 1;
}

static PyObject* build_result_object(PyObject **values, NamedField *fields, int field_count, int val_index, const char *return_type) {
    if (return_type && strcmp(return_type, "tuple") == 0) {
        PyObject *tuple = PyTuple_New(val_index);
        for (int i = 0; i < val_index; i++) {
            PyTuple_SetItem(tuple, i, values[i]);
        }
        return tuple;
    }

    if (return_type && strcmp(return_type, "list") == 0) {
        PyObject *list = PyList_New(val_index);
        for (int i = 0; i < val_index; i++) {
            PyList_SetItem(list, i, values[i]);
        }
        return list;
    }

    if (return_type && strcmp(return_type, "dict") == 0 && field_count > 0) {
        PyObject *dict = PyDict_New();
        for (int i = 0; i < field_count; i++) {
            PyDict_SetItemString(dict, fields[i].name, fields[i].value);
            free((void*)fields[i].name);
        }
        return dict;
    }

    if (return_type && strcmp(return_type, "tuple_list") == 0 && field_count > 0) {
        PyObject *list = PyList_New(field_count);
        for (int i = 0; i < field_count; i++) {
            PyObject *pair = PyTuple_Pack(2, PyUnicode_FromString(fields[i].name), fields[i].value);
            PyList_SetItem(list, i, pair);
            free((void*)fields[i].name);
        }
        return list;
    }

    if (field_count > 0) {
        PyObject *dict = PyDict_New();
        for (int i = 0; i < field_count; i++) {
            PyDict_SetItemString(dict, fields[i].name, fields[i].value);
            free((void*)fields[i].name);
        }
        return dict;
    }

    PyObject *list = PyList_New(val_index);
    for (int i = 0; i < val_index; i++) {
        PyList_SetItem(list, i, values[i]);
    }
    return list;
}
