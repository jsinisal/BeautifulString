#ifndef BEANALYZER_H
#define BEANALYZER_H

#include <Python.h>
#include "structmember.h"

// The BeautifulAnalyzer Python object structure with caching fields
typedef struct {
    PyObject_HEAD
    PyObject *text_blob;      // A single PyUnicodeObject holding the full text
    Py_ssize_t char_count;      // Cache for character count (-1 if not calculated)
    Py_ssize_t word_count;      // Cache for word count (-1 if not calculated)
    Py_ssize_t sentence_count;  // Cache for sentence count (-1 if not calculated)
} BeautifulAnalyzerObject;

// Forward declaration of the type object
extern PyTypeObject BeautifulAnalyzerType;

#endif
