/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strsearch.h"
#include "strmatch_internal.h"
#include <Python.h>
#include <string.h>


PyObject *strsearch(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *input_str, *pattern_str, *return_type = NULL;
  int start_index = 0, max_matches = -1;
  static char *kwlist[] = {"input_str", "pattern_str", "start_index", "max_matches", "return_container", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|iis", kwlist, &input_str, &pattern_str, &start_index, &max_matches, &return_type))
  {
    return NULL;
  }
  PyObject *matches = PyList_New(0);
  if (!matches)
    return NULL;
  const char *s = input_str + start_index;
  int match_count = 0;
  while (*s && (max_matches == -1 || match_count < max_matches))
  {
    int consumed = 0;
    PyObject *result = strmatch_internal(self, s, pattern_str, return_type, &consumed, /*partial_mode=*/1);
    if (result)
    {
      PyList_Append(matches, result);
      Py_DECREF(result);
      match_count++;
      s += consumed;
    }
    else
    {
      s++;
    }
  }
  if (match_count == 1 && return_type && strcmp(return_type, "tuple") == 0)
  {
    return PyList_GetItem(matches, 0);
  }
  return matches;
}