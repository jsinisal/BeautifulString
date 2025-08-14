/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strlearn.h"
#include <Python.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  TOKEN_TYPE_INT,
  TOKEN_TYPE_FLOAT,
  TOKEN_TYPE_STRING
} TokenType;

static TokenType infer_type(PyObject *token_str)
{
  const char *s = PyUnicode_AsUTF8(token_str);
  int has_dot = 0;
  if (!s || s[0] == '\0')
    return TOKEN_TYPE_STRING; 
  for (int i = 0; s[i]; ++i)
  {
    if (isdigit(s[i]))
      continue;
    if (s[i] == '.' && !has_dot)
    {
      has_dot = 1;
      continue;
    }
    return TOKEN_TYPE_STRING;
  }
  return has_dot ? TOKEN_TYPE_FLOAT : TOKEN_TYPE_INT;
}

static PyObject *tokenize_string(const char *str)
{
  PyObject *token_list = PyList_New(0);
  if (!token_list)
    return NULL;
  size_t i = 0;
  while (str[i])
  {
    while (str[i] && !isalnum(str[i]) && str[i] != '"')
      i++;
    if (!str[i])
      break;
    size_t start = i;
    if (str[i] == '"')
    {
      i++;
      start = i;
      while (str[i] && str[i] != '"')
        i++;
    }
    else
    {
      while (str[i] && (isalnum(str[i]) || str[i] == '.' || str[i] == '_'))
        i++;
    }
    size_t len = i - start;
    if (len > 0)
    {
      PyObject *token = PyUnicode_FromStringAndSize(&str[start], len);
      if (!token || PyList_Append(token_list, token) != 0)
      {
        Py_XDECREF(token);
        Py_DECREF(token_list);
        return NULL;
      }
      Py_DECREF(token);
    }
    if (str[i] == '"')
      i++;
  }
  return token_list;
}

PyObject *strlearn(PyObject *self, PyObject *args, PyObject *kwds)
{
  PyObject *input_list;
  const char *format = "list"; 
  static char *kwlist[] = {"list_of_strings", "format", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|s", kwlist, &PyList_Type, &input_list, &format))
  {
    return NULL;
  }
  Py_ssize_t num_rows = PyList_Size(input_list);
  if (num_rows < 1)
  {
    return PyList_New(0);
  }
  PyObject *tokenized_rows = PyList_New(num_rows);
  if (!tokenized_rows)
    return NULL;
  for (Py_ssize_t i = 0; i < num_rows; ++i)
  {
    PyObject *item = PyList_GetItem(input_list, i);
    if (!PyUnicode_Check(item))
    {
      PyErr_SetString(PyExc_TypeError, "All elements must be strings.");
      Py_DECREF(tokenized_rows);
      return NULL;
    }
    PyObject *tokens = tokenize_string(PyUnicode_AsUTF8(item));
    if (!tokens)
    {
      Py_DECREF(tokenized_rows);
      return NULL;
    }
    PyList_SET_ITEM(tokenized_rows, i, tokens);
  }
  PyObject *first_row = PyList_GetItem(tokenized_rows, 0);
  Py_ssize_t num_cols = PyList_Size(first_row);
  TokenType *col_types = PyMem_Malloc(num_cols * sizeof(TokenType));
  if (!col_types)
  {
    Py_DECREF(tokenized_rows);
    return PyErr_NoMemory();
  }
  for (Py_ssize_t col = 0; col < num_cols; ++col)
  {
    col_types[col] = infer_type(PyList_GetItem(first_row, col));
    for (Py_ssize_t row = 1; row < num_rows; ++row)
    {
      PyObject *current_row = PyList_GetItem(tokenized_rows, row);
      if (col >= PyList_Size(current_row) || col_types[col] == TOKEN_TYPE_STRING)
      {
        col_types[col] = TOKEN_TYPE_STRING;
        break;
      }
      if (infer_type(PyList_GetItem(current_row, col)) != col_types[col])
      {
        col_types[col] = TOKEN_TYPE_STRING;
      }
    }
  }
  Py_DECREF(tokenized_rows);

  PyObject *result = NULL;
  if (strcmp(format, "list") == 0)
  { 
    result = PyList_New(num_cols);
    for (Py_ssize_t i = 0; i < num_cols; ++i)
    {
      const char *type_name = "string";
      if (col_types[i] == TOKEN_TYPE_INT)
        type_name = "int";
      else if (col_types[i] == TOKEN_TYPE_FLOAT)
        type_name = "float";
      PyList_SET_ITEM(result, i, PyUnicode_FromString(type_name));
    }
  }
  else if (strcmp(format, "c-style") == 0)
  { 
    PyObject *parts = PyList_New(0);
    for (Py_ssize_t i = 0; i < num_cols; ++i)
    {
      const char *fmt = "%%s(field%zd)";
      if (col_types[i] == TOKEN_TYPE_INT)
        fmt = "%%d(field%zd)";
      else if (col_types[i] == TOKEN_TYPE_FLOAT)
        fmt = "%%f(field%zd)";
      PyObject *part = PyUnicode_FromFormat(fmt, i + 1);
      PyList_Append(parts, part);
      Py_DECREF(part);
    }
    PyObject *sep = PyUnicode_FromString(" ");
    result = PyUnicode_Join(sep, parts);
    Py_DECREF(sep);
    Py_DECREF(parts);
  }
  else
  {
    PyErr_SetString(PyExc_ValueError, "format must be 'list' or 'c-style'");
  }
  PyMem_Free(col_types);
  return result;
}