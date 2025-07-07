//
// Created by sinj on 27/06/2025.
//

#include "strvalidate_match.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// Helper to skip over expected literal
static int match_literal(const char **s_ptr, const char **f_ptr) {
    const char *s = *s_ptr;
    const char *f = *f_ptr;
    while (*f && *f != '%') {
        if (isspace((unsigned char)*f)) {
            while (isspace((unsigned char)*s)) s++;
            while (isspace((unsigned char)*f)) f++;
        } else {
            if (*s != *f) return 0;
            s++;
            f++;
        }
    }
    *s_ptr = s;
    *f_ptr = f;
    return 1;
}

// Try to match specifier value (basic validation)
static int match_specifier(const char **s_ptr, const char **f_ptr) {
    const char *s = *s_ptr;
    const char *f = *f_ptr;

    // Skip type specifier and move to after field name
    char type = *f;
    if (type == 'l' && *(f + 1) == 'f') {
        type = 'f';
        f += 2;
    } else {
        f++;
    }

    // Skip optional named field like (age)
    if (*f == '(') {
        while (*f && *f != ')') f++;
        if (*f == ')') f++;
    }

    while (isspace((unsigned char)*s)) s++;

    int n = 0;  // bytes consumed

    switch (type) {
        case 'd': {
            int tmp;
            if (sscanf(s, "%d%n", &tmp, &n) == 1) {
                *s_ptr += n;
                *f_ptr = f;
                return 1;
            }
            break;
        }
        case 'u': {
            unsigned int tmp;
            if (sscanf(s, "%u%n", &tmp, &n) == 1) {
                *s_ptr += n;
                *f_ptr = f;
                return 1;
            }
            break;
        }
        case 'f': {
            float tmp;
            if (sscanf(s, "%f%n", &tmp, &n) == 1) {
                *s_ptr += n;
                *f_ptr = f;
                return 1;
            }
            break;
        }
        case 'x': {
            unsigned int tmp;
            if (sscanf(s, "%x%n", &tmp, &n) == 1) {
                *s_ptr += n;
                *f_ptr = f;
                return 1;
            }
            break;
        }
        case 's': {
            if (*s == '"') {
                s++;
                while (*s && *s != '"') {
                    if (*s == '\\' && *(s + 1)) s++;  // skip escape
                    s++;
                }
                if (*s == '"') {
                    *s_ptr = s + 1;
                    *f_ptr = f;
                    return 1;
                }
            } else {
                const char *start = s;
                while (*s && !isspace((unsigned char)*s)) s++;
                if (s > start) {
                    *s_ptr = s;
                    *f_ptr = f;
                    return 1;
                }
            }
            break;
        }
        case 'w': {
            const char *start = s;
            while (isalnum((unsigned char)*s)) s++;
            if (s > start) {
                *s_ptr = s;
                *f_ptr = f;
                return 1;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

int strvalidate_match(const char *s, const char *f) {
    while (*f) {
        if (!match_literal(&s, &f)) return 0;

        if (*f == '%') {
            f++;
            const char *specifier = f;
            if (!match_specifier(&s, &specifier)) return 0;
            f = specifier;
        }
    }

    // while (isspace((unsigned char)*s)) s++;
    return *s == '\0';
}
