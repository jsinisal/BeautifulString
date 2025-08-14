/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "beanalyzer.h"
#include "bstring.h"
#include <ctype.h>
#include <python.h>

static Py_ssize_t _count_syllables_in_word(PyObject *word_obj)
{
  if (!word_obj || !PyUnicode_Check(word_obj))
    return 0;
  PyObject *lower_word = PyObject_CallMethod(word_obj, "lower", NULL);
  if (!lower_word)
    return 0;
  Py_ssize_t count = 0;
  int in_vowel_group = 0;
  Py_ssize_t len = PyUnicode_GET_LENGTH(lower_word);
  if (len == 0)
  {
    Py_DECREF(lower_word);
    return 0;
  }
  for (Py_ssize_t i = 0; i < len; ++i)
  {
    Py_UCS4 ch = PyUnicode_READ_CHAR(lower_word, i);
    int is_vowel = (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'y');
    if (is_vowel && !in_vowel_group)
    {
      in_vowel_group = 1;
      count++;
    }
    else if (!is_vowel)
    {
      in_vowel_group = 0;
    }
  }

  if (len > 2 && PyUnicode_READ_CHAR(lower_word, len - 1) == 'e')
  {
    Py_UCS4 penultimate = PyUnicode_READ_CHAR(lower_word, len - 2);
    int is_vowel_before = (penultimate == 'a' || penultimate == 'e' || penultimate == 'i' || penultimate == 'o' || penultimate == 'u' || penultimate == 'y');
    if (is_vowel_before && penultimate != 'l')
    { 
      count--;
    }
  }
  if (count == 0)
    count = 1; 
  Py_DECREF(lower_word);
  return count;
}

static PyObject *_BeautifulAnalyzer_get_total_syllables(BeautifulAnalyzerObject *self)
{
  if (!self->text_blob)
    return PyLong_FromLong(0);
  if (self->total_syllables < 0)
  {
    Py_ssize_t syllable_count = 0;
    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    Py_ssize_t word_start = -1;
    for (Py_ssize_t i = 0; i < len; ++i)
    {
      Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
      if (Py_UNICODE_ISALNUM(ch))
      {
        if (word_start < 0)
          word_start = i;
      }
      else if (word_start >= 0)
      {
        PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
        syllable_count += _count_syllables_in_word(word);
        Py_DECREF(word);
        word_start = -1;
      }
    }
    if (word_start >= 0)
    {
      PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
      syllable_count += _count_syllables_in_word(word);
      Py_DECREF(word);
    }
    self->total_syllables = syllable_count;
  }
  return PyLong_FromSsize_t(self->total_syllables);
}

static PyObject *BeautifulAnalyzer_readability_score(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds)
{

  const char *formula = "flesch_kincaid";
  static char *kwlist[] = {"formula", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &formula))
  {
    return NULL;
  }
  PyObject *word_count_obj = BeautifulAnalyzer_get_word_count(self, NULL);
  PyObject *sentence_count_obj = BeautifulAnalyzer_get_sentence_count(self, NULL);
  PyObject *syllable_count_obj = _BeautifulAnalyzer_get_total_syllables(self);
  double words = (double)PyLong_AsSsize_t(word_count_obj);
  double sentences = (double)PyLong_AsSsize_t(sentence_count_obj);
  double syllables = (double)PyLong_AsSsize_t(syllable_count_obj);
  Py_DECREF(word_count_obj);
  Py_DECREF(sentence_count_obj);
  Py_DECREF(syllable_count_obj);
  if (words == 0 || sentences == 0)
  {
    return PyFloat_FromDouble(0.0);
  }
  double score = 0.0;
  if (strcmp(formula, "flesch_kincaid") == 0)
  {

    score = 0.39 * (words / sentences) + 11.8 * (syllables / words) - 15.59;
  }
  else
  {
    PyErr_SetString(PyExc_ValueError, "Unknown formula specified. Use 'flesch_kincaid'.");
    return NULL;
  }
  return PyFloat_FromDouble(score);
}

static PyObject *BeautifulAnalyzer_word_frequency(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds)
{
  Py_ssize_t top_n = -1;
  static char *kwlist[] = {"top_n", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &top_n))
  {
    return NULL;
  }
  if (!self->text_blob)
    Py_RETURN_NONE;
  PyObject *word_counts = PyDict_New();
  if (!word_counts)
    return NULL;

  Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
  Py_ssize_t word_start = -1;
  for (Py_ssize_t i = 0; i < len; ++i)
  {
    Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
    if (Py_UNICODE_ISALNUM(ch))
    {
      if (word_start < 0)
        word_start = i;
    }
    else if (word_start >= 0)
    {
      PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
      PyObject *lower_word = PyObject_CallMethod(word, "lower", NULL);
      PyObject *count = PyDict_GetItem(word_counts, lower_word);
      Py_ssize_t new_count = count ? (PyLong_AsSsize_t(count) + 1) : 1;
      PyObject *new_count_obj = PyLong_FromSsize_t(new_count);
      PyDict_SetItem(word_counts, lower_word, new_count_obj);
      Py_DECREF(word);
      Py_DECREF(lower_word);
      Py_DECREF(new_count_obj);
      word_start = -1;
    }
  }
  if (word_start >= 0)
  { 
    PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
    PyObject *lower_word = PyObject_CallMethod(word, "lower", NULL);
    PyObject *count = PyDict_GetItem(word_counts, lower_word);
    Py_ssize_t new_count = count ? (PyLong_AsSsize_t(count) + 1) : 1;
    PyObject *new_count_obj = PyLong_FromSsize_t(new_count);
    PyDict_SetItem(word_counts, lower_word, new_count_obj);
    Py_DECREF(word);
    Py_DECREF(lower_word);
    Py_DECREF(new_count_obj);
  }

  PyObject *items = PyDict_Items(word_counts);
  Py_DECREF(word_counts);
  if (!items)
    return NULL;

  PyObject *operator_module = PyImport_ImportModule("operator");
  if (!operator_module)
  {
    Py_DECREF(items);
    return NULL;
  }
  PyObject *itemgetter = PyObject_GetAttrString(operator_module, "itemgetter");
  Py_DECREF(operator_module);
  if (!itemgetter)
  {
    Py_DECREF(items);
    return NULL;
  }
  PyObject *sort_key = PyObject_CallFunction(itemgetter, "i", 1);
  if (!sort_key)
  {
    Py_DECREF(itemgetter);
    Py_DECREF(items);
    return NULL;
  }

  PyObject *sort_method = PyObject_GetAttrString(items, "sort");
  if (!sort_method)
  {
    Py_DECREF(sort_key);
    Py_DECREF(itemgetter);
    Py_DECREF(items);
    return NULL;
  }

  PyObject *sort_kwargs = PyDict_New();
  PyDict_SetItemString(sort_kwargs, "key", sort_key);
  PyDict_SetItemString(sort_kwargs, "reverse", Py_True);

  PyObject *sort_result = PyObject_Call(sort_method, PyTuple_New(0), sort_kwargs);

  Py_DECREF(sort_method);
  Py_DECREF(sort_kwargs);
  Py_DECREF(sort_key);
  Py_DECREF(itemgetter);
  Py_XDECREF(sort_result); 

  if (PyErr_Occurred())
  { 
    Py_DECREF(items);
    return NULL;
  }

  if (top_n >= 0 && top_n < PyList_Size(items))
  {
    PyObject *sliced_list = PyList_GetSlice(items, 0, top_n);
    Py_DECREF(items);
    return sliced_list;
  }
  return items;
}

static PyObject *BeautifulAnalyzer_generate_ngrams(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds)
{
  Py_ssize_t n = 2; 
  static char *kwlist[] = {"n", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &n))
  {
    return NULL;
  }
  if (n <= 0)
  {
    PyErr_SetString(PyExc_ValueError, "n must be greater than 0");
    return NULL;
  }
  if (!self->text_blob)
    Py_RETURN_NONE;

  PyObject *words_list = PyList_New(0);
  if (!words_list)
    return NULL;
  Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
  Py_ssize_t word_start = -1;
  for (Py_ssize_t i = 0; i < len; ++i)
  {
    Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
    if (Py_UNICODE_ISALNUM(ch))
    {
      if (word_start < 0)
        word_start = i;
    }
    else if (word_start >= 0)
    {
      PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
      PyList_Append(words_list, word);
      Py_DECREF(word);
      word_start = -1;
    }
  }

  if (word_start >= 0)
  { 
    PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
    PyList_Append(words_list, word);
    Py_DECREF(word);
  }

  PyObject *ngrams_list = PyList_New(0);
  Py_ssize_t num_words = PyList_Size(words_list);

  if (num_words >= n)
  {
    for (Py_ssize_t i = 0; i <= num_words - n; ++i)
    {
      PyObject *slice = PyList_GetSlice(words_list, i, i + n);
      if (!slice)
      {
        Py_DECREF(words_list);
        Py_DECREF(ngrams_list);
        return NULL;
      }

      PyObject *args_tuple = PyList_AsTuple(slice);
      Py_DECREF(slice); 
      if (!args_tuple)
      {
        Py_DECREF(words_list);
        Py_DECREF(ngrams_list);
        return NULL;
      }

      PyObject *ngram_bstring = PyObject_CallObject((PyObject *)&BStringType, args_tuple);
      Py_DECREF(args_tuple); 

      if (!ngram_bstring)
      {
        Py_DECREF(words_list);
        Py_DECREF(ngrams_list);
        return NULL;
      }
      PyList_Append(ngrams_list, ngram_bstring);
      Py_DECREF(ngram_bstring);
    }
  }

  Py_DECREF(words_list); 
  return ngrams_list;
}

static PyObject *BeautifulAnalyzer_get_unique_word_count(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
    Py_RETURN_NONE;

  if (self->unique_word_count >= 0)
  {
    return PyLong_FromSsize_t(self->unique_word_count);
  }

  PyObject *unique_words_set = PySet_New(NULL);
  if (!unique_words_set)
    return NULL;
  Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
  Py_ssize_t word_start = -1;
  for (Py_ssize_t i = 0; i < len; ++i)
  {
    Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
    if (Py_UNICODE_ISALNUM(ch))
    {
      if (word_start == -1)
      {
        word_start = i; 
      }
    }
    else if (word_start != -1)
    {

      PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
      PyObject *lower_word = PyObject_CallMethod(word, "lower", NULL);
      PySet_Add(unique_words_set, lower_word);
      Py_DECREF(word);
      Py_DECREF(lower_word);
      word_start = -1; 
    }
  }

  if (word_start != -1)
  {
    PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
    PyObject *lower_word = PyObject_CallMethod(word, "lower", NULL);
    PySet_Add(unique_words_set, lower_word);
    Py_DECREF(word);
    Py_DECREF(lower_word);
  }

  self->unique_word_count = PySet_Size(unique_words_set);
  Py_DECREF(unique_words_set);

  return PyLong_FromSsize_t(self->unique_word_count);
}

static PyObject *BeautifulAnalyzer_get_average_sentence_length(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
    Py_RETURN_NONE;

  PyObject *word_count_obj = BeautifulAnalyzer_get_word_count(self, closure);
  PyObject *sentence_count_obj = BeautifulAnalyzer_get_sentence_count(self, closure);
  Py_ssize_t words = PyLong_AsSsize_t(word_count_obj);
  Py_ssize_t sentences = PyLong_AsSsize_t(sentence_count_obj);
  Py_DECREF(word_count_obj);
  Py_DECREF(sentence_count_obj);

  if (sentences == 0)
  {
    return PyFloat_FromDouble(0.0);
  }
  double avg = (double)words / (double)sentences;
  return PyFloat_FromDouble(avg);
}

static void BeautifulAnalyzer_dealloc(BeautifulAnalyzerObject *self)
{
  Py_XDECREF(self->text_blob);
  self->text_blob = NULL;
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static int BeautifulAnalyzer_init(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds)
{
  PyObject *text_input = NULL;
  static char *kwlist[] = {"text", NULL}; 

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &text_input))
  {
    return -1;
  }

  Py_XDECREF(self->text_blob);
  self->text_blob = NULL;

  if (PyUnicode_Check(text_input))
  {
    Py_INCREF(text_input);
    self->text_blob = text_input;
  }

  else if (PyObject_IsInstance(text_input, (PyObject *)&BStringType))
  {
    BStringObject *bstring_input = (BStringObject *)text_input;

    PyObject *temp_list = PyList_New(bstring_input->size);
    if (!temp_list)
      return -1;

    BStringNode *current = bstring_input->head;
    for (Py_ssize_t i = 0; i < bstring_input->size; ++i)
    {
      Py_INCREF(current->str);
      PyList_SET_ITEM(temp_list, i, current->str); 
      current = current->next;
    }

    PyObject *separator = PyUnicode_FromString(" ");
    self->text_blob = PyUnicode_Join(separator, temp_list);
    Py_DECREF(separator);
    Py_DECREF(temp_list);
    if (!self->text_blob)
      return -1;
  }

  else if (PyList_Check(text_input) || PyTuple_Check(text_input))
  {
    PyObject *separator = PyUnicode_FromString(" ");
    self->text_blob = PyUnicode_Join(separator, text_input);
    Py_DECREF(separator);
    if (!self->text_blob)
      return -1;
  }

  else
  {
    PyErr_SetString(PyExc_TypeError, "BeautifulAnalyzer input must be a string, BString, list, or tuple.");
    return -1;
  }

  self->char_count = -1;
  self->word_count = -1;
  self->sentence_count = -1;
  self->unique_word_count = -1;
  self->total_syllables = -1;
  return 0;
}

static PyObject *BeautifulAnalyzer_get_text_blob(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
  {
    Py_RETURN_NONE;
  }
  Py_INCREF(self->text_blob);
  return self->text_blob;
}

static PyObject *BeautifulAnalyzer_get_char_count(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
    Py_RETURN_NONE;
  if (self->char_count < 0)
  {
    self->char_count = PyUnicode_GET_LENGTH(self->text_blob);
  }
  return PyLong_FromSsize_t(self->char_count);
}

static PyObject *BeautifulAnalyzer_get_word_count(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
    Py_RETURN_NONE;
  if (self->word_count < 0)
  {
    Py_ssize_t count = 0;
    int in_word = 0;
    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    for (Py_ssize_t i = 0; i < len; ++i)
    {
      Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
      if (Py_UNICODE_ISALNUM(ch))
      {
        if (in_word == 0)
        {
          in_word = 1;
          count++;
        }
      }
      else
      {
        in_word = 0;
      }
    }
    self->word_count = count;
  }
  return PyLong_FromSsize_t(self->word_count);
}

static PyObject *BeautifulAnalyzer_get_sentence_count(BeautifulAnalyzerObject *self, void *closure)
{
  if (!self->text_blob)
    Py_RETURN_NONE;
  if (self->sentence_count < 0)
  {
    Py_ssize_t count = 0;
    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    for (Py_ssize_t i = 0; i < len; ++i)
    {
      Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
      if (ch == '.' || ch == '!' || ch == '?')
      {
        count++;
      }
    }
    if (count == 0 && len > 0)
    {
      count = 1;
    }
    self->sentence_count = count;
  }
  return PyLong_FromSsize_t(self->sentence_count);
}

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

static PyObject *BeautifulAnalyzer_learn_format(PyObject *type, PyObject *args)
{
  PyObject *input_list;
  if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &input_list))
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

  PyObject *result_list = PyList_New(num_cols);
  for (Py_ssize_t i = 0; i < num_cols; ++i)
  {
    const char *type_name = NULL;
    switch (col_types[i])
    {
    case TOKEN_TYPE_INT:
      type_name = "int";
      break;
    case TOKEN_TYPE_FLOAT:
      type_name = "float";
      break;
    case TOKEN_TYPE_STRING:
      type_name = "string";
      break;
    }
    PyList_SET_ITEM(result_list, i, PyUnicode_FromString(type_name));
  }

  PyMem_Free(col_types);
  return result_list;
}


static PyGetSetDef BeautifulAnalyzer_getsetters[] =
{
    {"text_blob", (getter)BeautifulAnalyzer_get_text_blob, NULL, "The full text content being analyzed.", NULL},
    {"char_count", (getter)BeautifulAnalyzer_get_char_count, NULL, "Total number of characters", NULL},
    {"word_count", (getter)BeautifulAnalyzer_get_word_count, NULL, "Total number of words", NULL},
    {"sentence_count", (getter)BeautifulAnalyzer_get_sentence_count, NULL, "Total number of sentences", NULL},
    {"unique_word_count", (getter)BeautifulAnalyzer_get_unique_word_count, NULL, "Total number of unique words", NULL},
    {"average_sentence_length", (getter)BeautifulAnalyzer_get_average_sentence_length, NULL, "Average words per sentence", NULL},
    {NULL}
};

static PyMethodDef BeautifulAnalyzer_methods[] =
{
    {"readability_score", (PyCFunction)BeautifulAnalyzer_readability_score, METH_VARARGS | METH_KEYWORDS, "Calculate a readability score."},
    {"word_frequency", (PyCFunction)BeautifulAnalyzer_word_frequency, METH_VARARGS | METH_KEYWORDS, "Get a list of (word, count) tuples, sorted by frequency."},
    {"generate_ngrams", (PyCFunction)BeautifulAnalyzer_generate_ngrams, METH_VARARGS | METH_KEYWORDS, "Generate n-grams from the text."},
    {"learn_format", (PyCFunction)BeautifulAnalyzer_learn_format, METH_VARARGS | METH_STATIC, "Infer column types from a list of strings."},
    {NULL, NULL, 0, NULL}
};

PyTypeObject BeautifulAnalyzerType =
{
    PyVarObject_HEAD_INIT(NULL, 0) "BeautifulString.BeautifulAnalyzer", /* tp_name */
    sizeof(BeautifulAnalyzerObject),                                    /* tp_basicsize */
    0,                                                                  /* tp_itemsize */
    (destructor)BeautifulAnalyzer_dealloc,                              /* tp_dealloc */
    0,                                                                  /* tp_vectorcall_offset */
    0,                                                                  /* tp_getattr */
    0,                                                                  /* tp_setattr */
    0,                                                                  /* tp_as_async */
    0,                                                                  /* tp_repr */
    0,                                                                  /* tp_as_number */
    0,                                                                  /* tp_as_sequence */
    0,                                                                  /* tp_as_mapping */
    0,                                                                  /* tp_hash */
    0,                                                                  /* tp_call */
    0,                                                                  /* tp_str */
    0,                                                                  /* tp_getattro */
    0,                                                                  /* tp_setattro */
    0,                                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                           /* tp_flags */
    "A class to analyze text and provide statistics.",                  /* tp_doc */
    0,                                                                  /* tp_traverse */
    0,                                                                  /* tp_clear */
    0,                                                                  /* tp_richcompare */
    0,                                                                  /* tp_weaklistoffset */
    0,                                                                  /* tp_iter */
    0,                                                                  /* tp_iternext */
    BeautifulAnalyzer_methods,                                          /* tp_methods */
    0,                                                                  /* tp_members */
    BeautifulAnalyzer_getsetters,                                       /* tp_getset */
    0,                                                                  /* tp_base */
    0,                                                                  /* tp_dict */
    0,                                                                  /* tp_descr_get */
    0,                                                                  /* tp_descr_set */
    0,                                                                  /* tp_dictoffset */
    (initproc)BeautifulAnalyzer_init,                                   /* tp_init */
    0,                                                                  /* tp_alloc */
    PyType_GenericNew,                                                  /* tp_new */
};