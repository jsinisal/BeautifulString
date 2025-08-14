/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strvalidate.h"
#include "python.h"
#include "strvalidate_match.h"

PyObject *strvalidate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *input_str;
  const char *format_str;
  static char *kwlist[] = {"input_str", "format_str", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", kwlist, &input_str, &format_str))
  {
    return NULL;
  }
  int match = strvalidate_match(input_str, format_str);
  if (match)
  {
    Py_RETURN_TRUE;
  }
  else
  {
    Py_RETURN_FALSE;
  }
}