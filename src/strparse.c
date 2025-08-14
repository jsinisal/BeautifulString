/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strparse.h"
#include <Python.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_whitespace(const char *s)
{
  while (isspace((unsigned char)*s))
    s++;
  return s;
}

static char *parse_string_token(const char **input_ptr)
{
  const char *p = *input_ptr;
  char *buffer = NULL;
  size_t i = 0;
  if (*p == '"' || *p == '\'')
  {
    char quote_char = *p++;
    size_t buffer_size = 128;
    buffer = malloc(buffer_size);
    if (!buffer)
      return NULL;
    while (*p && *p != quote_char)
    {
      if (i >= buffer_size - 1)
      {
        buffer_size *= 2;
        char *new_buffer = realloc(buffer, buffer_size);
        if (!new_buffer)
        {
          free(buffer);
          return NULL;
        }
        buffer = new_buffer;
      }
      if (*p == '\\' && *(p + 1))
      {
        p++;
        switch (*p)
        {
        case 'n':
          buffer[i++] = '\n';
          break;
        case 't':
          buffer[i++] = '\t';
          break;
        case '"':
          buffer[i++] = '"';
          break;
        case '\'':
          buffer[i++] = '\'';
          break;
        case '\\':
          buffer[i++] = '\\';
          break;
        default:
          buffer[i++] = *p;
          break;
        }
      }
      else
      {
        buffer[i++] = *p;
      }
      p++;
    }
    if (*p == quote_char)
      p++;
  }
  else
  {
    const char *start = p;
    while (*p && !isspace((unsigned char)*p))
      p++;
    size_t len = p - start;
    if (len == 0)
      return strdup("");
    buffer = malloc(len + 1);
    if (!buffer)
      return NULL;
    memcpy(buffer, start, len);
    i = len;
  }
  if (buffer)
    buffer[i] = '\0';
  *input_ptr = p;
  return buffer;
}

PyObject *strparse(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *input_str;
  const char *format_str;
  const char *return_type = "dict";
  PyObject *dict_keys_obj = NULL;
  static char *kwlist[] = {"input_str", "format_str", "return_type", "dict_keys", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|sO", kwlist, &input_str, &format_str, &return_type, &dict_keys_obj))
  {
    return NULL;
  }
  int is_dict = strcmp(return_type, "dict") == 0;
  int is_tuple = strcmp(return_type, "tuple") == 0;
  int is_tuple_list = strcmp(return_type, "tuple_list") == 0;
  int is_list = strcmp(return_type, "list") == 0;
  if (!is_dict && !is_tuple && !is_tuple_list && !is_list)
  {
    PyErr_SetString(PyExc_ValueError, "Invalid return_type");
    return NULL;
  }
  PyObject *values_list = PyList_New(0);
  PyObject *names_list = PyList_New(0);
  if (!values_list || !names_list)
    goto fail;
  const char *s = input_str;
  const char *f = format_str;
  int field_index = 0;
  while (*f)
  {
    if (*f == '%')
    {
      f++; 
      char type = *f;
      if (type == 'l' && *(f + 1) == 'f')
      {
        type = 'f';
        f++;
      }
      f++;
      char name_buffer[64];
      if (*f == '(')
      {
        f++;
        const char *name_start = f;
        while (*f && *f != ')')
          f++;
        size_t len = f - name_start;
        if (len >= sizeof(name_buffer) || !*f)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid field name");
          goto fail;
        }
        memcpy(name_buffer, name_start, len);
        name_buffer[len] = '\0';
        f++;
      }
      else
      {
        snprintf(name_buffer, sizeof(name_buffer), "field%d", field_index + 1);
      }
      PyList_Append(names_list, PyUnicode_FromString(name_buffer));
      s = skip_whitespace(s);
      if (*s == '\0')
      {
        PyErr_SetString(PyExc_ValueError, "Input ended before all fields were parsed");
        goto fail;
      }
      PyObject *val = NULL;
      char *endptr;
      switch (type)
      {
      case 's':
      {
        char *str_val = parse_string_token(&s);
        if (!str_val)
        {
          PyErr_SetString(PyExc_ValueError, "Failed to parse string");
          goto fail;
        }
        val = PyUnicode_FromString(str_val);
        free(str_val);
        break;
      }
      case 'd':
      {
        long v = strtol(s, &endptr, 10);
        if (s == endptr)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid integer");
          goto fail;
        }
        s = endptr;
        val = PyLong_FromLong(v);
        break;
      }
      case 'f':
      {
        double v = strtod(s, &endptr);
        if (s == endptr)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid float");
          goto fail;
        }
        s = endptr;
        val = PyFloat_FromDouble(v);
        break;
      }
      case 'u':
      {
        unsigned long v = strtoul(s, &endptr, 10);
        if (s == endptr)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid unsigned integer");
          goto fail;
        }
        s = endptr;
        val = PyLong_FromUnsignedLong(v);
        break;
      }
      case 'x':
      {
        unsigned long v = strtoul(s, &endptr, 16);
        if (s == endptr)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid hex integer");
          goto fail;
        }
        s = endptr;
        val = PyLong_FromUnsignedLong(v);
        break;
      }
      default:
        PyErr_Format(PyExc_ValueError, "Unknown format specifier: %c", type);
        goto fail;
      }
      if (!val)
        goto fail;
      PyList_Append(values_list, val);
      Py_DECREF(val);
      field_index++;
    }
    else if (isspace((unsigned char)*f))
    {
      f = skip_whitespace(f);
      s = skip_whitespace(s);
    }
    else
    { 
      if (*f != *s)
      {
        PyErr_Format(PyExc_ValueError, "Literal '%c' not found", *f);
        goto fail;
      }
      f++;
      s++;
    }
  }
  s = skip_whitespace(s);
  if (*s != '\0')
  {
    PyErr_SetString(PyExc_ValueError, "Input string has trailing characters");
    goto fail;
  }
  if (is_dict)
  {
    PyObject *dict = PyDict_New();
    PyObject *key_source = (dict_keys_obj && PySequence_Check(dict_keys_obj)) ? dict_keys_obj : names_list;
    if (PySequence_Length(key_source) != field_index)
    {
      PyErr_SetString(PyExc_ValueError, "Number of keys must match number of fields parsed");
      Py_DECREF(dict);
      goto fail;
    }
    for (int i = 0; i < field_index; i++)
    {
      PyObject *key = PySequence_GetItem(key_source, i);
      PyObject *value = PyList_GetItem(values_list, i);
      if (!key)
      {
        Py_DECREF(dict);
        goto fail;
      }
      PyDict_SetItem(dict, key, value);
      Py_DECREF(key);
    }
    Py_DECREF(values_list);
    Py_DECREF(names_list);
    return dict;
  }
  if (is_tuple_list)
  {
    PyObject *list = PyList_New(0);
    for (int i = 0; i < field_index; i++)
    {
      PyObject *pair = PyTuple_Pack(2, PyList_GetItem(names_list, i), PyList_GetItem(values_list, i));
      PyList_Append(list, pair);
      Py_DECREF(pair);
    }
    Py_DECREF(values_list);
    Py_DECREF(names_list);
    return list;
  }
  if (is_tuple)
  {
    PyObject *tup = PyList_AsTuple(values_list);
    Py_DECREF(values_list);
    Py_DECREF(names_list);
    return tup;
  }
  Py_DECREF(names_list);
  return values_list;

fail:
  Py_XDECREF(values_list);
  Py_XDECREF(names_list);
  return NULL;
}