//
// Created by sinj on 28/06/2025.
//
#define PY_SSIZE_T_CLEAN
#include "bstring.h"
#include "library.h"
#include "Python.h"

// Forward declarations of static functions
static BStringNode* new_BStringNode(PyObject *str_obj);
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
static PyObject* BString_extend(BStringObject *self, PyObject *args);
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

// ============================================================================
// BString CSV FILE I/O
// ============================================================================
// INTERNAL HELPER for rendering a BString as a single CSV formatted string.
// This function contains the core CSV logic and is called by other methods.
static PyObject *_BString_render_as_csv_string(BStringObject *self, const char *delimiter, const char *quotechar, int quoting) {
    if (strlen(delimiter) != 1 || strlen(quotechar) != 1) {
        PyErr_SetString(PyExc_ValueError, "delimiter and quotechar must be single characters");
        return NULL;
    }

    PyObject *processed_fields = PyList_New(0);
    if (!processed_fields) return NULL;

    BStringNode *current = self->head;
    while (current) {
        int needs_quoting = 0;
        PyObject *field = current->str;

        if (quoting == BSTRING_QUOTE_ALL) {
            needs_quoting = 1;
        } else if (quoting == BSTRING_QUOTE_MINIMAL) {
            PyObject *delim_obj = PyUnicode_FromString(delimiter);
            PyObject *qc_obj = PyUnicode_FromString(quotechar);
            if (delim_obj && qc_obj &&
                (PyUnicode_Find(field, delim_obj, 0, PyUnicode_GET_LENGTH(field), 1) != -1 ||
                 PyUnicode_Find(field, qc_obj, 0, PyUnicode_GET_LENGTH(field), 1) != -1)) {
                needs_quoting = 1;
            }
            Py_XDECREF(delim_obj);
            Py_XDECREF(qc_obj);
        }

        PyObject *final_field;
        if (needs_quoting) {
            PyObject *qc_obj = PyUnicode_FromString(quotechar);
            PyObject *doubled_qc_obj = PyUnicode_FromFormat("%s%s", quotechar, quotechar);
            PyObject *escaped_field = PyUnicode_Replace(field, qc_obj, doubled_qc_obj, -1);

            if (escaped_field) {
                final_field = PyUnicode_FromFormat("%s%s%s", quotechar, PyUnicode_AsUTF8(escaped_field), quotechar);
                Py_DECREF(escaped_field);
            } else {
                final_field = NULL; // Propagate error
            }

            Py_DECREF(qc_obj);
            Py_DECREF(doubled_qc_obj);
        } else {
            Py_INCREF(field);
            final_field = field;
        }

        if (!final_field || PyList_Append(processed_fields, final_field) != 0) {
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

// Helper function to write a single BString as a CSV row to a file.
static int BString_write_csv_row(FILE *file, PyObject *bstring_obj, PyObject *kwds) {
    // Check if the object is a BString instance
    if (!PyObject_IsInstance(bstring_obj, (PyObject *)&BStringType)) {
        PyErr_SetString(PyExc_TypeError, "All items in data list and header must be BString objects.");
        return -1;
    }

    // Prepare arguments for the internal __call__ method
    PyObject *call_args = PyTuple_New(0);
    // The passed 'kwds' dictionary is already prepared by the caller
    PyObject *csv_row_str = PyObject_Call(bstring_obj, call_args, kwds);

    Py_DECREF(call_args); // Clean up the empty tuple

    if (!csv_row_str) {
        return -1; // The __call__ method failed
    }

    // Final check on the result
    if (!PyUnicode_Check(csv_row_str)) {
        Py_DECREF(csv_row_str);
        PyErr_SetString(PyExc_TypeError, "Internal row formatter did not return a string.");
        return -1;
    }

    // Write the formatted string to the file and clean up
    fprintf(file, "%s\n", PyUnicode_AsUTF8(csv_row_str));
    Py_DECREF(csv_row_str);

    return 0; // Success
}

// Implements the BString.to_csv() class method
static PyObject *BString_to_csv(PyObject *type, PyObject *args, PyObject *kwds) {
    const char *filepath;
    PyObject *data_list = NULL;
    PyObject *header_obj = NULL;

    // We manually parse arguments instead of using a single format string for robustness.
    // 1. Get the required positional filepath.
    if (!PyArg_ParseTuple(args, "s", &filepath)) {
        PyErr_SetString(PyExc_TypeError, "to_csv() requires a filepath as the first positional argument.");
        return NULL;
    }

    // 2. Safely get the required 'data' keyword argument.
    if (kwds) {
        data_list = PyDict_GetItemString(kwds, "data");
    }
    if (!data_list) {
        PyErr_SetString(PyExc_TypeError, "to_csv() missing required keyword-only argument: 'data'");
        return NULL;
    }
    if (!PyList_Check(data_list)) {
        PyErr_SetString(PyExc_TypeError, "The 'data' argument must be a list of BString objects.");
        return NULL;
    }

    // 3. Safely get the optional 'header' keyword argument.
    if (kwds) {
        header_obj = PyDict_GetItemString(kwds, "header");
    }

    // Prepare keyword arguments for the internal __call__ method
    PyObject *call_kwargs = PyDict_New();
    if (!call_kwargs) return NULL;

    // Copy formatting arguments (delimiter, etc.) from the main call to the internal call
    if (kwds && PyDict_Check(kwds)) {
        Py_ssize_t pos = 0;
        PyObject *key, *value;
        while (PyDict_Next(kwds, &pos, &key, &value)) {
            const char* key_str = PyUnicode_AsUTF8(key);
            if (strcmp(key_str, "data") != 0 && strcmp(key_str, "header") != 0) {
                 PyDict_SetItem(call_kwargs, key, value);
            }
        }
    }
    // Set the mandatory 'container' argument for the internal call
    PyDict_SetItemString(call_kwargs, "container", PyUnicode_FromString("csv"));

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
        Py_DECREF(call_kwargs);
        return NULL;
    }

    PyObject *empty_tuple = PyTuple_New(0);

    // Write Header
    if (header_obj && header_obj != Py_None) {
        PyObject *header_row_str = PyObject_Call(header_obj, empty_tuple, call_kwargs);
        if (!header_row_str) { fclose(file); Py_DECREF(call_kwargs); Py_DECREF(empty_tuple); return NULL; }
        fprintf(file, "%s\n", PyUnicode_AsUTF8(header_row_str));
        Py_DECREF(header_row_str);
    }

    // Write Data
    Py_ssize_t num_rows = PyList_Size(data_list);
    for (Py_ssize_t i = 0; i < num_rows; ++i) {
        PyObject *row_obj = PyList_GET_ITEM(data_list, i);
        PyObject *data_row_str = PyObject_Call(row_obj, empty_tuple, call_kwargs);
        if (!data_row_str) { fclose(file); Py_DECREF(call_kwargs); Py_DECREF(empty_tuple); return NULL; }
        fprintf(file, "%s\n", PyUnicode_AsUTF8(data_row_str));
        Py_DECREF(data_row_str);
    }

    fclose(file);
    Py_DECREF(call_kwargs);
    Py_DECREF(empty_tuple);
    Py_RETURN_NONE;
}

// Implements the BString.from_csv() class method
static PyObject *BString_from_csv(PyObject *type, PyObject *args, PyObject *kwds) {
    const char *filepath;
    int header = 1;
    static char *kwlist[] = {"filepath", "header", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|p", kwlist, &filepath, &header)) {
        return NULL;
    }

    // --- Step 1: Get the 'open' function safely ---
    PyObject *builtins = PyImport_ImportModule("builtins");
    if (!builtins) return NULL;
    PyObject *open_func = PyObject_GetAttrString(builtins, "open");
    Py_DECREF(builtins);
    if (!open_func) return NULL;

    // --- Step 2: Open the file with UTF-8 encoding ---
    PyObject *open_args = Py_BuildValue("(ss)", filepath, "r");
    PyObject *open_kwargs = Py_BuildValue("{s:s}", "encoding", "utf-8");
    PyObject *file_handle = PyObject_Call(open_func, open_args, open_kwargs);
    Py_DECREF(open_func);
    Py_DECREF(open_args);
    Py_DECREF(open_kwargs);
    if (!file_handle) return NULL;

    // --- Step 3: Create the CSV Reader ---
    PyObject *csv_module = PyImport_ImportModule("csv");
    if (!csv_module) { Py_DECREF(file_handle); return NULL; }
    PyObject *reader_func = PyObject_GetAttrString(csv_module, "reader");
    Py_DECREF(csv_module);
    if (!reader_func) { Py_DECREF(file_handle); return NULL; }

    PyObject *reader_obj = PyObject_CallFunctionObjArgs(reader_func, file_handle, NULL);
    Py_DECREF(reader_func);
    Py_DECREF(file_handle);
    if (!reader_obj) return NULL;

    PyObject *reader_iter = PyObject_GetIter(reader_obj);
    Py_DECREF(reader_obj);
    if (!reader_iter) return NULL;

    // --- Step 4: Process the header ---
    PyObject *header_bstring = NULL;
    if (header) {
        PyObject *row_list = PyIter_Next(reader_iter);
        if (row_list) {
            // FIX IS HERE: Convert list to tuple before calling
            PyObject *row_tuple = PyList_AsTuple(row_list);
            header_bstring = PyObject_CallObject(type, row_tuple);
            Py_DECREF(row_tuple); // Clean up the new tuple
            Py_DECREF(row_list);
            if (!header_bstring) { Py_DECREF(reader_iter); return NULL; }
        } else if (PyErr_Occurred()) {
             Py_DECREF(reader_iter); return NULL;
        } else {
            header_bstring = PyObject_CallObject(type, PyTuple_New(0));
            if (!header_bstring) { Py_DECREF(reader_iter); return NULL; }
        }
    }

    // --- Step 5: Process data rows ---
    PyObject *data_rows_list = PyList_New(0);
    if (!data_rows_list) { Py_XDECREF(header_bstring); Py_DECREF(reader_iter); return NULL; }

    PyObject *row_list;
    while ((row_list = PyIter_Next(reader_iter))) {
        // FIX IS HERE: Convert list to tuple before calling
        PyObject *row_tuple = PyList_AsTuple(row_list);
        PyObject *row_bstring = PyObject_CallObject(type, row_tuple);
        Py_DECREF(row_tuple); // Clean up the new tuple
        Py_DECREF(row_list);
        if (!row_bstring) {
            Py_XDECREF(header_bstring);
            Py_DECREF(data_rows_list);
            Py_DECREF(reader_iter);
            return NULL;
        }
        if (PyList_Append(data_rows_list, row_bstring) < 0) {
            Py_DECREF(row_bstring);
            Py_XDECREF(header_bstring);
            Py_DECREF(data_rows_list);
            Py_DECREF(reader_iter);
            return NULL;
        }
        Py_DECREF(row_bstring);
    }
    Py_DECREF(reader_iter);

    if (PyErr_Occurred()) {
        Py_XDECREF(header_bstring);
        Py_DECREF(data_rows_list);
        return NULL;
    }

    // --- Step 6: Return the final result ---
    if (header) {
        PyObject* return_value = PyTuple_Pack(2, header_bstring, data_rows_list);
        Py_DECREF(header_bstring);
        Py_DECREF(data_rows_list);
        return return_value;
    } else {
        return data_rows_list;
    }
}



// Implements the .to_file() instance method
static PyObject *BString_to_file(BStringObject *self, PyObject *args) {
    const char *filepath;
    if (!PyArg_ParseTuple(args, "s", &filepath)) {
        return NULL;
    }

    FILE *file = fopen(filepath, "w");
    if (!file) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
        return NULL;
    }

    BStringNode *current = self->head;
    while (current) {
        fprintf(file, "%s\n", PyUnicode_AsUTF8(current->str));
        current = current->next;
    }

    fclose(file);
    Py_RETURN_NONE;
}

// Implements the .from_file() class method
static PyObject *BString_from_file(PyObject *type, PyObject *args) {
    const char *filepath;
    if (!PyArg_ParseTuple(args, "s", &filepath)) {
        return NULL;
    }

    FILE *file = fopen(filepath, "r");
    if (!file) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filepath);
        return NULL;
    }

    // Create a new, empty BString instance by calling the type object
    BStringObject *new_bstring = (BStringObject *)((PyTypeObject*)type)->tp_new((PyTypeObject*)type, NULL, NULL);
    if (!new_bstring) {
        fclose(file);
        return NULL;
    }

    // Read the file line by line
    char buffer[4096]; // A reasonably sized buffer for lines
    while (fgets(buffer, sizeof(buffer), file)) {
        // Remove trailing newline character(s)
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Create a Python string from the C buffer
        PyObject* line_str = PyUnicode_FromString(buffer);
        if (!line_str) {
            Py_DECREF(new_bstring);
            fclose(file);
            return NULL;
        }

        // Append the new string to our BString object
        // (This is the same logic as our BString_append method)
        BStringNode *new_node = new_BStringNode(line_str);
        Py_DECREF(line_str); // new_BStringNode now holds the reference

        if (!new_node) {
            Py_DECREF(new_bstring);
            fclose(file);
            return NULL;
        }

        if (new_bstring->tail == NULL) {
            new_bstring->head = new_bstring->tail = new_bstring->current = new_node;
        } else {
            new_bstring->tail->next = new_node;
            new_node->prev = new_bstring->tail;
            new_bstring->tail = new_node;
        }
        new_bstring->size++;
    }

    fclose(file);

    return (PyObject *)new_bstring;
}

// ============================================================================
// BString MUTABILITY HELPERS
// ============================================================================

// Helper to remove a specific node and return its string. The caller is responsible for the reference.
// This is the core logic for pop() and del.
static PyObject *BString_remove_node(BStringObject *self, BStringNode *node_to_remove) {
    if (!node_to_remove) return NULL; // Should not happen if called correctly

    BStringNode *node_before = node_to_remove->prev;
    BStringNode *node_after = node_to_remove->next;

    // Rewire the list to bypass the node
    if (node_before) {
        node_before->next = node_after;
    } else {
        // We are removing the head
        self->head = node_after;
    }

    if (node_after) {
        node_after->prev = node_before;
    } else {
        // We are removing the tail
        self->tail = node_before;
    }

    // If the 'current' pointer was on the removed node, move it to a safe place.
    if (self->current == node_to_remove) {
        self->current = self->head;
    }

    self->size--;

    PyObject* returned_str = node_to_remove->str; // The string to be returned
    PyMem_Free(node_to_remove); // Free the C struct for the node

    return returned_str; // The caller now owns this reference
}

// ============================================================================
// BString MUTABILITY METHODS
// ============================================================================

static PyObject *BString_append(BStringObject *self, PyObject *obj) {
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "can only append a string");
        return NULL;
    }

    BStringNode *new_node = new_BStringNode(obj);
    if (!new_node) return NULL;

    if (self->tail == NULL) { // The list is currently empty
        self->head = self->tail = self->current = new_node;
    } else {
        self->tail->next = new_node;
        new_node->prev = self->tail;
        self->tail = new_node;
    }
    self->size++;
    Py_RETURN_NONE;
}

static PyObject *BString_insert(BStringObject *self, PyObject *args) {
    Py_ssize_t index;
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "nO", &index, &obj)) {
        return NULL;
    }
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "can only insert a string");
        return NULL;
    }

    // Handle edge cases for index, similar to list.insert()
    if (index >= self->size) {
        return BString_append(self, obj);
    }
    if (index < 0) {
        index += self->size;
        if (index < 0) index = 0;
    }

    // Find the node currently at the target index
    BStringNode *node_at_index = self->head;
    for (Py_ssize_t i = 0; i < index; ++i) {
        node_at_index = node_at_index->next;
    }

    BStringNode *new_node = new_BStringNode(obj);
    if (!new_node) return NULL;

    BStringNode *node_before = node_at_index->prev;

    // Rewire pointers
    new_node->next = node_at_index;
    new_node->prev = node_before;
    node_at_index->prev = new_node;

    if (node_before) {
        node_before->next = new_node;
    } else {
        // Inserting at the head
        self->head = new_node;
    }
    self->size++;
    Py_RETURN_NONE;
}

static PyObject *BString_pop(BStringObject *self, PyObject *args) {
    Py_ssize_t index = -1;
    if (!PyArg_ParseTuple(args, "|n", &index)) {
        return NULL;
    }
    if (self->size == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from empty BString");
        return NULL;
    }

    // Handle negative index
    if (index < 0) {
        index += self->size;
    }

    // Check bounds
    if (index < 0 || index >= self->size) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }

    // Find the node to pop
    BStringNode *node_to_pop = self->head;
    for (Py_ssize_t i = 0; i < index; ++i) {
        node_to_pop = node_to_pop->next;
    }

    return BString_remove_node(self, node_to_pop);
}

static PyObject *BString_remove(BStringObject *self, PyObject *value) {
    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a string");
        return NULL;
    }

    BStringNode *current = self->head;
    while(current) {
        int cmp_res = PyObject_RichCompareBool(current->str, value, Py_EQ);
        if (cmp_res > 0) { // Found the string
            PyObject *removed_str = BString_remove_node(self, current);
            Py_DECREF(removed_str); // We don't return it, so we release our reference
            Py_RETURN_NONE;
        }
        if (cmp_res < 0) { // Comparison failed
            return NULL;
        }
        current = current->next;
    }

    PyErr_SetString(PyExc_ValueError, "BString.remove(x): x not in BString");
    return NULL;
}


// ============================================================================
// BString METHOD DEFINITIONS
// ============================================================================
static PyMethodDef BString_methods[] = {
    { "map", (PyCFunction)BString_map, METH_VARARGS, "Apply a string method to all elements..." },
    { "filter", (PyCFunction)BString_filter, METH_VARARGS, "Filter elements using a string method..." },
    {"extend", (PyCFunction)BString_extend, METH_O, "Extend with items from a sequence."},
    { "append", (PyCFunction)BString_append, METH_O, "Append string to the end of the BString." },
    { "insert", (PyCFunction)BString_insert, METH_VARARGS, "Insert string before index." },
    { "pop", (PyCFunction)BString_pop, METH_VARARGS, "Remove and return string at index (default last)." },
    { "remove", (PyCFunction)BString_remove, METH_O, "Remove first occurrence of a string." },
    { "to_file", (PyCFunction)BString_to_file, METH_VARARGS,"Save the BString contents to a file, one string per line."},
    {"from_file", (PyCFunction)BString_from_file, METH_VARARGS | METH_CLASS,"Create a new BString from a line-delimited text file."},
    {"from_csv", (PyCFunction)BString_from_csv, METH_VARARGS | METH_KEYWORDS | METH_CLASS, "Create BString rows from a CSV file."},
    {"to_csv", (PyCFunction)BString_to_csv, METH_VARARGS | METH_KEYWORDS | METH_CLASS, "Save a list of BString rows to a CSV file."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


// ============================================================================
// BString MAP and FILTER Methods
// ============================================================================

// Implements the .map() method - CORRECTED VERSION
static PyObject *BString_map(BStringObject *self, PyObject *args) {
    PyObject *method_name_obj;
    PyObject *method_args_tuple = NULL;
    Py_ssize_t num_args = PyTuple_Size(args);

    if (num_args < 1) {
        PyErr_SetString(PyExc_TypeError, "map() requires at least one argument (the method name)");
        return NULL;
    }

    // 1. The first argument is the method name.
    method_name_obj = PyTuple_GET_ITEM(args, 0);
    if (!PyUnicode_Check(method_name_obj)) {
        PyErr_SetString(PyExc_TypeError, "first argument to map() must be a string method name");
        return NULL;
    }

    // 2. Pack all subsequent arguments into a new tuple.
    method_args_tuple = PyTuple_New(num_args - 1);
    if (!method_args_tuple) {
        return NULL; // Failed to create tuple
    }
    for (Py_ssize_t i = 1; i < num_args; ++i) {
        PyObject *item = PyTuple_GET_ITEM(args, i);
        Py_INCREF(item); // PyTuple_SET_ITEM steals a reference
        PyTuple_SET_ITEM(method_args_tuple, i - 1, item);
    }

    // Create a new BString for the results
    BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result) {
        Py_DECREF(method_args_tuple);
        return NULL;
    }

    BStringNode *current = self->head;
    int error_occurred = 0;

    while (current) {
        // 3. Get the method from the string object (e.g., 'replace')
        PyObject *method = PyObject_GetAttr(current->str, method_name_obj);
        if (!method) {
            error_occurred = 1;
            break; // Method not found, break the loop
        }

        // 4. Call the method with the arguments tuple
        PyObject *call_result = PyObject_CallObject(method, method_args_tuple);
        Py_DECREF(method); // Clean up the method object

        if (!call_result) {
            error_occurred = 1;
            break; // Error during method call, break the loop
        }

        // Add the new resulting string to our new BString
        BStringNode *new_node = new_BStringNode(call_result);
        Py_DECREF(call_result); // new_BStringNode has its own reference

        if (!new_node) {
            error_occurred = 1;
            break; // Memory allocation failed
        }

        if (!result->head) {
            result->head = result->tail = result->current = new_node;
        } else {
            result->tail->next = new_node;
            new_node->prev = result->tail;
            result->tail = new_node;
        }
        result->size++;

        current = current->next;
    }

    Py_DECREF(method_args_tuple);

    if (error_occurred) {
        Py_DECREF(result); // Clean up the result BString on error
        return NULL;
    }

    return (PyObject *)result;
}

// Implements the .filter() method with support for callables
static PyObject *BString_filter(BStringObject *self, PyObject *args) {
    PyObject *filter_condition;
    Py_ssize_t num_args = PyTuple_Size(args);

    if (num_args < 1) {
        PyErr_SetString(PyExc_TypeError, "filter() requires at least one argument (a method name or a callable)");
        return NULL;
    }

    // The first argument is either the method name or the callable
    filter_condition = PyTuple_GET_ITEM(args, 0);

    // Create a new BString for the results
    BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result) return NULL;

    BStringNode *current = self->head;
    int error_occurred = 0;

    // --- BRANCH 1: Filter condition is a CALLABLE (e.g., a function or lambda) ---
    if (PyCallable_Check(filter_condition)) {
        if (num_args > 1) {
            PyErr_SetString(PyExc_TypeError, "when using a callable, filter() takes exactly 1 argument");
            Py_DECREF(result);
            return NULL;
        }

        while(current) {
            // Call the callable, passing the current string as the argument
            PyObject *call_result = PyObject_CallFunctionObjArgs(filter_condition, current->str, NULL);
            if (!call_result) {
                error_occurred = 1;
                break;
            }

            int is_true = PyObject_IsTrue(call_result);
            Py_DECREF(call_result);

            if (is_true < 0) { // Error during truthiness check
                error_occurred = 1;
                break;
            }
            if (is_true) {
                // If true, add the ORIGINAL string to the result list
                BStringNode *new_node = new_BStringNode(current->str);
                if (!new_node) { error_occurred = 1; break; }

                if (!result->head) {
                    result->head = result->tail = result->current = new_node;
                } else {
                    result->tail->next = new_node;
                    new_node->prev = result->tail;
                    result->tail = new_node;
                }
                result->size++;
            }
            current = current->next;
        }
    }
    // --- BRANCH 2: Filter condition is a STRING (a method name) ---
    else if (PyUnicode_Check(filter_condition)) {
        // Pack all subsequent arguments into a tuple for the method call
        PyObject *method_args_tuple = PyTuple_New(num_args - 1);
        if (!method_args_tuple) { Py_DECREF(result); return NULL; }
        for (Py_ssize_t i = 1; i < num_args; ++i) {
            PyObject *item = PyTuple_GET_ITEM(args, i);
            Py_INCREF(item);
            PyTuple_SET_ITEM(method_args_tuple, i - 1, item);
        }

        while(current) {
            const char* method_name = PyUnicode_AsUTF8(filter_condition);
            // Call the method on the string object
            PyObject *call_result = PyObject_CallMethod(current->str, method_name, "O", method_args_tuple);
            if (!call_result) {
                error_occurred = 1;
                break;
            }

            int is_true = PyObject_IsTrue(call_result);
            Py_DECREF(call_result);

            if (is_true < 0) { error_occurred = 1; break; }
            if (is_true) {
                 // If true, add the ORIGINAL string to the result list
                BStringNode *new_node = new_BStringNode(current->str);
                if (!new_node) { error_occurred = 1; break; }

                if (!result->head) {
                    result->head = result->tail = result->current = new_node;
                } else {
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
    // --- BRANCH 3: Invalid argument type ---
    else {
        PyErr_SetString(PyExc_TypeError, "first argument to filter() must be a string or a callable");
        Py_DECREF(result);
        return NULL;
    }

    if (error_occurred) {
        Py_DECREF(result);
        return NULL;
    }

    return (PyObject *)result;
}

// Implements the '*' operator for BString
static PyObject *BString_repeat(BStringObject *self, Py_ssize_t n) {
    // Create a new empty BString for the result
    BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result) {
        return NULL;
    }

    // If n is 0 or less, return an empty BString
    if (n <= 0) {
        return (PyObject *)result;
    }

    // Repeat n times
    for (Py_ssize_t i = 0; i < n; ++i) {
        BStringNode *current = self->head;
        while (current) {
            BStringNode *new_node = new_BStringNode(current->str);
            if (!new_node) {
                return NULL; // Handle memory allocation failure
            }
            if (!result->head) {
                result->head = result->tail = result->current = new_node;
            } else {
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

// Helper function for BString_concat on MSVC
static int BString_append_nodes_from_source(BStringObject *result, BStringObject* source) {
    BStringNode *current = source->head;
    while (current) {
        BStringNode *new_node = new_BStringNode(current->str);
        if (!new_node) return -1; // Error

        if (!result->head) {
            result->head = result->tail = result->current = new_node;
        } else {
            result->tail->next = new_node;
            new_node->prev = result->tail;
            result->tail = new_node;
        }
        result->size++;
        current = current->next;
    }
    return 0; // Success
}

static PyObject *BString_concat(BStringObject *self, PyObject *other) {
    if (!PyObject_IsInstance((PyObject *)other, (PyObject *)&BStringType)) {
        PyErr_SetString(PyExc_TypeError, "can only concatenate BString to BString");
        return NULL;
    }

    BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!result) return NULL;

    if (BString_append_nodes_from_source(result, self) != 0) {
        Py_DECREF(result);
        return NULL;
    }
    if (BString_append_nodes_from_source(result, (BStringObject *)other) != 0) {
        Py_DECREF(result);
        return NULL;
    }

    return (PyObject *)result;
}


// Helper function to create a new BString object from a single


// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Creates a new BStringNode
static BStringNode* new_BStringNode(PyObject *str_obj) {
    BStringNode *node = PyMem_Malloc(sizeof(BStringNode));
    if (!node) {
        PyErr_NoMemory();
        return NULL;
    }
    Py_INCREF(str_obj);
    node->str = str_obj;
    node->next = NULL;
    node->prev = NULL;
    return node;
}


// ============================================================================
// ITERATOR PROTOCOL IMPLEMENTATION
// This section is moved up to resolve the compilation error.
// ============================================================================

// Iterator's deallocation function
static void BStringIter_dealloc(BStringIterObject *iter) {
    // No need to DECREF current_node, it's a borrowed reference
    PyObject_Del(iter);
}

// Iterator's __next__ method
static PyObject *BStringIter_iternext(BStringIterObject *iter) {
    if (iter->current_node) {
        PyObject *str_obj = iter->current_node->str;
        Py_INCREF(str_obj); // Return a new reference
        iter->current_node = iter->current_node->next;
        return str_obj;
    } else {
        // No more items, signal the end of iteration
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

// The Iterator Type Definition
// Now defined before it is used.
PyTypeObject BStringIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "BStringIter",
    .tp_basicsize = sizeof(BStringIterObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)BStringIter_dealloc,
    .tp_iter = PyObject_SelfIter, // The iterator is its own iterator
    .tp_iternext = (iternextfunc)BStringIter_iternext,
};


// ============================================================================
// BString MAIN TYPE METHODS
// ============================================================================

// __new__
static PyObject *BString_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    BStringObject *self;
    self = (BStringObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->head = NULL;
        self->tail = NULL;
        self->current = NULL;
        self->size = 0;
        self->weakreflist = NULL;
    }
    return (PyObject *)self;
}

// __init__
static int BString_init(BStringObject *self, PyObject *args, PyObject *kwds) {
    for (int i = 0; i < PyTuple_GET_SIZE(args); ++i) {
        PyObject *item = PyTuple_GET_ITEM(args, i);
        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "All arguments must be strings.");
            return -1;
        }

        BStringNode *node = new_BStringNode(item);
        if (!node) return -1;

        if (!self->head) { // First item
            self->head = self->tail = self->current = node;
        } else {
            self->tail->next = node;
            node->prev = self->tail;
            self->tail = node;
        }
        self->size++;
    }
    return 0;
}

// __dealloc__
static void BString_dealloc(BStringObject *self) {
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

    BStringNode *current = self->head;
    while (current) {
        BStringNode *next = current->next;
        Py_DECREF(current->str); // Decrement ref count for the Python string
        PyMem_Free(current);     // Free the node itself
        current = next;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

// __repr__ or print()
static PyObject *BString_repr(BStringObject *self) {
    PyObject *list = PyList_New(0);
    if (!list) return NULL;

    BStringNode *current = self->head;
    while (current) {
        if (PyList_Append(list, current->str) != 0) {
            Py_DECREF(list);
            return NULL;
        }
        current = current->next;
    }

    PyObject *repr = PyObject_Repr(list);
    Py_DECREF(list);
    return repr;
}

// BString_call function
static PyObject *BString_call(BStringObject *self, PyObject *args, PyObject *kwds) {
    // Default values for parameters
    char *container = "list";
    PyObject *keys = NULL;
    char *delimiter = ",";
    char *quotechar = "\"";
    int quoting = BSTRING_QUOTE_MINIMAL;

    static char *kwlist[] = {"container", "keys", "delimiter", "quotechar", "quoting", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOssi", kwlist,
                                     &container, &keys, &delimiter, &quotechar, &quoting)) {
        return NULL;
    }

    // --- Handle 'list' container ---
    if (strcmp(container, "list") == 0) {
        PyObject *list = PyList_New(self->size);
        if (!list) return NULL;
        BStringNode *current = self->head;
        for (Py_ssize_t i = 0; i < self->size; ++i) {
            Py_INCREF(current->str);
            PyList_SET_ITEM(list, i, current->str);
            current = current->next;
        }
        return list;
    }

    // --- Handle 'tuple' container ---
    if (strcmp(container, "tuple") == 0) {
        PyObject *tuple = PyTuple_New(self->size);
        if (!tuple) return NULL;
        BStringNode *current = self->head;
        for (Py_ssize_t i = 0; i < self->size; ++i) {
            Py_INCREF(current->str);
            PyTuple_SET_ITEM(tuple, i, current->str);
            current = current->next;
        }
        return tuple;
    }

    // --- Handle 'dict' container ---
    if (strcmp(container, "dict") == 0) {
        if (!keys || !PyList_Check(keys) || PyList_Size(keys) != self->size) {
            PyErr_SetString(PyExc_ValueError, "'keys' must be a list of the same length as the BString.");
            return NULL;
        }
        PyObject *dict = PyDict_New();
        if (!dict) return NULL;
        BStringNode *current = self->head;
        for (Py_ssize_t i = 0; i < self->size; ++i) {
            if (PyDict_SetItem(dict, PyList_GET_ITEM(keys, i), current->str) != 0) {
                Py_DECREF(dict);
                return NULL;
            }
            current = current->next;
        }
        return dict;
    }

    // --- Handle 'csv' container ---
    if (strcmp(container, "csv") == 0) {
        return _BString_render_as_csv_string(self, delimiter, quotechar, quoting);
    }

    // --- Handle 'json' container ---
    if (strcmp(container, "json") == 0) {
        PyObject *data_to_dump = NULL;

        if (keys == NULL) { // No keys provided, create a list
            data_to_dump = BString_call(self, PyTuple_New(0), NULL); // Reuse list creation
        } else { // Keys provided, create a dict
            PyObject* kwargs = PyDict_New();
            PyDict_SetItemString(kwargs, "keys", keys);
            data_to_dump = BString_call(self, PyTuple_New(0), kwargs); // Reuse dict creation
            Py_DECREF(kwargs);
        }
        if (!data_to_dump) return NULL;

        // Use Python's json module to dump the object
        PyObject *json_module = PyImport_ImportModule("json");
        if (!json_module) { Py_DECREF(data_to_dump); return NULL; }

        PyObject *dumps_func = PyObject_GetAttrString(json_module, "dumps");
        PyObject *json_string = PyObject_CallFunctionObjArgs(dumps_func, data_to_dump, NULL);

        Py_DECREF(dumps_func);
        Py_DECREF(json_module);
        Py_DECREF(data_to_dump);

        return json_string;
    }

    // Fallback for invalid container name
    PyErr_SetString(PyExc_ValueError, "Invalid container type specified. Choose from 'list', 'tuple', 'dict', 'csv', 'json'.");
    return NULL;
}

// Implements the .extend() method
static PyObject *BString_extend(BStringObject *self, PyObject *iterable) {
    // Get an iterator from the input object
    PyObject *iterator = PyObject_GetIter(iterable);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "extend() argument must be an iterable");
        return NULL;
    }

    PyObject *item;
    // Loop through the iterator
    while ((item = PyIter_Next(iterator))) {
        // Check if the item is a string
        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "BString can only extend with an iterable of strings");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return NULL;
        }

        // Create a new node and append it
        BStringNode *new_node = new_BStringNode(item);
        if (!new_node) {
            Py_DECREF(item);
            Py_DECREF(iterator);
            return NULL;
        }

        if (self->tail == NULL) { // The BString is currently empty
            self->head = self->tail = self->current = new_node;
        } else {
            self->tail->next = new_node;
            new_node->prev = self->tail;
            self->tail = new_node;
        }
        self->size++;

        // We are done with the item from the iterator
        Py_DECREF(item);
    }

    // Clean up the iterator
    Py_DECREF(iterator);

    // Check if an error occurred during iteration
    if (PyErr_Occurred()) {
        return NULL;
    }

    Py_RETURN_NONE;
}


// BString's __iter__ method
static PyObject *BString_iter(BStringObject *self) {
    // Reset the main object's current pointer to the head,
    // useful for consistent behavior of .next/.prev after iteration.
    self->current = self->head;

    // Create a new, dedicated iterator object
    BStringIterObject *iter = PyObject_New(BStringIterObject, &BStringIter_Type);
    if (!iter) {
        return NULL;
    }

    // The new iterator starts at the head of the BString's list
    iter->current_node = self->head;

    return (PyObject *)iter;
}


// ============================================================================
// BString PROTOCOL DEFINITIONS
// ============================================================================

// Sequence Protocol
static Py_ssize_t BString_length(BStringObject *self) {
    return self->size;
}

// ============================================================================
// BString SEQUENCE METHODS (MODIFIED)
// ============================================================================

// --- This function replaces the old BString_getitem ---
static PyObject *BString_getitem(BStringObject *self, PyObject *key) {
    // Check if the key is a slice object
    if (PySlice_Check(key)) {
        Py_ssize_t start, stop, step, slicelength;

        // PySlice_GetIndicesEx is the modern, safe way to get slice indices.
        // It correctly handles negative indices and out-of-bounds values.
        if (PySlice_GetIndicesEx(key, self->size, &start, &stop, &step, &slicelength) < 0) {
            return NULL; // Error occurred
        }

        // Create a new BString object to hold the slice result
        BStringObject *result = (BStringObject *)BString_new(&BStringType, NULL, NULL);
        if (!result) return NULL;

        if (slicelength <= 0) {
            return (PyObject *)result; // Return empty BString
        }

        // Find the starting node
        BStringNode *current = self->head;
        for (Py_ssize_t i = 0; i < start; ++i) {
            current = current->next;
        }

        // Populate the new BString with the sliced nodes
        for (Py_ssize_t i = 0; i < slicelength; ++i) {
            BStringNode *new_node = new_BStringNode(current->str);
            if (!new_node) {
                BString_dealloc(result);
                return NULL;
            }
            if (!result->head) {
                result->head = result->tail = result->current = new_node;
            } else {
                result->tail->next = new_node;
                new_node->prev = result->tail;
                result->tail = new_node;
            }
            result->size++;

            // Move to the next node according to the step
            for(Py_ssize_t j = 0; j < step && current; ++j) {
                current = current->next;
            }
        }

        return (PyObject *)result;
    }
    // --- Fallback to integer indexing if not a slice ---
    else if (PyLong_Check(key)) {
        Py_ssize_t i = PyLong_AsSsize_t(key);
        if (i < 0) {
            i += self->size;
        }
        if (i < 0 || i >= self->size) {
            PyErr_SetString(PyExc_IndexError, "BString index out of range");
            return NULL;
        }
        BStringNode *current = self->head;
        for (Py_ssize_t j = 0; j < i; ++j) {
            current = current->next;
        }
        Py_INCREF(current->str);
        return current->str;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "BString indices must be integers or slices");
        return NULL;
    }
}

// Helper to remove a range of nodes for slice deletion.
// Returns 0 on success, -1 on failure.
static int BString_delete_slice_nodes(BStringObject *self, Py_ssize_t start, Py_ssize_t stop) {
    if (start >= stop) {
        return 0; // Nothing to delete.
    }

    // Find the node at the start of the slice.
    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < start && current; ++i) {
        current = current->next;
    }

    // Find the node before the slice and the node at the end of the slice.
    BStringNode *node_before_slice = (start > 0 && current) ? current->prev : NULL;
    BStringNode *node_at_stop = current;
    for (Py_ssize_t i = start; i < stop && node_at_stop; ++i) {
        node_at_stop = node_at_stop->next;
    }

    // Delete the nodes in the slice range.
    for (Py_ssize_t i = start; i < stop; ++i) {
        if (!current) break; // Should not happen with correct bounds.
        BStringNode *next_node = current->next;
        Py_DECREF(current->str);
        PyMem_Free(current);
        current = next_node;
        self->size--;
    }

    // Re-link the list across the gap.
    if (node_before_slice) {
        node_before_slice->next = node_at_stop;
    } else {
        self->head = node_at_stop; // Deleted from the beginning.
    }

    if (node_at_stop) {
        node_at_stop->prev = node_before_slice;
    } else {
        self->tail = node_before_slice; // Deleted to the end.
    }

    self->current = self->head; // Reset current pointer to a safe location.
    return 0;
}


static int BString_ass_item(BStringObject *self, PyObject *key, PyObject *value) {
    // --- Handle Slice Keys ---
    if (PySlice_Check(key)) {
        Py_ssize_t start, stop, step, slicelength;
        if (PySlice_GetIndicesEx(key, self->size, &start, &stop, &step, &slicelength) < 0) {
            return -1; // Error parsing slice.
        }

        // For now, we only support simple slices (step=1) for assignment/deletion.
        if (step != 1) {
            PyErr_SetString(PyExc_NotImplementedError, "BString slicing with step != 1 is not supported for assignment.");
            return -1;
        }

        // First, delete the nodes in the target slice.
        if (BString_delete_slice_nodes(self, start, stop) != 0) {
            return -1; // Deletion failed.
        }

        // If 'value' is not NULL, it's an assignment, so insert the new items.
        // If 'value' is NULL, it was a deletion, and we are done.
        if (value) {
            // Find the node at the insertion point.
            BStringNode *insertion_point = self->head;
            for (Py_ssize_t i = 0; i < start && insertion_point; ++i) {
                insertion_point = insertion_point->next;
            }

            // Get an iterator for the value to be assigned.
            PyObject *iterator = PyObject_GetIter(value);
            if (!iterator) {
                PyErr_SetString(PyExc_TypeError, "can only assign an iterable to a slice");
                return -1;
            }

            // Loop through the iterator and insert new nodes.
            PyObject *item;
            BStringNode *last_inserted_node = insertion_point ? insertion_point->prev : self->tail;

            while ((item = PyIter_Next(iterator))) {
                BStringNode *new_node = new_BStringNode(item);
                Py_DECREF(item);
                if (!new_node) { Py_DECREF(iterator); return -1; }

                new_node->prev = last_inserted_node;
                new_node->next = insertion_point;

                if (last_inserted_node) {
                    last_inserted_node->next = new_node;
                } else {
                    self->head = new_node;
                }
                last_inserted_node = new_node;
                self->size++;
            }
            Py_DECREF(iterator);

            if (insertion_point) {
                insertion_point->prev = last_inserted_node;
            } else {
                self->tail = last_inserted_node;
            }
            
            if (PyErr_Occurred()) return -1; // Check for errors during iteration.
        }
        return 0; // Success.
    }
    // --- Handle Integer Keys ---
    else if (PyLong_Check(key)) {
        Py_ssize_t i = PyLong_AsSsize_t(key);
        if (i < 0) { i += self->size; }

        if (i < 0 || i >= self->size) {
            PyErr_SetString(PyExc_IndexError, "BString assignment index out of range");
            return -1;
        }

        BStringNode *target_node = self->head;
        for (Py_ssize_t j = 0; j < i; ++j) { target_node = target_node->next; }

        // Handle deletion: del my_str[i]
        if (value == NULL) {
            // We need a helper for removing a single node to avoid code duplication with pop().
            // Let's assume a helper BString_remove_node exists.
            PyObject *removed_str = BString_remove_node(self, target_node);
            Py_DECREF(removed_str);
            return 0;
        }

        // Handle assignment: my_str[i] = "new"
        if (!PyUnicode_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "BString items must be strings");
            return -1;
        }

        Py_DECREF(target_node->str); // DECREF the old string.
        Py_INCREF(value);            // INCREF the new one.
        target_node->str = value;
        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "BString indices must be integers or slices");
        return -1;
    }
}



// This replaces the old PySequenceMethods definition
static PySequenceMethods BString_as_sequence = {
    (lenfunc)BString_length,        // sq_length
    (binaryfunc)BString_concat,     // sq_concat (+)
    (ssizeargfunc)BString_repeat,   // sq_repeat (*)
    0,                              // sq_item (handled by mapping protocol)
    0,                              // sq_ass_item (handled by mapping protocol)
    0,                              // sq_contains
    0,                              // sq_inplace_concat
    0,                              // sq_inplace_repeat
};

// We use PyMappingMethods because it supports general key types (slices).
// This is the modern way to implement subscripting.
static PyMappingMethods BString_as_mapping = {
    (lenfunc)BString_length,        // mp_length
    (binaryfunc)BString_getitem,    // mp_subscript
    (objobjargproc)BString_ass_item // mp_ass_subscript
};

// ============================================================================
// BString GETTERS (for head, tail, next, prev) - REVISED SECTION
// ============================================================================

static PyObject *BString_CopyWithCurrent(BStringObject *original, BStringNode *orig_target_current) {
    if (!original) {
        Py_RETURN_NONE;
    }

    // Step 1: Create a new BString and deep copy the entire linked list structure.
    BStringObject *new_bstring = (BStringObject *)BString_new(&BStringType, NULL, NULL);
    if (!new_bstring) return NULL;

    BStringNode *temp_node = original->head;
    while(temp_node) {
         BStringNode *new_node = new_BStringNode(temp_node->str);
         if (!new_node) {
             BString_dealloc(new_bstring);
             return NULL;
         }
         if (!new_bstring->head) {
             new_bstring->head = new_bstring->tail = new_node;
         } else {
             new_bstring->tail->next = new_node;
             new_node->prev = new_bstring->tail;
             new_bstring->tail = new_node;
         }
         new_bstring->size++;
         temp_node = temp_node->next;
    }

    // Step 2: Find the equivalent node in the NEW list to set as 'current'.
    BStringNode *orig_iter = original->head;
    BStringNode *new_iter = new_bstring->head;
    while(orig_iter) {
        if (orig_iter == orig_target_current) {
            new_bstring->current = new_iter;
            break;
        }
        orig_iter = orig_iter->next;
        new_iter = new_iter->next;
    }

    // If for some reason the target wasn't found, default to head.
    if (!orig_iter) {
        new_bstring->current = new_bstring->head;
    }

    return (PyObject *)new_bstring;
}

// The 'head' of a BString is its CURRENT item.
static PyObject *BString_get_head(BStringObject *self, void *closure) {
    // If current isn't set, default to the absolute head.
    if (!self->current) {
        self->current = self->head;
    }
    if (!self->current) {
        Py_RETURN_NONE;
    }
    Py_INCREF(self->current->str);
    return self->current->str;
}

// The 'tail' of a BString is always the absolute last item.
static PyObject *BString_get_tail(BStringObject *self, void *closure) {
    if (!self->tail) {
        Py_RETURN_NONE;
    }
    // Accessing tail should reset the 'current' pointer to the end.
    self->current = self->tail;
    Py_INCREF(self->tail->str);
    return self->tail->str;
}

// FINAL Getter for .next, using the new helper.
static PyObject *BString_get_next(BStringObject *self, void *closure) {
    if (!self->current || !self->current->next) {
        Py_RETURN_NONE;
    }
    // Create a copy, telling it to set its 'current' to our 'next'.
    return BString_CopyWithCurrent(self, self->current->next);
}

// FINAL Getter for .prev, using the new helper.
static PyObject *BString_get_prev(BStringObject *self, void *closure) {
    if (!self->current || !self->current->prev) {
        Py_RETURN_NONE;
    }
    // Create a copy, telling it to set its 'current' to our 'prev'.
    return BString_CopyWithCurrent(self, self->current->prev);
}


// Getters/Setters definition array - THIS REMAINS THE SAME
static PyGetSetDef BString_getsetters[] = {
    {"head", (getter)BString_get_head, NULL, "Current string in the sequence", NULL},
    {"tail", (getter)BString_get_tail, NULL, "Last string in the collection", NULL},
    {"next", (getter)BString_get_next, NULL, "New BString with pointer moved to the next element", NULL},
    {"prev", (getter)BString_get_prev, NULL, "New BString with pointer moved to the previous element", NULL},
    {NULL}  /* Sentinel */
};

// ============================================================================
// BString TYPE DEFINITION (MODIFIED)
// ============================================================================
PyTypeObject BStringType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "BeautifulString.BString",
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
    .tp_methods = BString_methods, // ADD THIS
    .tp_call = (ternaryfunc)BString_call,
    .tp_iter = (getiterfunc)BString_iter,
    .tp_getset = BString_getsetters,
    .tp_weaklistoffset = offsetof(BStringObject, weakreflist),
};



// ============================================================================
// BString HELPER for Deleting a Slice
// ============================================================================
// This is a new helper function to cleanly handle the logic of removing a range of nodes.
static int String_delete_slice_nodes(BStringObject *self, Py_ssize_t start, Py_ssize_t stop) {
    // Note: This simple version only handles step=1 slices for deletion.
    // A full implementation would need to handle steps, but this covers `my_str[a:b]`.
    if (start >= stop) {
        return 0; // Nothing to delete
    }

    // 1. Find the starting node
    BStringNode *current = self->head;
    for (Py_ssize_t i = 0; i < start && current; ++i) {
        current = current->next;
    }

    // 2. Find the node *before* the slice and *at the end* of the slice
    BStringNode *node_before_slice = (start > 0) ? current->prev : NULL;
    BStringNode *node_at_stop = current;
    for (Py_ssize_t i = start; i < stop && node_at_stop; ++i) {
        node_at_stop = node_at_stop->next;
    }

    // 3. Delete nodes from `start` up to (but not including) `stop`
    for (Py_ssize_t i = start; i < stop; ++i) {
        if (!current) break;
        BStringNode *next_node = current->next;
        Py_DECREF(current->str);
        PyMem_Free(current);
        current = next_node;
        self->size--;
    }

    // 4. Re-link the list
    if (node_before_slice) {
        node_before_slice->next = node_at_stop;
    } else {
        // We deleted from the beginning
        self->head = node_at_stop;
    }

    if (node_at_stop) {
        node_at_stop->prev = node_before_slice;
    } else {
        // We deleted to the end
        self->tail = node_before_slice;
    }

    // Reset current pointer to a safe location
    self->current = self->head;

    return 0;
}