#include "beanalyzer.h"
#include <ctype.h>
#include <python.h>
#include "bstring.h"

static PyObject *BeautifulAnalyzer_get_word_count(BeautifulAnalyzerObject *self, void *closure);
static PyObject *BeautifulAnalyzer_get_sentence_count(BeautifulAnalyzerObject *self, void *closure);

// INTERNAL HELPER: A simple heuristic-based syllable counter for English words.
static Py_ssize_t _count_syllables_in_word(PyObject *word_obj) {
    if (!word_obj || !PyUnicode_Check(word_obj)) return 0;

    PyObject *lower_word = PyObject_CallMethod(word_obj, "lower", NULL);
    if (!lower_word) return 0;

    Py_ssize_t count = 0;
    int in_vowel_group = 0;
    Py_ssize_t len = PyUnicode_GET_LENGTH(lower_word);
    if (len == 0) {
        Py_DECREF(lower_word);
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(lower_word, i);
        int is_vowel = (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'y');
        if (is_vowel && !in_vowel_group) {
            in_vowel_group = 1;
            count++;
        } else if (!is_vowel) {
            in_vowel_group = 0;
        }
    }

    // Handle common case of silent 'e' at the end
    if (len > 2 && PyUnicode_READ_CHAR(lower_word, len - 1) == 'e') {
        Py_UCS4 penultimate = PyUnicode_READ_CHAR(lower_word, len - 2);
        int is_vowel_before = (penultimate == 'a' || penultimate == 'e' || penultimate == 'i' || penultimate == 'o' || penultimate == 'u' || penultimate == 'y');
        if (is_vowel_before && penultimate != 'l') { // but keep 'le' as a syllable
            count--;
        }
    }

    if (count == 0) count = 1; // Every word must have at least one syllable.
    Py_DECREF(lower_word);
    return count;
}


// A private getter for total syllables, with caching.
static PyObject *_BeautifulAnalyzer_get_total_syllables(BeautifulAnalyzerObject *self) {
    if (!self->text_blob) return PyLong_FromLong(0);
    if (self->total_syllables < 0) {
        Py_ssize_t syllable_count = 0;
        Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
        Py_ssize_t word_start = -1;

        for (Py_ssize_t i = 0; i < len; ++i) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
            if (Py_UNICODE_ISALNUM(ch)) {
                if (word_start < 0) word_start = i;
            } else if (word_start >= 0) {
                PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
                syllable_count += _count_syllables_in_word(word);
                Py_DECREF(word);
                word_start = -1;
            }
        }
        if (word_start >= 0) {
            PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
            syllable_count += _count_syllables_in_word(word);
            Py_DECREF(word);
        }
        self->total_syllables = syllable_count;
    }
    return PyLong_FromSsize_t(self->total_syllables);
}


// Method: readability_score(formula='flesch_kincaid')
static PyObject *BeautifulAnalyzer_readability_score(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds) {
    // For now, we only implement one formula, but this shows how to add more.
    const char *formula = "flesch_kincaid";
    static char *kwlist[] = {"formula", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &formula)) {
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

    if (words == 0 || sentences == 0) {
        return PyFloat_FromDouble(0.0);
    }

    double score = 0.0;
    if (strcmp(formula, "flesch_kincaid") == 0) {
        // Flesch-Kincaid Grade Level formula
        score = 0.39 * (words / sentences) + 11.8 * (syllables / words) - 15.59;
    } else {
        PyErr_SetString(PyExc_ValueError, "Unknown formula specified. Use 'flesch_kincaid'.");
        return NULL;
    }

    return PyFloat_FromDouble(score);
}

static PyObject *BeautifulAnalyzer_word_frequency(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds) {
    Py_ssize_t top_n = -1;
    static char *kwlist[] = {"top_n", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &top_n)) {
        return NULL;
    }

    if (!self->text_blob) Py_RETURN_NONE;

    PyObject *word_counts = PyDict_New();
    if (!word_counts) return NULL;

    // --- Step 1: Count word occurrences (This part is correct) ---
    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    Py_ssize_t word_start = -1;
    for (Py_ssize_t i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
        if (Py_UNICODE_ISALNUM(ch)) {
            if (word_start < 0) word_start = i;
        } else if (word_start >= 0) {
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
    if (word_start >= 0) { // Handle the last word
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

    // --- Step 2: Convert dict to list and sort by value (count) ---
    PyObject *items = PyDict_Items(word_counts);
    Py_DECREF(word_counts);
    if (!items) return NULL;

    // --- THE FIX IS HERE ---
    // To sort by the second element of each tuple (the count), we must
    // call the list's "sort" method with a key.
    PyObject *operator_module = PyImport_ImportModule("operator");
    if (!operator_module) { Py_DECREF(items); return NULL; }

    PyObject *itemgetter = PyObject_GetAttrString(operator_module, "itemgetter");
    Py_DECREF(operator_module);
    if (!itemgetter) { Py_DECREF(items); return NULL; }

    PyObject *sort_key = PyObject_CallFunction(itemgetter, "i", 1);
    if (!sort_key) { Py_DECREF(itemgetter); Py_DECREF(items); return NULL; }

    // Get the list's "sort" method
    PyObject *sort_method = PyObject_GetAttrString(items, "sort");
    if (!sort_method) { Py_DECREF(sort_key); Py_DECREF(itemgetter); Py_DECREF(items); return NULL; }

    // Build the keyword arguments: key=itemgetter(1), reverse=True
    PyObject *sort_kwargs = PyDict_New();
    PyDict_SetItemString(sort_kwargs, "key", sort_key);
    PyDict_SetItemString(sort_kwargs, "reverse", Py_True);

    // Call list.sort(**sort_kwargs)
    PyObject *sort_result = PyObject_Call(sort_method, PyTuple_New(0), sort_kwargs);

    // Clean up all the temporary objects used for sorting
    Py_DECREF(sort_method);
    Py_DECREF(sort_kwargs);
    Py_DECREF(sort_key);
    Py_DECREF(itemgetter);
    Py_XDECREF(sort_result); // .sort() returns None, but we DECREF it for safety.

    if (PyErr_Occurred()) { // Check if the sort call failed
        Py_DECREF(items);
        return NULL;
    }
    // --- END OF FIX ---

    // --- Step 3: Slice the list if top_n is specified ---
    if (top_n >= 0 && top_n < PyList_Size(items)) {
        PyObject *sliced_list = PyList_GetSlice(items, 0, top_n);
        Py_DECREF(items);
        return sliced_list;
    }

    return items;
}

// ----------------------------------------------------------------------------
// BeautifulAnalyzer Lifecycle and Methods
// ----------------------------------------------------------------------------

// Implements the .generate_ngrams() method
static PyObject *
BeautifulAnalyzer_generate_ngrams(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds) {
    Py_ssize_t n = 2; // Default to bigrams
    static char *kwlist[] = {"n", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &n)) {
        return NULL;
    }

    if (n <= 0) {
        PyErr_SetString(PyExc_ValueError, "n must be greater than 0");
        return NULL;
    }

    if (!self->text_blob) Py_RETURN_NONE;

    // --- Step 1: Tokenize the entire text into a Python list of words ---
    PyObject *words_list = PyList_New(0);
    if (!words_list) return NULL;

    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    Py_ssize_t word_start = -1;
    for (Py_ssize_t i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
        if (Py_UNICODE_ISALNUM(ch)) {
            if (word_start < 0) word_start = i;
        } else if (word_start >= 0) {
            PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
            PyList_Append(words_list, word);
            Py_DECREF(word);
            word_start = -1;
        }
    }
    if (word_start >= 0) { // Handle the last word
        PyObject *word = PyUnicode_Substring(self->text_blob, word_start, len);
        PyList_Append(words_list, word);
        Py_DECREF(word);
    }

    // --- Step 2: Generate n-grams from the list of words ---
    PyObject *ngrams_list = PyList_New(0);
    Py_ssize_t num_words = PyList_Size(words_list);

    if (num_words >= n) {
        for (Py_ssize_t i = 0; i <= num_words - n; ++i) {
            PyObject *slice = PyList_GetSlice(words_list, i, i + n);
            if (!slice) {
                Py_DECREF(words_list);
                Py_DECREF(ngrams_list);
                return NULL;
            }

            // --- THE FIX IS HERE ---
            // Convert the list slice into a tuple before calling the constructor
            PyObject *args_tuple = PyList_AsTuple(slice);
            Py_DECREF(slice); // We are done with the temporary list slice
            if (!args_tuple) {
                Py_DECREF(words_list);
                Py_DECREF(ngrams_list);
                return NULL;
            }

            // Create a BString from the args_tuple
            PyObject *ngram_bstring = PyObject_CallObject((PyObject*)&BStringType, args_tuple);
            Py_DECREF(args_tuple); // We are done with the temporary tuple

            if (!ngram_bstring) {
                Py_DECREF(words_list);
                Py_DECREF(ngrams_list);
                return NULL;
            }

            PyList_Append(ngrams_list, ngram_bstring);
            Py_DECREF(ngram_bstring);
        }
    }

    Py_DECREF(words_list); // Clean up the token list
    return ngrams_list;
}


static PyObject *
BeautifulAnalyzer_get_unique_word_count(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) Py_RETURN_NONE;

    // Use cached value if available
    if (self->unique_word_count >= 0) {
        return PyLong_FromSsize_t(self->unique_word_count);
    }

    // A Python Set object is the perfect tool for tracking uniqueness
    PyObject *unique_words_set = PySet_New(NULL);
    if (!unique_words_set) return NULL;

    Py_ssize_t len = PyUnicode_GET_LENGTH(self->text_blob);
    Py_ssize_t word_start = -1;

    for (Py_ssize_t i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(self->text_blob, i);
        if (Py_UNICODE_ISALNUM(ch)) {
            if (word_start == -1) {
                word_start = i; // Mark the beginning of a new word
            }
        } else if (word_start != -1) {
            // End of a word found. Extract it, lowercase it, and add to the set.
            PyObject *word = PyUnicode_Substring(self->text_blob, word_start, i);
            PyObject *lower_word = PyObject_CallMethod(word, "lower", NULL);
            PySet_Add(unique_words_set, lower_word);
            Py_DECREF(word);
            Py_DECREF(lower_word);
            word_start = -1; // Reset for the next word
        }
    }
    // Handle the last word if the text doesn't end with a separator
    if (word_start != -1) {
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


static PyObject *
BeautifulAnalyzer_get_average_sentence_length(BeautifulAnalyzerObject *self, void *closure) {
    if (!self->text_blob) Py_RETURN_NONE;

    // To get the counts, we can call the other getter functions.
    // This automatically uses their cached values if available.
    PyObject *word_count_obj = BeautifulAnalyzer_get_word_count(self, closure);
    PyObject *sentence_count_obj = BeautifulAnalyzer_get_sentence_count(self, closure);

    Py_ssize_t words = PyLong_AsSsize_t(word_count_obj);
    Py_ssize_t sentences = PyLong_AsSsize_t(sentence_count_obj);

    Py_DECREF(word_count_obj);
    Py_DECREF(sentence_count_obj);

    // Avoid division by zero
    if (sentences == 0) {
        return PyFloat_FromDouble(0.0);
    }

    double avg = (double)words / (double)sentences;
    return PyFloat_FromDouble(avg);
}


static void BeautifulAnalyzer_dealloc(BeautifulAnalyzerObject *self) {
    Py_XDECREF(self->text_blob);
    self->text_blob = NULL;
    Py_TYPE(self)->tp_free((PyObject *)self);

}

static int BeautifulAnalyzer_init(BeautifulAnalyzerObject *self, PyObject *args, PyObject *kwds) {
    PyObject *text_input = NULL;
    static char *kwlist[] = {"text", NULL}; // Expect a 'text' keyword

    // We use "O" to accept any object type initially
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
    self->unique_word_count = -1;
    self->total_syllables = -1;

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
    {"unique_word_count", (getter)BeautifulAnalyzer_get_unique_word_count, NULL, "Total number of unique words", NULL},
    {"average_sentence_length", (getter)BeautifulAnalyzer_get_average_sentence_length, NULL, "Average words per sentence", NULL},
    {NULL}
};

static PyMethodDef BeautifulAnalyzer_methods[] = {
    {"readability_score", (PyCFunction)BeautifulAnalyzer_readability_score, METH_VARARGS | METH_KEYWORDS, "Calculate a readability score."},
    {"word_frequency", (PyCFunction)BeautifulAnalyzer_word_frequency, METH_VARARGS | METH_KEYWORDS, "Get a list of (word, count) tuples, sorted by frequency."},
    {"generate_ngrams", (PyCFunction)BeautifulAnalyzer_generate_ngrams, METH_VARARGS | METH_KEYWORDS, "Generate n-grams from the text."},
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