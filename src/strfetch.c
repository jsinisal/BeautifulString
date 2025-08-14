/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strfetch.h"
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PyObject *parse_slice_component(const char *str, size_t len)
{
  if (len == 0)
  {
    Py_RETURN_NONE;
  }

  char temp[32];
  if (len >= sizeof(temp))
  {
    PyErr_SetString(PyExc_ValueError, "Slice component is too long");
    return NULL;
  }
  memcpy(temp, str, len);
  temp[len] = '\0';
  char *endptr;
  long val = strtol(temp, &endptr, 10);

  if (*endptr != '\0')
  {
    PyErr_Format(PyExc_ValueError, "Invalid character in slice component: %s", temp);
    return NULL;
  }
  return PyLong_FromLong(val);
}

PyObject *strfetch(PyObject *self, PyObject *args)
{
  const char *input_str;
  const char *slices_format_str;
  if (!PyArg_ParseTuple(args, "ss", &input_str, &slices_format_str))
    return NULL;
  PyObject *py_input_str = PyUnicode_FromString(input_str);
  if (!py_input_str)
    return NULL;
  PyObject *result_list = PyList_New(0);
  if (!result_list)
  {
    Py_DECREF(py_input_str);
    return NULL;
  }
  PyObject *start_obj = NULL, *end_obj = NULL, *step_obj = NULL;
  PyObject *slice_obj = NULL;
  PyObject *substr = NULL;
  const char *p = slices_format_str;
  while (*p)
  {

    while (*p && (*p == ' ' || *p == '[' || *p == ','))
      p++;
    if (!*p)
      break; 
    const char *slice_start_ptr = p;

    while (*p && *p != ']' && *p != ',')
      p++;
    const char *slice_end_ptr = p;
    size_t slice_len = slice_end_ptr - slice_start_ptr;

    if (slice_len == 0)
    { 
      start_obj = Py_None;
      Py_INCREF(start_obj);
      end_obj = Py_None;
      Py_INCREF(end_obj);
      step_obj = Py_None;
      Py_INCREF(step_obj);
    }
    else
    {
      const char *first_colon = memchr(slice_start_ptr, ':', slice_len);
      const char *second_colon = first_colon ? memchr(first_colon + 1, ':', slice_end_ptr - (first_colon + 1)) : NULL;
      if (!first_colon)
      { 
        start_obj = parse_slice_component(slice_start_ptr, slice_len);
        if (start_obj && start_obj != Py_None)
        { 
          end_obj = PyLong_FromLong(PyLong_AsLong(start_obj) + 1);
        }
        else
        { 
          end_obj = Py_None;
          Py_INCREF(end_obj);
        }
        step_obj = Py_None;
        Py_INCREF(step_obj);
      }
      else if (!second_colon)
      { 
        start_obj = parse_slice_component(slice_start_ptr, first_colon - slice_start_ptr);
        end_obj = parse_slice_component(first_colon + 1, slice_end_ptr - (first_colon + 1));
        step_obj = Py_None;
        Py_INCREF(step_obj);
      }
      else
      { 
        start_obj = parse_slice_component(slice_start_ptr, first_colon - slice_start_ptr);
        end_obj = parse_slice_component(first_colon + 1, second_colon - (first_colon + 1));
        step_obj = parse_slice_component(second_colon + 1, slice_end_ptr - (second_colon + 1));
      }
    }
    if (!start_obj || !end_obj || !step_obj)
      goto fail;
    slice_obj = PySlice_New(start_obj, end_obj, step_obj);
    if (!slice_obj)
      goto fail;
    substr = PyObject_GetItem(py_input_str, slice_obj);
    if (!substr)
    {
      goto fail;
    }
    PyList_Append(result_list, substr);

    Py_DECREF(substr);
    substr = NULL;
    Py_DECREF(slice_obj);
    slice_obj = NULL;
    Py_DECREF(start_obj);
    start_obj = NULL;
    Py_DECREF(end_obj);
    end_obj = NULL;
    Py_DECREF(step_obj);
    step_obj = NULL;

    p = slice_end_ptr;
    while (*p && *p == ']')
      p++;
  }
  Py_DECREF(py_input_str);
  return result_list;

fail:

  Py_XDECREF(start_obj);
  Py_XDECREF(end_obj);
  Py_XDECREF(step_obj);
  Py_XDECREF(slice_obj);
  Py_XDECREF(substr);
  Py_DECREF(result_list);
  Py_DECREF(py_input_str);
  return NULL;
}