/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#define PY_SSIZE_T_CLEAN
#include "bstring.h"
#include "Python.h"
#include "library.h"

static PyObject *BString_unique(BStringObject *self, PyObject *Py_UNUSED(args))
{

  BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
  if (!result)
    return NULL;

  PyObject *seen_set = PySet_New(NULL);
  if (!seen_set)
  {
    Py_DECREF(result);
    return NULL;
  }
  BStringNode *current = self->head;
  while (current)
  {

    int contains = PySet_Contains(seen_set, current->str);
    if (contains < 0)
    { 
      Py_DECREF(result);
      Py_DECREF(seen_set);
      return NULL;
    }

    if (contains == 0)
    {

      if (PySet_Add(seen_set, current->str) != 0)
      {
        Py_DECREF(result);
        Py_DECREF(seen_set);
        return NULL;
      }

      BString_append(result, current->str);
    }
    current = current->next;
  }
  Py_DECREF(seen_set); 
  return (PyObject *)result;
}

static PyObject *BString_contains(BStringObject *self, PyObject *args, PyObject *kwds)
{
  const char *substring_cstr;
  int case_sensitive = 1; 
  static char *kwlist[] = {"substring", "case_sensitive", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|p", kwlist, &substring_cstr, &case_sensitive))
  {
    return NULL;
  }

  PyObject *substring_obj = PyUnicode_FromString(substring_cstr);
  if (!substring_obj)
    return NULL;

  BStringNode *current = self->head;
  while (current)
  {
    PyObject *target_str = current->str;
    PyObject *target_substr = substring_obj;
    int found = 0;

    if (!case_sensitive)
    {
      target_str = PyObject_CallMethod(current->str, "lower", NULL);
      target_substr = PyObject_CallMethod(substring_obj, "lower", NULL);
    }

    if (target_str && target_substr)
    {
      if (PyUnicode_Find(target_str, target_substr, 0, PyUnicode_GET_LENGTH(target_str), 1) != -1)
      {
        found = 1;
      }
    }

    if (!case_sensitive)
    {
      Py_XDECREF(target_str);
      Py_XDECREF(target_substr);
    }

    if (found || PyErr_Occurred())
    {
      Py_DECREF(substring_obj);
      if (PyErr_Occurred())
        return NULL;
      Py_RETURN_TRUE;
    }

    current = current->next;
  }
  Py_DECREF(substring_obj);
  Py_RETURN_FALSE;
}


static PyObject *BString_join(BStringObject *self, PyObject *separator)
{
  if (!PyUnicode_Check(separator))
  {
    PyErr_SetString(PyExc_TypeError, "separator must be a string");
    return NULL;
  }

  PyObject *list = PyList_New(self->size);
  if (!list)
    return NULL;

  BStringNode *current = self->head;
  for (Py_ssize_t i = 0; i < self->size; ++i)
  {
    Py_INCREF(current->str);
    PyList_SET_ITEM(list, i, current->str);
    current = current->next;
  }

  PyObject *result = PyUnicode_Join(separator, list);
  Py_DECREF(list); 
  return result;
}

static PyObject *BString_split(PyObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *string_to_split;
  const char *delimiter = NULL;
  static char *kwlist[] = {"string", "delimiter", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|s", kwlist, &string_to_split, &delimiter))
  {
    return NULL;
  }

  PyObject *split_list = PyObject_CallMethod(string_to_split, "split", "z", delimiter);
  if (!split_list)
    return NULL;

  PyObject *args_tuple = PyList_AsTuple(split_list);
  Py_DECREF(split_list); 
  if (!args_tuple)
  {
    return NULL; 
  }

  PyObject *result_bstring = PyObject_CallObject(type, args_tuple);
  Py_DECREF(args_tuple); 
  return result_bstring;
}

static PyObject *_BString_render_as_csv_string(BStringObject *self, const char *delimiter, const char *quotechar, int quoting)
{
  if (strlen(delimiter) != 1 || strlen(quotechar) != 1)
  {
    PyErr_SetString(PyExc_ValueError, "delimiter and quotechar must be single characters");
    return NULL;
  }
  PyObject *processed_fields = PyList_New(0);
  if (!processed_fields)
    return NULL;

  BStringNode *current = self->head;
  while (current)
  {
    int needs_quoting = 0;
    PyObject *field = current->str;

    if (quoting == BSTRING_QUOTE_ALL)
    {
      needs_quoting = 1;
    }
    else if (quoting == BSTRING_QUOTE_MINIMAL)
    {
      PyObject *delim_obj = PyUnicode_FromString(delimiter);
      PyObject *qc_obj = PyUnicode_FromString(quotechar);
      if (delim_obj && qc_obj &&
          (PyUnicode_Find(field, delim_obj, 0, PyUnicode_GET_LENGTH(field), 1) != -1 || PyUnicode_Find(field, qc_obj, 0, PyUnicode_GET_LENGTH(field), 1) != -1))
      {
        needs_quoting = 1;
      }
      Py_XDECREF(delim_obj);
      Py_XDECREF(qc_obj);
    }

    PyObject *final_field;
    if (needs_quoting)
    {
      PyObject *qc_obj = PyUnicode_FromString(quotechar);
      PyObject *doubled_qc_obj = PyUnicode_FromFormat("%s%s", quotechar, quotechar);
      PyObject *escaped_field = PyUnicode_Replace(field, qc_obj, doubled_qc_obj, -1);

      if (escaped_field)
      {
        final_field = PyUnicode_FromFormat("%s%s%s", quotechar, PyUnicode_AsUTF8(escaped_field), quotechar);
        Py_DECREF(escaped_field);
      }
      else
      {
        final_field = NULL; 
      }
      Py_DECREF(qc_obj);
      Py_DECREF(doubled_qc_obj);
    }
    else
    {
      Py_INCREF(field);
      final_field = field;
    }

    if (!final_field || PyList_Append(processed_fields, final_field) != 0)
    {
      Py_XDECREF(final_field);
      Py_DECREF(processed_fields);
      return NULL;
    }
    Py_DECREF(final_field);
    current = current->next;
  }

  PyObject *delimiter_obj = PyUnicode_FromString(delimiter);
  PyObject *csv_string = PyUnicode_Join(delimiter_obj, processed_fields);
  Py_DECREF(delimiter_obj);
  Py_DECREF(processed_fields);
  return csv_string;
}

static int BString_write_csv_row(FILE *file, PyObject *bstring_obj, PyObject *kwds)
{
  if (!PyObject_IsInstance(bstring_obj, (PyObject *)&BStringType))
  {
    PyErr_SetString(PyExc_TypeError, "All items in data list and header must be BString objects.");
    return -1;
  }

  PyObject *call_args = PyTuple_New(0);
  PyObject *csv_row_str = PyObject_Call(bstring_obj, call_args, kwds);
  Py_DECREF(call_args); 
  if (!csv_row_str)
  {
    return -1; 
  }

  if (!PyUnicode_Check(csv_row_str))
  {
    Py_DECREF(csv_row_str);
    PyErr_SetString(PyExc_TypeError, "Internal row formatter did not return a string.");
    return -1;
  }

  fprintf(file, "%s\n", PyUnicode_AsUTF8(csv_row_str));
  Py_DECREF(csv_row_str);
  return 0; 
}

static PyObject *BString_to_csv(PyObject *type, PyObject *args, PyObject *kwds)
{
  const char *filepath;
  PyObject *data_list = NULL;
  PyObject *header_obj = NULL;

  if (!PyArg_ParseTuple(args, "s", &filepath))
  {
    PyErr_SetString(PyExc_TypeError, "to_csv() requires a filepath as the first positional argument.");
    return NULL;
  }

  if (kwds)
  {
    data_list = PyDict_GetItemString(kwds, "data");
  }

  if (!data_list)
  {
    PyErr_SetString(PyExc_TypeError, "to_csv() missing required keyword-only argument: 'data'");
    return NULL;
  }

  if (!PyList_Check(data_list))
  {
    PyErr_SetString(PyExc_TypeError, "The 'data' argument must be a list of BString objects.");
    return NULL;
  }

  if (kwds)
  {
    header_obj = PyDict_GetItemString(kwds, "header");
  }

  PyObject *call_kwargs = PyDict_New();
  if (!call_kwargs)
    return NULL;

  if (kwds && PyDict_Check(kwds))
  {
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(kwds, &pos, &key, &value))
    {
      const char *key_str = PyUnicode_AsUTF8(key);
      if (strcmp(key_str, "data") != 0 && strcmp(key_str, "header") != 0)
      {
        PyDict_SetItem(call_kwargs, key, value);
      }
    }
  }

  PyDict_SetItemString(call_kwargs, "container", PyUnicode_FromString("csv"));
  FILE *file = fopen(filepath, "wb");
  if (!file)
  {
    PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
    Py_DECREF(call_kwargs);
    return NULL;
  }

  PyObject *empty_tuple = PyTuple_New(0);

  if (header_obj && header_obj != Py_None)
  {
    PyObject *header_row_str = PyObject_Call(header_obj, empty_tuple, call_kwargs);
    if (!header_row_str)
    {
      fclose(file);
      Py_DECREF(call_kwargs);
      Py_DECREF(empty_tuple);
      return NULL;
    }

    fprintf(file, "%s\n", PyUnicode_AsUTF8(header_row_str));
    Py_DECREF(header_row_str);
  }

  Py_ssize_t num_rows = PyList_Size(data_list);
  for (Py_ssize_t i = 0; i < num_rows; ++i)
  {
    PyObject *row_obj = PyList_GET_ITEM(data_list, i);
    PyObject *data_row_str = PyObject_Call(row_obj, empty_tuple, call_kwargs);
    if (!data_row_str)
    {
      fclose(file);
      Py_DECREF(call_kwargs);
      Py_DECREF(empty_tuple);
      return NULL;
    }

    fprintf(file, "%s\n", PyUnicode_AsUTF8(data_row_str));
    Py_DECREF(data_row_str);
  }

  fclose(file);
  Py_DECREF(call_kwargs);
  Py_DECREF(empty_tuple);
  Py_RETURN_NONE;
}

static PyObject *BString_from_csv(PyObject *type, PyObject *args, PyObject *kwds)
{
  const char *filepath;
  int header = 1;
  static char *kwlist[] = {"filepath", "header", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|p", kwlist, &filepath, &header))
  {
    return NULL;
  }

  PyObject *builtins = NULL, *open_func = NULL, *file_handle = NULL;
  PyObject *csv_module = NULL, *reader_func = NULL, *reader_obj = NULL, *reader_iter = NULL;
  PyObject *header_bstring = NULL, *data_rows_list = NULL, *return_value = NULL;

  builtins = PyImport_ImportModule("builtins");
  if (!builtins)
    goto cleanup;

  open_func = PyObject_GetAttrString(builtins, "open");

  if (!open_func)
    goto cleanup;

  PyObject *open_args = Py_BuildValue("(ss)", filepath, "r");
  PyObject *open_kwargs = Py_BuildValue("{s:s}", "encoding", "utf-8");
  file_handle = PyObject_Call(open_func, open_args, open_kwargs);
  Py_DECREF(open_args);
  Py_DECREF(open_kwargs);

  if (!file_handle)
    goto cleanup;

  csv_module = PyImport_ImportModule("csv");
  if (!csv_module)
    goto cleanup;

  reader_func = PyObject_GetAttrString(csv_module, "reader");

  if (!reader_func)
    goto cleanup;

  reader_obj = PyObject_CallFunctionObjArgs(reader_func, file_handle, NULL);

  if (!reader_obj)
    goto cleanup;

  reader_iter = PyObject_GetIter(reader_obj);

  if (!reader_iter)
    goto cleanup;

  if (header)
  {
    PyObject *row_list = PyIter_Next(reader_iter);
    if (row_list)
    {
      PyObject *row_tuple = PyList_AsTuple(row_list);
      header_bstring = PyObject_CallObject(type, row_tuple);
      Py_DECREF(row_tuple);
      Py_DECREF(row_list);
      if (!header_bstring)
        goto cleanup;
    }
    else if (PyErr_Occurred())
    {
      goto cleanup;
    }
    else
    {
      header_bstring = PyObject_CallObject(type, PyTuple_New(0));
      if (!header_bstring)
        goto cleanup;
    }
  }

  data_rows_list = PyList_New(0);
  if (!data_rows_list)
    goto cleanup;

  PyObject *row_list;
  while ((row_list = PyIter_Next(reader_iter)))
  {
    PyObject *row_tuple = PyList_AsTuple(row_list);
    PyObject *row_bstring = PyObject_CallObject(type, row_tuple);
    Py_DECREF(row_tuple);
    Py_DECREF(row_list);

    if (!row_bstring)
      goto cleanup;

    if (PyList_Append(data_rows_list, row_bstring) < 0)
    {
      Py_DECREF(row_bstring);
      goto cleanup;
    }
    Py_DECREF(row_bstring);
  }

  if (header)
  {
    return_value = PyTuple_Pack(2, header_bstring, data_rows_list);
    header_bstring = NULL; 
    data_rows_list = NULL; 
  }
  else
  {
    return_value = data_rows_list;
    data_rows_list = NULL; 
  }

cleanup:

  Py_XDECREF(reader_iter);
  Py_XDECREF(reader_obj);
  Py_XDECREF(reader_func);
  Py_XDECREF(csv_module);

  if (file_handle)
  {
    PyObject_CallMethod(file_handle, "close", NULL);
    Py_XDECREF(file_handle);
  }

  Py_XDECREF(builtins);
  Py_XDECREF(open_func);
  Py_XDECREF(header_bstring);
  Py_XDECREF(data_rows_list);

  if (PyErr_Occurred())
  {
    Py_XDECREF(return_value);
    return NULL;
  }
  return return_value;
}

static PyObject *BString_to_file(BStringObject *self, PyObject *args)
{
  const char *filepath;
  if (!PyArg_ParseTuple(args, "s", &filepath))
  {
    return NULL;
  }
  FILE *file = fopen(filepath, "w");
  if (!file)
  {
    PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
    return NULL;
  }
  BStringNode *current = self->head;
  while (current)
  {
    fprintf(file, "%s\n", PyUnicode_AsUTF8(current->str));
    current = current->next;
  }
  fclose(file);
  Py_RETURN_NONE;
}

static PyObject *BString_from_file(PyObject *type, PyObject *args)
{
  const char *filepath;
  if (!PyArg_ParseTuple(args, "s", &filepath))
  {
    return NULL;
  }
  FILE *file = fopen(filepath, "r");
  if (!file)
  {
    PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
    return NULL;
  }

  BStringObject *new_bstring = (BStringObject *)((PyTypeObject *)type)->tp_new((PyTypeObject *)type, NULL, NULL);
  if (!new_bstring)
  {
    fclose(file);
    return NULL;
  }

  char buffer[4096]; 
  while (fgets(buffer, sizeof(buffer), file))
  {

    buffer[strcspn(buffer, "\r\n")] = 0;

    PyObject *line_str = PyUnicode_FromString(buffer);
    if (!line_str)
    {
      Py_DECREF(new_bstring);
      fclose(file);
      return NULL;
    }

    BStringNode *new_node = new_BStringNode(line_str);
    Py_DECREF(line_str); 
    if (!new_node)
    {
      Py_DECREF(new_bstring);
      fclose(file);
      return NULL;
    }
    if (new_bstring->tail == NULL)
    {
      new_bstring->head = new_bstring->tail = new_bstring->current = new_node;
    }
    else
    {
      new_bstring->tail->next = new_node;
      new_node->prev = new_bstring->tail;
      new_bstring->tail = new_node;
    }
    new_bstring->size++;
  }
  fclose(file);
  return (PyObject *)new_bstring;
}

static PyObject *BString_transform_chars(BStringObject *self, PyObject *args, PyObject *kwds)
{
  const char *characters;
  const char *mode = "remove";
  int inplace = 0; 
  static char *kwlist[] = {"characters", "mode", "inplace", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|sp", kwlist, &characters, &mode, &inplace))
  {
    return NULL;
  }

  int is_remove_mode = (strcmp(mode, "remove") == 0);
  if (!is_remove_mode && strcmp(mode, "keep") != 0)
  {
    PyErr_SetString(PyExc_ValueError, "mode must be either 'remove' or 'keep'");
    return NULL;
  }

  PyObject *char_set = PySet_New(PyUnicode_FromString(characters));
  if (!char_set)
    return NULL;
  BStringObject *target_bstring = self;
  BStringObject *result_bstring = NULL;
  BStringNode *target_node = NULL;

  if (!inplace)
  {
    result_bstring = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result_bstring)
    {
      Py_DECREF(char_set);
      return NULL;
    }
    target_bstring = result_bstring;
  }

  BStringNode *source_node = self->head;
  while (source_node)
  {
    PyObject *source_str = source_node->str;
    PyObject *parts_list = PyList_New(0); 
    if (!parts_list)
    {
      Py_DECREF(char_set);
      Py_XDECREF(result_bstring);
      return NULL;
    }
    Py_ssize_t len = PyUnicode_GET_LENGTH(source_str);
    for (Py_ssize_t i = 0; i < len; ++i)
    {

      PyObject *single_char = PyUnicode_FromOrdinal(PyUnicode_READ_CHAR(source_str, i));
      if (!single_char)
      { /* handle error */
      }
      int contains = PySet_Contains(char_set, single_char);
      int keep_char = (is_remove_mode && !contains) || (!is_remove_mode && contains);
      if (keep_char)
      {
        PyList_Append(parts_list, single_char);
      }
      Py_DECREF(single_char);
    }
    PyObject *empty_str = PyUnicode_FromString("");
    PyObject *transformed_str = PyUnicode_Join(empty_str, parts_list);
    Py_DECREF(empty_str);
    Py_DECREF(parts_list);
    if (!transformed_str)
    { /* handle error */
    }
    if (inplace)
    {
      Py_DECREF(source_node->str);        
      source_node->str = transformed_str; 
    }
    else
    {

      BString_append((BStringObject *)target_bstring, transformed_str);
      Py_DECREF(transformed_str);
    }
    source_node = source_node->next;
  }
  Py_DECREF(char_set);
  if (inplace)
  {
    Py_RETURN_NONE;
  }
  else
  {
    return (PyObject *)result_bstring;
  }
}

static PyObject *BString_remove_node(BStringObject *self, BStringNode *node_to_remove)
{
  if (!node_to_remove)
    return NULL; 
  BStringNode *node_before = node_to_remove->prev;
  BStringNode *node_after = node_to_remove->next;

  if (node_before)
  {
    node_before->next = node_after;
  }
  else
  {

    self->head = node_after;
  }
  if (node_after)
  {
    node_after->prev = node_before;
  }
  else
  {

    self->tail = node_before;
  }

  if (self->current == node_to_remove)
  {
    self->current = self->head;
  }
  self->size--;
  PyObject *returned_str = node_to_remove->str; 
  PyMem_Free(node_to_remove);                   
  return returned_str; 
}

static PyObject *BString_append(BStringObject *self, PyObject *obj)
{
  if (!PyUnicode_Check(obj))
  {
    PyErr_SetString(PyExc_TypeError, "can only append a string");
    return NULL;
  }
  BStringNode *new_node = new_BStringNode(obj);
  if (!new_node)
    return NULL;
  if (self->tail == NULL)
  { 
    self->head = self->tail = self->current = new_node;
  }
  else
  {
    self->tail->next = new_node;
    new_node->prev = self->tail;
    self->tail = new_node;
  }
  self->size++;
  Py_RETURN_NONE;
}

static PyObject *BString_insert(BStringObject *self, PyObject *args)
{
  Py_ssize_t index;
  PyObject *obj;
  if (!PyArg_ParseTuple(args, "nO", &index, &obj))
  {
    return NULL;
  }
  if (!PyUnicode_Check(obj))
  {
    PyErr_SetString(PyExc_TypeError, "can only insert a string");
    return NULL;
  }

  if (index >= self->size)
  {
    return BString_append(self, obj);
  }
  if (index < 0)
  {
    index += self->size;
    if (index < 0)
      index = 0;
  }

  BStringNode *node_at_index = self->head;
  for (Py_ssize_t i = 0; i < index; ++i)
  {
    node_at_index = node_at_index->next;
  }
  BStringNode *new_node = new_BStringNode(obj);
  if (!new_node)
    return NULL;
  BStringNode *node_before = node_at_index->prev;

  new_node->next = node_at_index;
  new_node->prev = node_before;
  node_at_index->prev = new_node;
  if (node_before)
  {
    node_before->next = new_node;
  }
  else
  {

    self->head = new_node;
  }
  self->size++;
  Py_RETURN_NONE;
}

static PyObject *BString_pop(BStringObject *self, PyObject *args)
{
  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index))
  {
    return NULL;
  }
  if (self->size == 0)
  {
    PyErr_SetString(PyExc_IndexError, "pop from empty BString");
    return NULL;
  }

  if (index < 0)
  {
    index += self->size;
  }

  if (index < 0 || index >= self->size)
  {
    PyErr_SetString(PyExc_IndexError, "pop index out of range");
    return NULL;
  }

  BStringNode *node_to_pop = self->head;
  for (Py_ssize_t i = 0; i < index; ++i)
  {
    node_to_pop = node_to_pop->next;
  }
  return BString_remove_node(self, node_to_pop);
}

static PyObject *BString_remove(BStringObject *self, PyObject *value)
{
  if (!PyUnicode_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "argument must be a string");
    return NULL;
  }
  BStringNode *current = self->head;
  while (current)
  {
    int cmp_res = PyObject_RichCompareBool(current->str, value, Py_EQ);
    if (cmp_res > 0)
    { 
      PyObject *removed_str = BString_remove_node(self, current);
      Py_DECREF(removed_str); 
      Py_RETURN_NONE;
    }
    if (cmp_res < 0)
    { 
      return NULL;
    }
    current = current->next;
  }
  PyErr_SetString(PyExc_ValueError, "BString.remove(x): x not in BString");
  return NULL;
}

static PyMethodDef BString_methods[] =
{
    {"map", (PyCFunction)BString_map, METH_VARARGS, "Apply a string method to all elements..."},
    {"filter", (PyCFunction)BString_filter, METH_VARARGS, "Filter elements using a string method..."},
    {"extend", (PyCFunction)BString_extend, METH_O, "Extend with items from a sequence."},
    {"append", (PyCFunction)BString_append, METH_O, "Append string to the end of the BString."},
    {"insert", (PyCFunction)BString_insert, METH_VARARGS, "Insert string before index."},
    {"pop", (PyCFunction)BString_pop, METH_VARARGS, "Remove and return string at index (default last)."},
    {"remove", (PyCFunction)BString_remove, METH_O, "Remove first occurrence of a string."},
    {"transform_chars", (PyCFunction)BString_transform_chars, METH_VARARGS | METH_KEYWORDS, "Remove or keep a selected set of characters in each string."},
    {"to_file", (PyCFunction)BString_to_file, METH_VARARGS, "Save the BString contents to a file, one string per line."},
    {"from_file", (PyCFunction)BString_from_file, METH_VARARGS | METH_CLASS, "Create a new BString from a line-delimited text file."},
    {"from_csv", (PyCFunction)BString_from_csv, METH_VARARGS | METH_KEYWORDS | METH_CLASS, "Create BString rows from a CSV file."},
    {"to_csv", (PyCFunction)BString_to_csv, METH_VARARGS | METH_KEYWORDS | METH_CLASS, "Save a list of BString rows to a CSV file."},
    {"move_next", (PyCFunction)BString_move_next, METH_NOARGS, "Move cursor to the next item. Returns False if at the end."},
    {"move_prev", (PyCFunction)BString_move_prev, METH_NOARGS, "Move cursor to the previous item. Returns False if at the beginning."},
    {"move_to_head", (PyCFunction)BString_move_to_head, METH_NOARGS, "Reset the cursor to the first item."},
    {"move_to_tail", (PyCFunction)BString_move_to_tail, METH_NOARGS, "Move the cursor to the last item."},
    {"join", (PyCFunction)BString_join, METH_O, "Join elements into a single string with a separator."},
    {"split", (PyCFunction)BString_split, METH_VARARGS | METH_KEYWORDS | METH_CLASS, "Create a BString by splitting a string."},
    {"contains", (PyCFunction)BString_contains, METH_VARARGS | METH_KEYWORDS, "Check if any string in the BString contains a substring."},
    {"unique", (PyCFunction)BString_unique, METH_NOARGS, "Return a new BString with duplicate strings removed."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};


static PyObject *BString_map(BStringObject *self, PyObject *args)
{
  PyObject *method_name_obj;
  PyObject *method_args_tuple = NULL;
  Py_ssize_t num_args = PyTuple_Size(args);
  if (num_args < 1)
  {
    PyErr_SetString(PyExc_TypeError, "map() requires at least one argument (the method name)");
    return NULL;
  }

  method_name_obj = PyTuple_GET_ITEM(args, 0);
  if (!PyUnicode_Check(method_name_obj))
  {
    PyErr_SetString(PyExc_TypeError, "first argument to map() must be a string method name");
    return NULL;
  }

  method_args_tuple = PyTuple_New(num_args - 1);
  if (!method_args_tuple)
  {
    return NULL; 
  }
  for (Py_ssize_t i = 1; i < num_args; ++i)
  {
    PyObject *item = PyTuple_GET_ITEM(args, i);
    Py_INCREF(item); 
    PyTuple_SET_ITEM(method_args_tuple, i - 1, item);
  }

  BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
  if (!result)
  {
    Py_DECREF(method_args_tuple);
    return NULL;
  }
  BStringNode *current = self->head;
  int error_occurred = 0;
  while (current)
  {

    PyObject *method = PyObject_GetAttr(current->str, method_name_obj);
    if (!method)
    {
      error_occurred = 1;
      break; 
    }

    PyObject *call_result = PyObject_CallObject(method, method_args_tuple);
    Py_DECREF(method); 
    if (!call_result)
    {
      error_occurred = 1;
      break; 
    }

    BStringNode *new_node = new_BStringNode(call_result);
    Py_DECREF(call_result); 
    if (!new_node)
    {
      error_occurred = 1;
      break; 
    }
    if (!result->head)
    {
      result->head = result->tail = result->current = new_node;
    }
    else
    {
      result->tail->next = new_node;
      new_node->prev = result->tail;
      result->tail = new_node;
    }
    result->size++;
    current = current->next;
  }
  Py_DECREF(method_args_tuple);
  if (error_occurred)
  {
    Py_DECREF(result); 
    return NULL;
  }
  return (PyObject *)result;
}

static PyObject *BString_filter(BStringObject *self, PyObject *args)
{
  PyObject *filter_condition;
  Py_ssize_t num_args = PyTuple_Size(args);
  if (num_args < 1)
  {
    PyErr_SetString(PyExc_TypeError, "filter() requires at least one argument (a method name or a callable)");
    return NULL;
  }

  filter_condition = PyTuple_GET_ITEM(args, 0);

  BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
  if (!result)
    return NULL;
  BStringNode *current = self->head;
  int error_occurred = 0;

  if (PyCallable_Check(filter_condition))
  {
    if (num_args > 1)
    {
      PyErr_SetString(PyExc_TypeError, "when using a callable, filter() takes exactly 1 argument");
      Py_DECREF(result);
      return NULL;
    }
    while (current)
    {

      PyObject *call_result = PyObject_CallFunctionObjArgs(filter_condition, current->str, NULL);
      if (!call_result)
      {
        error_occurred = 1;
        break;
      }
      int is_true = PyObject_IsTrue(call_result);
      Py_DECREF(call_result);
      if (is_true < 0)
      { 
        error_occurred = 1;
        break;
      }
      if (is_true)
      {

        BStringNode *new_node = new_BStringNode(current->str);
        if (!new_node)
        {
          error_occurred = 1;
          break;
        }
        if (!result->head)
        {
          result->head = result->tail = result->current = new_node;
        }
        else
        {
          result->tail->next = new_node;
          new_node->prev = result->tail;
          result->tail = new_node;
        }
        result->size++;
      }
      current = current->next;
    }
  }

  else if (PyUnicode_Check(filter_condition))
  {

    PyObject *method_args_tuple = PyTuple_New(num_args - 1);
    if (!method_args_tuple)
    {
      Py_DECREF(result);
      return NULL;
    }
    for (Py_ssize_t i = 1; i < num_args; ++i)
    {
      PyObject *item = PyTuple_GET_ITEM(args, i);
      Py_INCREF(item);
      PyTuple_SET_ITEM(method_args_tuple, i - 1, item);
    }
    while (current)
    {
      const char *method_name = PyUnicode_AsUTF8(filter_condition);

      PyObject *call_result = PyObject_CallMethod(current->str, method_name, "O", method_args_tuple);
      if (!call_result)
      {
        error_occurred = 1;
        break;
      }
      int is_true = PyObject_IsTrue(call_result);
      Py_DECREF(call_result);
      if (is_true < 0)
      {
        error_occurred = 1;
        break;
      }
      if (is_true)
      {

        BStringNode *new_node = new_BStringNode(current->str);
        if (!new_node)
        {
          error_occurred = 1;
          break;
        }
        if (!result->head)
        {
          result->head = result->tail = result->current = new_node;
        }
        else
        {
          result->tail->next = new_node;
          new_node->prev = result->tail;
          result->tail = new_node;
        }
        result->size++;
      }
      current = current->next;
    }
    Py_DECREF(method_args_tuple);
  }

  else
  {
    PyErr_SetString(PyExc_TypeError, "first argument to filter() must be a string or a callable");
    Py_DECREF(result);
    return NULL;
  }
  if (error_occurred)
  {
    Py_DECREF(result);
    return NULL;
  }
  return (PyObject *)result;
}

static PyObject *BString_repeat(BStringObject *self, Py_ssize_t n)
{

  BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
  if (!result)
  {
    return NULL;
  }

  if (n <= 0)
  {
    return (PyObject *)result;
  }

  for (Py_ssize_t i = 0; i < n; ++i)
  {
    BStringNode *current = self->head;
    while (current)
    {
      BStringNode *new_node = new_BStringNode(current->str);
      if (!new_node)
      {
        return NULL; 
      }
      if (!result->head)
      {
        result->head = result->tail = result->current = new_node;
      }
      else
      {
        result->tail->next = new_node;
        new_node->prev = result->tail;
        result->tail = new_node;
      }
      result->size++;
      current = current->next;
    }
  }
  return (PyObject *)result;
}

static int BString_append_nodes_from_source(BStringObject *result, BStringObject *source)
{
  BStringNode *current = source->head;
  while (current)
  {
    BStringNode *new_node = new_BStringNode(current->str);
    if (!new_node)
      return -1; 
    if (!result->head)
    {
      result->head = result->tail = result->current = new_node;
    }
    else
    {
      result->tail->next = new_node;
      new_node->prev = result->tail;
      result->tail = new_node;
    }
    result->size++;
    current = current->next;
  }
  return 0; 
}

static PyObject *BString_concat(BStringObject *self, PyObject *other)
{
  if (!PyObject_IsInstance((PyObject *)other, (PyObject *)&BStringType))
  {
    PyErr_SetString(PyExc_TypeError, "can only concatenate BString to BString");
    return NULL;
  }
  BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
  if (!result)
    return NULL;
  if (BString_append_nodes_from_source(result, self) != 0)
  {
    Py_DECREF(result);
    return NULL;
  }
  if (BString_append_nodes_from_source(result, (BStringObject *)other) != 0)
  {
    Py_DECREF(result);
    return NULL;
  }
  return (PyObject *)result;
}

static BStringNode *new_BStringNode(PyObject *str_obj)
{
  BStringNode *node = PyMem_Malloc(sizeof(BStringNode));
  if (!node)
  {
    PyErr_NoMemory();
    return NULL;
  }
  Py_INCREF(str_obj);
  node->str = str_obj;
  node->next = NULL;
  node->prev = NULL;
  return node;
}

static void BStringIter_dealloc(BStringIterObject *iter)
{

  PyObject_Del(iter);
}

static PyObject *BStringIter_iternext(BStringIterObject *iter)
{
  if (iter->current_node)
  {
    PyObject *str_obj = iter->current_node->str;
    Py_INCREF(str_obj); 
    iter->current_node = iter->current_node->next;
    return str_obj;
  }
  else
  {

    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
  }
}

PyTypeObject BStringIter_Type =
{
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "BStringIter",
    .tp_basicsize = sizeof(BStringIterObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)BStringIter_dealloc,
    .tp_iter = PyObject_SelfIter, 
    .tp_iternext = (iternextfunc)BStringIter_iternext,
};

static PyObject *BString_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  BStringObject *self;
  self = (BStringObject *)type->tp_alloc(type, 0);
  if (self != NULL)
  {
    self->head = NULL;
    self->tail = NULL;
    self->current = NULL;
    self->size = 0;
    self->weakreflist = NULL;
  }
  return (PyObject *)self;
}

static int BString_init(BStringObject *self, PyObject *args, PyObject *kwds)
{
  for (int i = 0; i < PyTuple_GET_SIZE(args); ++i)
  {
    PyObject *item = PyTuple_GET_ITEM(args, i);
    if (!PyUnicode_Check(item))
    {
      PyErr_SetString(PyExc_TypeError, "All arguments must be strings.");
      return -1;
    }
    BStringNode *node = new_BStringNode(item);
    if (!node)
      return -1;
    if (!self->head)
    { 
      self->head = self->tail = self->current = node;
    }
    else
    {
      self->tail->next = node;
      node->prev = self->tail;
      self->tail = node;
    }
    self->size++;
  }
  return 0;
}

static void BString_dealloc(BStringObject *self)
{
  if (self->weakreflist != NULL)
  {
    PyObject_ClearWeakRefs((PyObject *)self);
  }
  BStringNode *current = self->head;
  while (current)
  {
    BStringNode *next = current->next;
    Py_DECREF(current->str); 
    PyMem_Free(current);     
    current = next;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *BString_repr(BStringObject *self)
{
  PyObject *list = PyList_New(0);
  if (!list)
    return NULL;
  BStringNode *current = self->head;
  while (current)
  {
    if (PyList_Append(list, current->str) != 0)
    {
      Py_DECREF(list);
      return NULL;
    }
    current = current->next;
  }
  PyObject *repr = PyObject_Repr(list);
  Py_DECREF(list);
  return repr;
}

static PyObject *BString_call(BStringObject *self, PyObject *args, PyObject *kwds)
{

  char *container = "list";
  PyObject *keys = NULL;
  char *delimiter = ",";
  char *quotechar = "\"";
  int quoting = BSTRING_QUOTE_MINIMAL;
  static char *kwlist[] = {"container", "keys", "delimiter", "quotechar", "quoting", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOssi", kwlist, &container, &keys, &delimiter, &quotechar, &quoting))
  {
    return NULL;
  }

  if (strcmp(container, "list") == 0)
  {
    PyObject *list = PyList_New(self->size);
    if (!list)
      return NULL;
    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < self->size; ++i)
    {
      Py_INCREF(current->str);
      PyList_SET_ITEM(list, i, current->str);
      current = current->next;
    }
    return list;
  }

  if (strcmp(container, "tuple") == 0)
  {
    PyObject *tuple = PyTuple_New(self->size);
    if (!tuple)
      return NULL;
    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < self->size; ++i)
    {
      Py_INCREF(current->str);
      PyTuple_SET_ITEM(tuple, i, current->str);
      current = current->next;
    }
    return tuple;
  }

  if (strcmp(container, "dict") == 0)
  {
    if (!keys || !PyList_Check(keys) || PyList_Size(keys) != self->size)
    {
      PyErr_SetString(PyExc_ValueError, "'keys' must be a list of the same length as the BString.");
      return NULL;
    }
    PyObject *dict = PyDict_New();
    if (!dict)
      return NULL;
    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < self->size; ++i)
    {
      if (PyDict_SetItem(dict, PyList_GET_ITEM(keys, i), current->str) != 0)
      {
        Py_DECREF(dict);
        return NULL;
      }
      current = current->next;
    }
    return dict;
  }

  if (strcmp(container, "csv") == 0)
  {
    return _BString_render_as_csv_string(self, delimiter, quotechar, quoting);
  }

  if (strcmp(container, "json") == 0)
  {
    PyObject *data_to_dump = NULL;
    if (keys == NULL)
    {                                                          
      data_to_dump = BString_call(self, PyTuple_New(0), NULL); 
    }
    else
    { 
      PyObject *kwargs = PyDict_New();
      PyDict_SetItemString(kwargs, "keys", keys);
      data_to_dump = BString_call(self, PyTuple_New(0), kwargs); 
      Py_DECREF(kwargs);
    }
    if (!data_to_dump)
      return NULL;

    PyObject *json_module = PyImport_ImportModule("json");
    if (!json_module)
    {
      Py_DECREF(data_to_dump);
      return NULL;
    }
    PyObject *dumps_func = PyObject_GetAttrString(json_module, "dumps");
    PyObject *json_string = PyObject_CallFunctionObjArgs(dumps_func, data_to_dump, NULL);
    Py_DECREF(dumps_func);
    Py_DECREF(json_module);
    Py_DECREF(data_to_dump);
    return json_string;
  }

  PyErr_SetString(PyExc_ValueError, "Invalid container type specified. Choose from 'list', 'tuple', 'dict', 'csv', 'json'.");
  return NULL;
}

static PyObject *BString_extend(BStringObject *self, PyObject *iterable)
{

  PyObject *iterator = PyObject_GetIter(iterable);
  if (!iterator)
  {
    PyErr_SetString(PyExc_TypeError, "extend() argument must be an iterable");
    return NULL;
  }
  PyObject *item;

  while ((item = PyIter_Next(iterator)))
  {

    if (!PyUnicode_Check(item))
    {
      PyErr_SetString(PyExc_TypeError, "BString can only extend with an iterable of strings");
      Py_DECREF(item);
      Py_DECREF(iterator);
      return NULL;
    }

    BStringNode *new_node = new_BStringNode(item);
    if (!new_node)
    {
      Py_DECREF(item);
      Py_DECREF(iterator);
      return NULL;
    }
    if (self->tail == NULL)
    { 
      self->head = self->tail = self->current = new_node;
    }
    else
    {
      self->tail->next = new_node;
      new_node->prev = self->tail;
      self->tail = new_node;
    }
    self->size++;

    Py_DECREF(item);
  }

  Py_DECREF(iterator);

  if (PyErr_Occurred())
  {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *BString_iter(BStringObject *self)
{

  self->current = self->head;

  BStringIterObject *iter = PyObject_New(BStringIterObject, &BStringIter_Type);
  if (!iter)
  {
    return NULL;
  }

  iter->current_node = self->head;
  return (PyObject *)iter;
}

static Py_ssize_t BString_length(BStringObject *self)
{
  return self->size;
}

static PyObject *BString_getitem(BStringObject *self, PyObject *key)
{
  if (PySlice_Check(key))
  {
    Py_ssize_t start, stop, step, slicelength;

    if (PySlice_GetIndicesEx(key, self->size, &start, &stop, &step, &slicelength) < 0)
    {
      return NULL; 
    }

    BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result)
      return NULL;
    if (slicelength <= 0)
    {
      return (PyObject *)result; 
    }

    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < start; ++i)
    {
      current = current->next;
    }

    for (Py_ssize_t i = 0; i < slicelength; ++i)
    {
      BStringNode *new_node = new_BStringNode(current->str);
      if (!new_node)
      {
        BString_dealloc(result);
        return NULL;
      }
      if (!result->head)
      {
        result->head = result->tail = result->current = new_node;
      }
      else
      {
        result->tail->next = new_node;
        new_node->prev = result->tail;
        result->tail = new_node;
      }
      result->size++;

      for (Py_ssize_t j = 0; j < step && current; ++j)
      {
        current = current->next;
      }
    }
    return (PyObject *)result;
  }

  else if (PyLong_Check(key))
  {
    Py_ssize_t i = PyLong_AsSsize_t(key);
    if (i < 0)
    {
      i += self->size;
    }
    if (i < 0 || i >= self->size)
    {
      PyErr_SetString(PyExc_IndexError, "BString index out of range");
      return NULL;
    }
    BStringNode *current = self->head;
    for (Py_ssize_t j = 0; j < i; ++j)
    {
      current = current->next;
    }
    Py_INCREF(current->str);
    return current->str;
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "BString indices must be integers or slices");
    return NULL;
  }
}

static int BString_delete_slice_nodes(BStringObject *self, Py_ssize_t start, Py_ssize_t stop)
{
  if (start >= stop)
  {
    return 0; 
  }

  BStringNode *current = self->head;
  for (Py_ssize_t i = 0; i < start && current; ++i)
  {
    current = current->next;
  }

  BStringNode *node_before_slice = (start > 0 && current) ? current->prev : NULL;
  BStringNode *node_at_stop = current;
  for (Py_ssize_t i = start; i < stop && node_at_stop; ++i)
  {
    node_at_stop = node_at_stop->next;
  }

  for (Py_ssize_t i = start; i < stop; ++i)
  {
    if (!current)
      break; 
    BStringNode *next_node = current->next;
    Py_DECREF(current->str);
    PyMem_Free(current);
    current = next_node;
    self->size--;
  }

  if (node_before_slice)
  {
    node_before_slice->next = node_at_stop;
  }
  else
  {
    self->head = node_at_stop; 
  }
  if (node_at_stop)
  {
    node_at_stop->prev = node_before_slice;
  }
  else
  {
    self->tail = node_before_slice; 
  }
  self->current = self->head; 
  return 0;
}

static int BString_ass_item(BStringObject *self, PyObject *key, PyObject *value)
{

  if (PySlice_Check(key))
  {
    Py_ssize_t start, stop, step, slicelength;
    if (PySlice_GetIndicesEx(key, self->size, &start, &stop, &step, &slicelength) < 0)
    {
      return -1; 
    }

    if (step != 1)
    {
      PyErr_SetString(PyExc_NotImplementedError, "BString slicing with step != 1 is not supported for assignment.");
      return -1;
    }

    if (BString_delete_slice_nodes(self, start, stop) != 0)
    {
      return -1; 
    }

    if (value)
    {

      BStringNode *insertion_point = self->head;
      for (Py_ssize_t i = 0; i < start && insertion_point; ++i)
      {
        insertion_point = insertion_point->next;
      }

      PyObject *iterator = PyObject_GetIter(value);
      if (!iterator)
      {
        PyErr_SetString(PyExc_TypeError, "can only assign an iterable to a slice");
        return -1;
      }

      PyObject *item;
      BStringNode *last_inserted_node = insertion_point ? insertion_point->prev : self->tail;
      while ((item = PyIter_Next(iterator)))
      {
        BStringNode *new_node = new_BStringNode(item);
        Py_DECREF(item);
        if (!new_node)
        {
          Py_DECREF(iterator);
          return -1;
        }
        new_node->prev = last_inserted_node;
        new_node->next = insertion_point;
        if (last_inserted_node)
        {
          last_inserted_node->next = new_node;
        }
        else
        {
          self->head = new_node;
        }
        last_inserted_node = new_node;
        self->size++;
      }
      Py_DECREF(iterator);
      if (insertion_point)
      {
        insertion_point->prev = last_inserted_node;
      }
      else
      {
        self->tail = last_inserted_node;
      }
      if (PyErr_Occurred())
        return -1; 
    }
    return 0; 
  }

  else if (PyLong_Check(key))
  {
    Py_ssize_t i = PyLong_AsSsize_t(key);
    if (i < 0)
    {
      i += self->size;
    }
    if (i < 0 || i >= self->size)
    {
      PyErr_SetString(PyExc_IndexError, "BString assignment index out of range");
      return -1;
    }
    BStringNode *target_node = self->head;
    for (Py_ssize_t j = 0; j < i; ++j)
    {
      target_node = target_node->next;
    }

    if (value == NULL)
    {

      PyObject *removed_str = BString_remove_node(self, target_node);
      Py_DECREF(removed_str);
      return 0;
    }

    if (!PyUnicode_Check(value))
    {
      PyErr_SetString(PyExc_TypeError, "BString items must be strings");
      return -1;
    }
    Py_DECREF(target_node->str); 
    Py_INCREF(value);            
    target_node->str = value;
    return 0;
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "BString indices must be integers or slices");
    return -1;
  }
}

static PySequenceMethods BString_as_sequence =
{
    (lenfunc)BString_length,      
    (binaryfunc)BString_concat,   
    (ssizeargfunc)BString_repeat, 
    0,                            
    0,                            
    0,                            
    0,                            
    0,                            
};

static PyMappingMethods BString_as_mapping =
{
    (lenfunc)BString_length,        
    (binaryfunc)BString_getitem,    
    (objobjargproc)BString_ass_item 
};

static PyObject *BString_get_head(BStringObject *self, void *closure)
{
  if (!self->head)
    Py_RETURN_NONE;
  Py_INCREF(self->head->str);
  return self->head->str;
}

static PyObject *BString_get_tail(BStringObject *self, void *closure)
{
  if (!self->tail)
    Py_RETURN_NONE;
  Py_INCREF(self->tail->str);
  return self->tail->str;
}

static PyObject *BString_get_current(BStringObject *self, void *closure)
{
  if (!self->current)
    Py_RETURN_NONE;
  Py_INCREF(self->current->str);
  return self->current->str;
}

static PyObject *BString_move_next(BStringObject *self, PyObject *Py_UNUSED(args))
{
  if (self->current && self->current->next)
  {
    self->current = self->current->next;
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

static PyObject *BString_move_prev(BStringObject *self, PyObject *Py_UNUSED(args))
{
  if (self->current && self->current->prev)
  {
    self->current = self->current->prev;
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

static PyObject *BString_move_to_head(BStringObject *self, PyObject *Py_UNUSED(args))
{
  self->current = self->head;
  Py_RETURN_NONE;
}

static PyObject *BString_move_to_tail(BStringObject *self, PyObject *Py_UNUSED(args))
{
  self->current = self->tail;
  Py_RETURN_NONE;
}

static PyGetSetDef BString_getsetters[] =
{
    {"head", (getter)BString_get_head, NULL, "The first string in the sequence (read-only).", NULL},
    {"tail", (getter)BString_get_tail, NULL, "The last string in the sequence (read-only).", NULL},
    {"current", (getter)BString_get_current, NULL, "The string at the current cursor position (read-only).", NULL},
    {NULL} /* Sentinel */
};

PyTypeObject BStringType =
{
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "BeautifulString.BString",
    .tp_doc = "A mutable, iterable, multi-string container with slicing.",
    .tp_basicsize = sizeof(BStringObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = BString_new,
    .tp_init = (initproc)BString_init,
    .tp_dealloc = (destructor)BString_dealloc,
    .tp_repr = (reprfunc)BString_repr,
    .tp_as_sequence = &BString_as_sequence,
    .tp_as_mapping = &BString_as_mapping,
    .tp_methods = BString_methods, 
    .tp_call = (ternaryfunc)BString_call,
    .tp_iter = (getiterfunc)BString_iter,
    .tp_getset = BString_getsetters,
    .tp_weaklistoffset = offsetof(BStringObject, weakreflist),
};

static int String_delete_slice_nodes(BStringObject *self, Py_ssize_t start, Py_ssize_t stop)
{
  if (start >= stop)
  {
    return 0; 
  }

  BStringNode *current = self->head;
  for (Py_ssize_t i = 0; i < start && current; ++i)
  {
    current = current->next;
  }

  BStringNode *node_before_slice = (start > 0) ? current->prev : NULL;
  BStringNode *node_at_stop = current;
  for (Py_ssize_t i = start; i < stop && node_at_stop; ++i)
  {
    node_at_stop = node_at_stop->next;
  }

  for (Py_ssize_t i = start; i < stop; ++i)
  {
    if (!current)
      break;
    BStringNode *next_node = current->next;
    Py_DECREF(current->str);
    PyMem_Free(current);
    current = next_node;
    self->size--;
  }

  if (node_before_slice)
  {
    node_before_slice->next = node_at_stop;
  }
  else
  {

    self->head = node_at_stop;
  }
  if (node_at_stop)
  {
    node_at_stop->prev = node_before_slice;
  }
  else
  {

    self->tail = node_before_slice;
  }

  self->current = self->head;
  return 0;
}