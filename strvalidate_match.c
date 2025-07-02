//
// Created by sinj on 27/06/2025.
//

#include <ctype.h>
#include <string.h>
#include "starvalidate_match.h"

int parse_constraints(const char *format, int *min_val, int *max_val) {
    *min_val = -1;
    *max_val = -1;

    const char *p = strstr(format, "(");
    if (!p) return 0;

    const char *end = strstr(p, ")");
    if (!end) return 0;

    char constraint[128];
    strncpy(constraint, p + 1, end - p - 1);
    constraint[end - p - 1] = '\0';

    char *token = strtok(constraint, ",");
    while (token) {
        if (strncmp(token, "min=", 4) == 0) {
            *min_val = atoi(token + 4);
        } else if (strncmp(token, "max=", 4) == 0) {
            *max_val = atoi(token + 4);
        }
        token = strtok(NULL, ",");
    }

    return 1;
}

int strvalidate_match(const char *input, const char *format) {
    const char *p_fmt = format;
    const char *p_in = input;

    while (*p_fmt && *p_in) {
        if (*p_fmt == '%') {
            p_fmt++;
            int type = 0;
            int min_val = -1, max_val = -1;

            if (*p_fmt == 'd') {
                type = 1;
                parse_constraints(p_fmt, &min_val, &max_val);
            } else if (*p_fmt == 's') {
                type = 2;
            } else if (*p_fmt == 'w') {
                type = 3;
            }

            // Skip past specifier and any constraint text
            while (*p_fmt && *p_fmt != ' ' && *p_fmt != '%' && *p_fmt != ':' && *p_fmt != '\0') {
                if (*p_fmt == ')') {
                    p_fmt++;
                    break;
                }
                p_fmt++;
            }

            if (type == 1) {
                char num_buf[32];
                int i = 0;
                while (isdigit(*p_in) && i < 31) {
                    num_buf[i++] = *p_in++;
                }
                num_buf[i] = '\0';
                int value = atoi(num_buf);
                if ((min_val != -1 && value < min_val) || (max_val != -1 && value > max_val)) {
                    return 0;
                }
            } else if (type == 2) {
                while (*p_in && !isspace(*p_in)) p_in++;
            } else if (type == 3) {
                while (*p_in && (isalnum(*p_in) || *p_in == '_')) p_in++;
            }
        } else {
            if (*p_fmt != *p_in) {
                return 0;
            }
            p_fmt++;
            p_in++;
        }
    }

    return *p_fmt == '\0';
}