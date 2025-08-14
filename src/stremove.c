/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "stremove.h"
#include <string.h>

static PyObject *_process_string(const char *input_str, const char *char_set, int remove_mode)
{
  int keep_char[256] = {0};

  if (remove_mode)
  {

    for (int i = 0; i < 256; ++i)
      keep_char[i] = 1;

    for (const char *p = char_set; *p; ++p)
    {
      keep_char[(unsigned char)*p] = 0;
    }
  }
  else
  { 

    for (const char *p = char_set; *p; ++p)
    {
      keep_char[(unsigned char)*p] = 1;
    }
  }
  size_t input_len = strlen(input_str);

  char *result_str = (char *)PyMem_Malloc(input_len + 1);
  if (!result_str)
  {
    return PyErr_NoMemory();
  }
  char *writer = result_str;
  for (const char *reader = input_str; *reader; ++reader)
  {
    if (keep_char[(unsigned char)*reader])
    {
      *writer++ = *reader;
    }
  }
  *writer = '\0'; 
  PyObject *py_result = PyUnicode_FromString(result_str);
  PyMem_Free(result_str); 
  return py_result;
}

PyObject *stremove(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *data_obj;
  const char *char_set;
  const char *action = "remove"; 
  static char *kwlist[] = {"data", "char_set", "action", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Os|s", kwlist, &data_obj, &char_set, &action))
  {
    return NULL;
  }
  int remove_mode = (strcmp(action, "remove") == 0);
  if (!remove_mode && strcmp(action, "keep") != 0)
  {
    PyErr_SetString(PyExc_ValueError, "action must be 'remove' or 'keep'");
    return NULL;
  }

  if (PyUnicode_Check(data_obj))
  {
    const char *input_str = PyUnicode_AsUTF8(data_obj);
    if (!input_str)
      return NULL;
    return _process_string(input_str, char_set, remove_mode);
  }

  if (PyList_Check(data_obj))
  {
    Py_ssize_t list_size = PyList_Size(data_obj);
    PyObject *result_list = PyList_New(list_size);
    if (!result_list)
      return NULL;
    for (Py_ssize_t i = 0; i < list_size; ++i)
    {
      PyObject *item = PyList_GetItem(data_obj, i);
      if (!PyUnicode_Check(item))
      {
        Py_DECREF(result_list);
        PyErr_SetString(PyExc_TypeError, "All items in list must be strings");
        return NULL;
      }
      const char *input_str = PyUnicode_AsUTF8(item);
      if (!input_str)
      {
        Py_DECREF(result_list);
        return NULL;
      }
      PyObject *processed_item = _process_string(input_str, char_set, remove_mode);
      if (!processed_item)
      {
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