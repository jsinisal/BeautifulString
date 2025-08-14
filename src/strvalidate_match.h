//
// Created by sinj on 27/06/2025.
//
/*

This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com

*/

#ifndef STRVALIDATE_MATCH_H
#define STRVALIDATE_MATCH_H

typedef struct
{
  int has_min;
  int has_max;
  double min_val;
  double max_val;
  int has_minlen;
  int has_maxlen;
  int minlen;
  int maxlen;
} field_constraint;

int strvalidate_match(const char *input, const char *format);
int parse_constraints(const char *format, int *min_val, int *max_val);

#endif // STRVALIDATE_MATCH_H

