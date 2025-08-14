/*

This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com

*/


#ifndef BSTRING_H
#define BSTRING_H

#include <Python.h>

// Constants for CSV quoting strategies
#define BSTRING_QUOTE_MINIMAL 0
#define BSTRING_QUOTE_ALL 1
#define BSTRING_QUOTE_NONNUMERIC 2
#define BSTRING_QUOTE_NONE 3

// Forward declare the main struct to solve circular dependencies
typedef struct BStringObject BStringObject;

// A slab of memory for pre-allocating nodes.
typedef struct Slab {
    struct Slab *next;
} Slab;

// Represents a node in the BString's internal doubly linked list.
typedef struct BStringNode {
    PyObject *str;
    struct BStringNode *next;
    struct BStringNode *prev;
} BStringNode;

// The structure for the dedicated iterator object.
typedef struct {
    PyObject_HEAD
    BStringNode *current_node;
    BStringObject *bstring;    // A back-reference to the BString being iterated.
} BStringIterObject;

// The main BString Python object structure.
struct BStringObject {
    PyObject_HEAD
    BStringNode *head;
    BStringNode *tail;
    BStringNode *current;
    Py_ssize_t size;
    PyObject *weakreflist;

    // Members for the custom memory pool
    Slab *slabs;
    BStringNode *free_nodes;
};

static BStringNode *new_BStringNode(PyObject *str_obj);
static void BString_dealloc(BStringObject *self);
static PyObject *BString_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int BString_init(BStringObject *self, PyObject *args, PyObject *kwds);
static PyObject *BString_repr(BStringObject *self);
static PyObject *BString_call(BStringObject *self, PyObject *args, PyObject *kwds);
static PyObject *BString_iter(BStringObject *self);
static PyObject *BString_iternext(BStringObject *self);
static Py_ssize_t BString_length(BStringObject *self);
static PyObject *BString_getitem(BStringObject *self, PyObject *key);
static PyObject *BString_from_node(BStringNode *node);
static PyObject *BString_extend(BStringObject *self, PyObject *args);
static PyObject *BString_append(BStringObject *self, PyObject *obj);
static PyObject *BString_transform_chars(BStringObject *self, PyObject *args, PyObject *kwds);
static PyObject *BString_repeat(BStringObject *self, Py_ssize_t n);
static PyObject *BString_filter(BStringObject *self, PyObject *args);
static PyObject *BString_from_file(PyObject *type, PyObject *args);
static PyObject *BString_to_file(BStringObject *self, PyObject *args);
static PyObject *BString_from_csv(PyObject *type, PyObject *args, PyObject *kwds);
static PyObject *BString_to_csv(PyObject *type, PyObject *args, PyObject *kwds);
static PyObject *BString_render_as_csv_string(BStringObject *self, const char *delimiter, const char *quotechar, int quoting);
static PyObject *BString_map(BStringObject *self, PyObject *args);
static PyObject *BString_get_head(BStringObject *self, void *closure);
static PyObject *BString_get_tail(BStringObject *self, void *closure);
static PyObject *BString_get_next(BStringObject *self, void *closure);
static PyObject *BString_get_prev(BStringObject *self, void *closure);
static PyObject *BString_move_next(BStringObject *self, PyObject *Py_UNUSED(args));
static PyObject *BString_move_prev(BStringObject *self, PyObject *Py_UNUSED(args));
static PyObject *BString_move_to_head(BStringObject *self, PyObject *Py_UNUSED(args));
static PyObject *BString_move_to_tail(BStringObject *self, PyObject *Py_UNUSED(args));


// Forward declarations of the type objects.
extern PyTypeObject BStringType;
extern PyTypeObject BStringIter_Type;

#endif // BSTRING_H