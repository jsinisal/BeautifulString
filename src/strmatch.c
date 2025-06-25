#include "strmatch.h"
#include "strmatch_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_MATCHES 32

static void skip_spaces(const char **p) {
    while (isspace(**p)) (*p)++;
}

static int parse_named_specifier(const char **p, char *type, char *name) {
    *type = **p;
    (*p)++;
    if (**p == '(') {
        (*p)++;
        int i = 0;
        while (**p && **p != ')' && i < 63) {
            name[i++] = **p;
            (*p)++;
        }
        name[i] = '\0';
        if (**p == ')') {
            (*p)++;
            return 1;
        } else {
            return 0;  // Unterminated name
        }
    }
    name[0] = '\0';
    return 1;
}

PyObject* strmatch(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *input;
    const char *pattern;
    const char *return_container = NULL;

    static char *kwlist[] = {"input", "pattern", "return_container", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|s", kwlist,
                                     &input, &pattern, &return_container)) {
        fprintf(stderr, "[DEBUG] PyArg_ParseTuple failed\n");
        return NULL;
                                     }

    return strmatch_internal(self, input, pattern, return_container, NULL, 0);
}

