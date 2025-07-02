//
// Created by sinj on 28/06/2025.
//

#ifndef BSTRING_H
#define BSTRING_H

#include <Python.h>

// Constants for CSV quoting strategies
#define BSTRING_QUOTE_MINIMAL 0
#define BSTRING_QUOTE_ALL 1
#define BSTRING_QUOTE_NONNUMERIC 2 // Placeholder for a potential future feature
#define BSTRING_QUOTE_NONE 3

// Represents a node in the BString's internal doubly linked list.
typedef struct BStringNode {
    PyObject *str;              // A PyUnicodeObject (Python string)
    struct BStringNode *next;
    struct BStringNode *prev;
} BStringNode;

// The structure for the dedicated iterator object
typedef struct {
    PyObject_HEAD
    BStringNode *current_node; // The node the iterator is currently pointing to
} BStringIterObject;


// The main BString Python object structure.
typedef struct {
    PyObject_HEAD
    BStringNode *head;          // Pointer to the first node
    BStringNode *tail;          // Pointer to the last node
    BStringNode *current;       // Pointer to the "active" node for head/tail/next/prev logic
    Py_ssize_t size;            // Number of strings stored
    PyObject *weakreflist;      // For weak references
} BStringObject;

// Forward declaration of the type object, defined in bstring.c
extern PyTypeObject BStringType;

extern PyTypeObject BStringType;
extern PyTypeObject BStringIter_Type;

#endif // BSTRING_H