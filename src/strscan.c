/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strscan.h"
#include <Python.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 64

static const char *skip_whitespace(const char *s)
{
  while (isspace((unsigned char)*s))
    s++;
  return s;
}

static const char *match_literal(const char *input, const char *literal)
{
  size_t len = strlen(literal);
  if (strncmp(input, literal, len) == 0)
  {
    return input + len;
  }
  return NULL;
}

static char *parse_quoted_string(const char **input_ptr)
{
  const char *p = *input_ptr;
  if (*p != '"')
    return NULL;
  p++; 
  size_t buffer_size = 128; 
  char *buffer = malloc(buffer_size);
  if (!buffer)
    return NULL; 
  size_t i = 0;
  while (*p && *p != '"')
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
    if (*p == '\\')
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
  if (*p != '"')
  {
    free(buffer);
    return NULL;
  }
  buffer[i] = '\0';
  *input_ptr = p + 1;
  return buffer;
}

PyObject *strscan(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *input_str;
  const char *format_str;
  const char *return_type = "list";
  PyObject *dict_keys_obj = NULL;
  static char *kwlist[] = {"input_str", "format_str", "return_type", "dict_keys", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|sO", kwlist, &input_str, &format_str, &return_type, &dict_keys_obj))
  {
    return NULL;
  }
  int is_dict = strcmp(return_type, "dict") == 0;
  int is_list = strcmp(return_type, "list") == 0;
  int is_tuple = strcmp(return_type, "tuple") == 0;
  int is_tuple_list = strcmp(return_type, "tuple_list") == 0;
  if (!is_dict && !is_list && !is_tuple && !is_tuple_list)
  {
    PyErr_SetString(PyExc_ValueError, "Invalid return_type: must be 'list', 'tuple', 'dict', or 'tuple_list'");
    return NULL;
  }
  if (dict_keys_obj && !is_dict)
  {
    PyErr_SetString(PyExc_ValueError, "dict_keys can only be used when return_type is 'dict'");
    return NULL;
  }
  PyObject *result_list = PyList_New(0);
  if (!result_list)
    return NULL;
  PyObject *field_name_list = PyList_New(0);
  if (!field_name_list)
  {
    Py_DECREF(result_list);
    return NULL;
  }
  const char *input = input_str;
  const char *fmt = format_str;
  int field_index = 0;
  while (*fmt)
  {
    fmt = skip_whitespace(fmt);
    input = skip_whitespace(input);
    if (!*fmt)
      break;
    if (*fmt == '%')
    {
      fmt++;
      char type = *fmt;
      const char *type_start = fmt;
      if (type == 'l' && *(fmt + 1) == 'f')
      {
        type = 'f';
        fmt += 2;
      }
      else
      {
        fmt++;
      }
      char field_name_buffer[64];
      const char *field_name_ptr = NULL;
      if (*fmt == '(')
      {
        fmt++;
        const char *start = fmt;
        while (*fmt && *fmt != ')')
          fmt++;
        if (!*fmt)
        {
          PyErr_SetString(PyExc_ValueError, "Unmatched parenthesis in field name");
          goto fail;
        }
        size_t len = fmt - start;
        if (len >= sizeof(field_name_buffer))
        {
          PyErr_SetString(PyExc_ValueError, "Field name is too long");
          goto fail;
        }
        memcpy(field_name_buffer, start, len);
        field_name_buffer[len] = '\0';
        field_name_ptr = field_name_buffer;
        fmt++;
      }
      else
      {
        snprintf(field_name_buffer, sizeof(field_name_buffer), "field%d", field_index + 1);
        field_name_ptr = field_name_buffer;
      }
      PyObject *field_name_obj = PyUnicode_FromString(field_name_ptr);
      PyList_Append(field_name_list, field_name_obj);
      Py_DECREF(field_name_obj);
      PyObject *value = NULL;
      input = skip_whitespace(input);
      if (type == 's')
      {
        char *token = NULL;
        if (*input == '"')
        {
          token = parse_quoted_string(&input);
          if (!token)
          {
            PyErr_SetString(PyExc_ValueError, "Failed to parse quoted string");
            goto fail;
          }
        }
        else
        {
          const char *start = input;
          while (*input && !isspace((unsigned char)*input))
            input++;
          size_t len = input - start;
          if (len == 0 && *fmt != '\0')
          {
            PyErr_SetString(PyExc_ValueError, "Expected a string for %s, but found empty space");
            goto fail;
          }
          token = malloc(len + 1);
          memcpy(token, start, len);
          token[len] = '\0';
        }
        value = PyUnicode_FromString(token);
        free(token);
      }
      else if (type == 'd' || type == 'u' || type == 'x')
      {
        char *end;
        if (!*input)
        {
          PyErr_Format(PyExc_ValueError, "Expected number, but found end of string");
          goto fail;
        }
        if (type == 'd')
        {
          long v = strtol(input, &end, 10);
          if (end == input)
          {
            PyErr_SetString(PyExc_ValueError, "Invalid integer for %d");
            goto fail;
          }
          value = PyLong_FromLong(v);
        }
        else
        {
          unsigned long v = strtoul(input, &end, (type == 'u' ? 10 : 16));
          if (end == input)
          {
            PyErr_Format(PyExc_ValueError, "Invalid unsigned number for %%%c", type);
            goto fail;
          }
          value = PyLong_FromUnsignedLong(v);
        }
        input = end;
      }
      else if (type == 'f')
      {
        char *end;
        if (!*input)
        {
          PyErr_SetString(PyExc_ValueError, "Expected float for %f, but found end of string");
          goto fail;
        }
        double v = strtod(input, &end);
        if (end == input)
        {
          PyErr_SetString(PyExc_ValueError, "Invalid float for %f");
          goto fail;
        }
        value = PyFloat_FromDouble(v);
        input = end;
      }
      else
      {
        size_t len = fmt - type_start;
        char temp_spec[16];
        if (len < 15)
        {
          memcpy(temp_spec, type_start, len);
          temp_spec[len] = '\0';
          PyErr_Format(PyExc_ValueError, "Unknown format type: %%%s", temp_spec);
        }
        else
        {
          PyErr_Format(PyExc_ValueError, "Unknown format type near: %c", type);
        }
        goto fail;
      }
      if (!value)
      {
        goto fail;
      }
      PyList_Append(result_list, value);
      Py_DECREF(value);
      field_index++;
    }
    else
    {
      const char *start = fmt;
      while (*fmt && *fmt != '%' && !isspace((unsigned char)*fmt))
        fmt++;
      size_t len = fmt - start;
      char literal[128];
      if (len >= sizeof(literal))
      {
        PyErr_SetString(PyExc_ValueError, "Literal in format string is too long");
        goto fail;
      }
      memcpy(literal, start, len);
      literal[len] = '\0';
      const char *next_input = match_literal(input, literal);
      if (!next_input)
      {
        PyErr_Format(PyExc_ValueError, "Expected literal '%s' not found", literal);
        goto fail;
      }
      input = next_input;
    }
  }

  if (is_dict)
  {
    PyObject *dict = PyDict_New();
    if (dict_keys_obj && PySequence_Check(dict_keys_obj))
    {
      if (PySequence_Length(dict_keys_obj) != field_index)
      {
        PyErr_SetString(PyExc_ValueError, "Number of dict_keys must match number of fields parsed");
        Py_DECREF(dict);
        goto fail;
      }
      for (int i = 0; i < field_index; ++i)
      {
        PyObject *key = PySequence_GetItem(dict_keys_obj, i);
        PyObject *val = PyList_GetItem(result_list, i);
        PyDict_SetItem(dict, key, val);
        Py_DECREF(key);
      }
    }
    else
    {
      for (int i = 0; i < field_index; ++i)
      {
        PyDict_SetItem(dict, PyList_GetItem(field_name_list, i), PyList_GetItem(result_list, i));
      }
    }
    Py_DECREF(result_list);
    Py_DECREF(field_name_list);
    return dict;
  }

  if (is_tuple_list)
  {
    PyObject *new_list = PyList_New(0);
    for (int i = 0; i < field_index; ++i)
    {
      PyObject *pair = PyTuple_Pack(2, PyList_GetItem(field_name_list, i), PyList_GetItem(result_list, i));
      PyList_Append(new_list, pair);
      Py_DECREF(pair);
    }
    Py_DECREF(result_list);
    Py_DECREF(field_name_list);
    return new_list;
  }

  if (is_tuple)
  {
    PyObject *tup = PyList_AsTuple(result_list);
    Py_DECREF(result_list);
    Py_DECREF(field_name_list);
    return tup;
  }

  Py_DECREF(field_name_list);
  return result_list;

fail:
  Py_DECREF(result_list);
  Py_DECREF(field_name_list);
  return NULL;
}