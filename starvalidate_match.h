//
// Created by sinj on 27/06/2025.
//


#ifndef STRVALIDATE_MATCH_H
#define STRVALIDATE_MATCH_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int has_min;
        int has_max;
        double min_val;
        double max_val;
        int has_minlen;
        int has_maxlen;
        int minlen;
        int maxlen;
    } field_constraint;

    // Performs validation of input string against format specifiers with optional constraints.
    // Returns 1 if valid, 0 if invalid.
    int strvalidate_match(const char *input, const char *format);
    int parse_constraints(const char *format, int *min_val, int *max_val);

#ifdef __cplusplus
}
#endif

#endif // STRVALIDATE_MATCH_H

