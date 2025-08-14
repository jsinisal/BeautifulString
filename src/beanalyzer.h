/*

This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com

*/

#ifndef BEANALYZER_H
#define BEANALYZER_H

#include <Python.h>
#include "structmember.h"

// The BeautifulAnalyzer Python object structure with caching fields
typedef struct {
    PyObject_HEAD
    PyObject *text_blob;
    Py_ssize_t char_count;
    Py_ssize_t word_count;
    Py_ssize_t sentence_count;
    Py_ssize_t unique_word_count;
    Py_ssize_t total_syllables;
} BeautifulAnalyzerObject;

// Define token types
typedef enum {
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_STRING
} TokenType;

static PyObject *BeautifulAnalyzer_get_word_count(BeautifulAnalyzerObject *self, void *closure);
static PyObject *BeautifulAnalyzer_get_sentence_count(BeautifulAnalyzerObject *self, void *closure);

// Forward declaration of the type object
extern PyTypeObject BeautifulAnalyzerType;

#endif
