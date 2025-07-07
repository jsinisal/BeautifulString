#ifndef BEANALYZER_H
#define BEANALYZER_H

#include <Python.h>
#include "structmember.h"

// The BeautifulAnalyzer Python object structure with caching fields
// In beanalyzer.h

typedef struct {
    PyObject_HEAD
    PyObject *text_blob;
    Py_ssize_t char_count;
    Py_ssize_t word_count;
    Py_ssize_t sentence_count;
    Py_ssize_t unique_word_count;
    Py_ssize_t total_syllables;
} BeautifulAnalyzerObject;

// Forward declaration of the type object
extern PyTypeObject BeautifulAnalyzerType;

#endif
