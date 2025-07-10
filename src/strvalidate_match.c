//
// Created by sinj on 27/06/2025.
//

#include "strvalidate_match.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if defined(_WIN32)
#define strncasecmp _strnicmp
#endif

// Helper to skip over expected literal text and whitespace
static int match_literal(const char **s_ptr, const char **f_ptr) {
    const char *s = *s_ptr;
    const char *f = *f_ptr;
    while (*f && *f != '%') {
        if (isspace((unsigned char)*f)) {
            // Flexible whitespace matching
            while (isspace((unsigned char)*s)) s++;
            while (isspace((unsigned char)*f)) f++;
        } else {
            // Exact character match
            if (*s != *f) return 0;
            s++;
            f++;
        }
    }
    *s_ptr = s;
    *f_ptr = f;
    return 1;
}

// Helper to match a single format specifier
static int match_specifier(const char **s_ptr, const char **f_ptr) {
    const char *s = *s_ptr;
    const char *f = *f_ptr;
    char *endptr = (char *)s; // Initialize to current position

    char type = *f;
    f++; // Advance format pointer past the specifier character

    // Skip optional named field like (age)
    if (*f == '(') {
        const char *start_paren = f;
        while (*f && *f != ')') f++;
        if (*f == ')') {
            f++; // Skip ')' only if found
        } else {
            f = start_paren; // Malformed, backtrack
        }
    }

    // Skip leading whitespace in the source string before matching a value
    while (isspace((unsigned char)*s)) s++;

    const char *s_before_match = s;

    switch (type) {
        case 'd': strtol(s, &endptr, 10); break;
        case 'u': strtoul(s, &endptr, 10); break;
        case 'x': strtol(s, &endptr, 16); break;
        case 'f': strtod(s, &endptr); break;
        case 'b': {
            if (strncasecmp(s, "true", 4) == 0) s += 4;
            else if (strncasecmp(s, "false", 5) == 0) s += 5;
            else if (*s == '1') s += 1;
            else if (*s == '0') s += 1;
            endptr = (char*)s;
            break;
        }
        case 's': {
            if (*s == '"' || *s == '\'') {
                char opening_quote = *s;
                s++;
                while (*s && *s != opening_quote) {
                    if (*s == '\\' && *(s + 1)) s++;
                    s++;
                }
                // --- THE FIX IS HERE ---
                if (*s == opening_quote) {
                    s++; // Consume matching closing quote
                } else {
                    // Unclosed quote, this is a failure.
                    s = s_before_match; // Reset pointer to signal no match
                }
            } else { // Unquoted, non-whitespace string
                while (*s && !isspace((unsigned char)*s)) s++;
            }
            endptr = (char*)s;
            break;
        }
        case 'w': { // Alphanumeric word
            while (isalnum((unsigned char)*s)) s++;
            endptr = (char*)s;
            break;
        }
        case '[': { // Character sets
            int negated = 0;
            char set[256] = {0};

            if (*f == '^') {
                negated = 1;
                f++;
            }

            while (*f && *f != ']') {
                set[(unsigned char)*f] = 1;
                f++;
            }

            if (*f != ']') return 0; // Malformed set
            f++; // Consume the ']'

            while (*s) {
                if (negated) {
                    if (set[(unsigned char)*s]) break;
                } else {
                    if (!set[(unsigned char)*s]) break;
                }
                s++;
            }
            endptr = (char*)s;
            break;
        }
        default: return 0; // Unknown specifier
    }

    if (endptr > s_before_match) {
        *s_ptr = endptr;
        *f_ptr = f;
        return 1;
    }

    return 0;
}

// Main validation function
int strvalidate_match(const char *s, const char *f) {
    while (*f) {
        if (!match_literal(&s, &f)) return 0;
        if (*f == '%') {
            f++;
            if (!match_specifier(&s, &f)) return 0;
        }
    }

    while (isspace((unsigned char)*s)) s++;

    return *s == '\0';
}