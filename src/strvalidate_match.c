/*
This file is part of BeautifulString python extension library.
Developed by Juha Sinisalo
Email: juha.a.sinisalo@gmail.com
*/

#include "strvalidate_match.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define strncasecmp _strnicmp
#endif

static int match_literal(const char **s_ptr, const char **f_ptr)
{
  const char *s = *s_ptr;
  const char *f = *f_ptr;
  while (*f && *f != '%')
  {
    if (isspace((unsigned char)*f))
    {

      while (isspace((unsigned char)*s))
        s++;
      while (isspace((unsigned char)*f))
        f++;
    }
    else
    {

      if (*s != *f)
        return 0;
      s++;
      f++;
    }
  }
  *s_ptr = s;
  *f_ptr = f;
  return 1;
}

static int match_specifier(const char **s_ptr, const char **f_ptr)
{
  const char *s = *s_ptr;
  const char *f = *f_ptr;
  char *endptr = (char *)s; 
  char type = *f;
  f++; 

  if (*f == '(')
  {
    const char *start_paren = f;
    while (*f && *f != ')')
      f++;
    if (*f == ')')
    {
      f++; 
    }
    else
    {
      f = start_paren; 
    }
  }

  while (isspace((unsigned char)*s))
    s++;
  const char *s_before_match = s;
  switch (type)
  {
  case 'd':
    strtol(s, &endptr, 10);
    break;
  case 'u':
    strtoul(s, &endptr, 10);
    break;
  case 'x':
    strtol(s, &endptr, 16);
    break;
  case 'f':
    strtod(s, &endptr);
    break;
  case 'b':
  {
    if (strncasecmp(s, "true", 4) == 0)
      s += 4;
    else if (strncasecmp(s, "false", 5) == 0)
      s += 5;
    else if (*s == '1')
      s += 1;
    else if (*s == '0')
      s += 1;
    endptr = (char *)s;
    break;
  }
  case 's':
  {
    if (*s == '"' || *s == '\'')
    {
      char opening_quote = *s;
      s++;
      while (*s && *s != opening_quote)
      {
        if (*s == '\\' && *(s + 1))
          s++;
        s++;
      }

      if (*s == opening_quote)
      {
        s++; 
      }
      else
      {

        s = s_before_match; 
      }
    }
    else
    { 
      while (*s && !isspace((unsigned char)*s))
        s++;
    }
    endptr = (char *)s;
    break;
  }
  case 'w':
  { 
    while (isalnum((unsigned char)*s))
      s++;
    endptr = (char *)s;
    break;
  }
  case '[':
  { 
    int negated = 0;
    char set[256] = {0};
    if (*f == '^')
    {
      negated = 1;
      f++;
    }
    while (*f && *f != ']')
    {
      set[(unsigned char)*f] = 1;
      f++;
    }
    if (*f != ']')
      return 0; 
    f++;        
    while (*s)
    {
      if (negated)
      {
        if (set[(unsigned char)*s])
          break;
      }
      else
      {
        if (!set[(unsigned char)*s])
          break;
      }
      s++;
    }
    endptr = (char *)s;
    break;
  }
  default:
    return 0; 
  }
  if (endptr > s_before_match)
  {
    *s_ptr = endptr;
    *f_ptr = f;
    return 1;
  }
  return 0;
}

int strvalidate_match(const char *s, const char *f)
{
  while (*f)
  {
    if (!match_literal(&s, &f))
      return 0;
    if (*f == '%')
    {
      f++;
      if (!match_specifier(&s, &f))
        return 0;
    }
  }
  while (isspace((unsigned char)*s))
    s++;
  return *s == '\0';
}