#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>


size_t STR_strlen(const char *str);

char* STR_strcpy(char* dest, const char* src);

char *STR_strncpy(char* dest, const char* src, size_t n);

char* STR_strcat(char* dest, const char* src);


char* STR_strncat(char* dest, const char* src, size_t n);

int STR_strcmp(const char* s1, const char* s2);

int STR_strncmp(const char* s1, const char* s2, size_t n);

const char* STR_strchr(const char* str, char c);

const char* STR_strrchr(const char* str, int c);

const char* STR_strstr(const char* haystack, const char* needle);

char* STR_int2str(int value, char *buffer, const int base);
char* STR_64int2str(int64_t value, char *buffer, const int64_t base);

char* STR_CEncode(char* str);

int STR_str2int(const char *str);

char* STR_lpad(const char *str, char *buffer, int width, char padding);
char* STR_rpad(const char *str, char *buffer, int width, char padding);

#endif