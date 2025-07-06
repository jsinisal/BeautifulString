#include "beanalyzer.h"
#include <ctype.h>
#include <python.h>
#include "bstring.h"

// ----------------------------------------------------------------------------
// BeautifulAnalyzer Lifecycle and Methods
// ----------------------------------------------------------------------------

static void BeautifulAnalyzer_dealloc(BeautifulAnalyzerObject *self) {
    Py_XDECREF(self->text_blob);
    self->text_blob = NULL;
    Py_TYPE(self)->tp_free((PyObject *)self);

}

// Replaces the previous minimal __init__ function
static int
BeautifulAnalyzer_init(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds) {
    PyObject *text_input = NULL;
    // We only expect one positional argument, so we clear the keywords.
    static char *kwlist[] = {"text_input", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &text_input)) {
        return -1;
    }

    // Clear any previous text blob that might exist
    Py_XDECREF(self->text_blob);
    self->text_blob = NULL;

    // Case 1: Input is a single string
    if (PyUnicode_Check(text_input)) {
        Py_INCREF(text_input);
        self->text_blob = text_input;
    }
    // Case 2: Input is a BString object (Safe Handling)
    else if (PyObject_IsInstance(text_input, (PyObject*)&BStringType)) {
        BStringObject *bstring_input = (BStringObject*)text_input;
        // Create a temporary Python list to hold the strings
        PyObject *temp_list = PyList_New(bstring_input->size);
        if (!temp_list) return -1;

        // Manually copy strings from our BString into the PyList
        BStringNode *current = bstring_input->head;
        for (Py_ssize_t i = 0; i < bstring_input->size; ++i) {
            Py_INCREF(current->str);
            PyList_SET_ITEM(temp_list, i, current->str); // Steals the reference
            current = current->next;
        }

        // Join the standard PyList, which is a safe operation
        PyObject *separator = PyUnicode_FromString(" ");
        self->text_blob = PyUnicode_Join(separator, temp_list);
        Py_DECREF(separator);
        Py_DECREF(temp_list);

        if (!self->text_blob) return -1;
    }
    // Case 3: Input is a standard list or tuple
    else if (PyList_Check(text_input) || PyTuple_Check(text_input)) {
        PyObject *separator = PyUnicode_FromString(" ");
        self->text_blob = PyUnicode_Join(separator, text_input);
        Py_DECREF(separator);
        if (!self->text_blob) return -1;
    }
    // Case 4: Invalid input type
    else {
        PyErr_SetString(PyExc_TypeError, "BeautifulAnalyzer input must be a string, BString, list, or tuple.");
        return -1;
    }

    // Reset caches for the new text
    self->char_count = -1;
    self->word_count = -1;
    self->sentence_count = -1;

    return 0;
}

static PyObject *BeautifulAnalyzer_get_text_blob(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) {
        Py_RETURN_NONE;
    }
    Py_INCREF(self->text_blob);
    return self->text_blob;
}

static PyObject *BeautifulAnalyzer_get_char_count(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) Py_RETURN_NONE;
    if (self->char_count < 0) {
        self->char_count = PyUnicode_GET_LENGTH(self->text_blob);
    }
    return PyLong_FromSsize_t(self->char_count);
}

static PyObject *BeautifulAnalyzer_get_word_count(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) Py_RETURN_NONE;
    if (self->word_count < 0) {
        Py_ssize_t count = 0;
        int in_word = 0;
        Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
        for (Py_ssize_t i = 0; i < len; ++i) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
            if (Py_UNICODE_ISALNUM(ch)) {
                if (in_word == 0) {
                    in_word = 1;
                    count++;
                }
            } else {
                in_word = 0;
            }
        }
        self->word_count = count;
    }
    return PyLong_FromSsize_t(self->word_count);
}

static PyObject *BeautifulAnalyzer_get_sentence_count(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) Py_RETURN_NONE;
    if (self->sentence_count < 0) {
        Py_ssize_t count = 0;
        Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
        for (Py_ssize_t i = 0; i < len; ++i) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
            if (ch == '.' || ch == '!' || ch == '?') {
                count++;
            }
        }
        if (count == 0 && len > 0) {
            count = 1;
        }
        self->sentence_count = count;
    }
    return PyLong_FromSsize_t(self->sentence_count);
}

// ----------------------------------------------------------------------------
// BeautifulAnalyzer Definition Tables
// ----------------------------------------------------------------------------

// Add the new "text_blob" entry to this array
static PyGetSetDef BeautifulAnalyzer_getsetters[] = {
    {"text_blob", (getter)BeautifulAnalyzer_get_text_blob, NULL, "The full text content being analyzed.", NULL},
    {"char_count", (getter)BeautifulAnalyzer_get_char_count, NULL, "Total number of characters", NULL},
    {"word_count", (getter)BeautifulAnalyzer_get_word_count, NULL, "Total number of words", NULL},
    {"sentence_count", (getter)BeautifulAnalyzer_get_sentence_count, NULL, "Total number of sentences", NULL},
    {NULL}
};



// For this minimal version, the methods table is empty.
static PyMethodDef BeautifulAnalyzer_methods[] = {
    {NULL, NULL, 0, NULL}
};


// ----------------------------------------------------------------------------
// BeautifulAnalyzer Type Definition (Correct C89 Positional Order)
// ----------------------------------------------------------------------------

PyTypeObject BeautifulAnalyzerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "BeautifulString.BeautifulAnalyzer",              /* tp_name */
    sizeof(BeautifulAnalyzerObject),                  /* tp_basicsize */
    0,                                                /* tp_itemsize */
    (destructor)BeautifulAnalyzer_dealloc,            /* tp_dealloc */
    0,                                                /* tp_vectorcall_offset */
    0,                                                /* tp_getattr */
    0,                                                /* tp_setattr */
    0,                                                /* tp_as_async */
    0,                                                /* tp_repr */
    0,                                                /* tp_as_number */
    0,                                                /* tp_as_sequence */
    0,                                                /* tp_as_mapping */
    0,                                                /* tp_hash */
    0,                                                /* tp_call */
    0,                                                /* tp_str */
    0,                                                /* tp_getattro */
    0,                                                /* tp_setattro */
    0,                                                /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,         /* tp_flags */
    "A class to analyze text and provide statistics.",/* tp_doc */
    0,                                                /* tp_traverse */
    0,                                                /* tp_clear */
    0,                                                /* tp_richcompare */
    0,                                                /* tp_weaklistoffset */
    0,                                                /* tp_iter */
    0,                                                /* tp_iternext */
    BeautifulAnalyzer_methods,                        /* tp_methods */
    0,                                                /* tp_members */
    BeautifulAnalyzer_getsetters,                     /* tp_getset */
    0,                                                /* tp_base */
    0,                                                /* tp_dict */
    0,                                                /* tp_descr_get */
    0,                                                /* tp_descr_set */
    0,                                                /* tp_dictoffset */
    (initproc)BeautifulAnalyzer_init,                 /* tp_init */
    0,                                                /* tp_alloc */
    PyType_GenericNew,                                /* tp_new */
};