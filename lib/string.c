#include <stdint.h>
#include <stddef.h>
#include "string.h"


size_t STR_strlen(const char *str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != 0x00) {
        len ++;
    }
    return len;
}

char* STR_strcpy(char* dest, const char* src) {
    if (!dest || !src) return NULL;
    char *temp = dest;
    int i = 0;
    while (src[i] != 0x00) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0x00;
    return temp;
}

char *STR_strncpy(char* dest, const char* src, size_t n) {
    if (!dest || !src) return NULL;
    char *temp = dest;
    size_t i;
    for (i = 0; i < n && src[i] != 0x00; i++) {
        dest[i] = src[i];
    }

    for (; i < n; i++) {
        dest[i] = 0x00;
    }

    return temp;
}

char* STR_strcat(char* dest, const char* src) {
    if (!dest || !src) return NULL;
    int dest_len = 0;
    while (dest[dest_len] != 0x00) {
        dest_len++;
    }
    int j = 0;
    while (src[j] != 0x00) {
        dest[dest_len + j] = src[j];
        j++;
    }
    dest[dest_len + j] = 0x00;
    return dest;
}


char* STR_strncat(char* dest, const char* src, size_t n) {
    if (!dest || !src) return NULL;
    size_t dest_len = STR_strlen(dest);
    size_t j = 0;
    while (j < n && src[j] != 0x00) {
        dest[dest_len + j] = src[j];
        j++;
    }
    dest[dest_len + j] = 0x00;
    return dest;
}

int STR_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    int i = 0;
    while (s1[i] == s2[i] && s1[i] != 0x00) i++;

    return s1[i] - s2[i];
}

int STR_strncmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2) return 0;
    size_t i = 0;
    for (i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0')
            return s1[i] - s2[i];
    }
    return 0;
}

const char* STR_strchr(const char* str, char c) {
    if (!str) return NULL;
    while ((*str) != 0) {
        if ((*str) == c)
            return str;
        str++;
    }

    if ((*str) == c)
        return str;
    return NULL;
}

const char* STR_strrchr(const char* str, int c) {
    if (!str) return NULL;
    const char* i = str;
    while ((*i) != 0x00) {
        i ++;
    }

    for (; i >= str; i--) {
        if (*i == c)
            return i;
    }

    return NULL;
}

const char* STR_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    int i = 0;
    while (haystack[i] != 0x00) {
        int j = 0;
        while (haystack[i+j] != 0x00 && haystack[i+j] == needle[j]) {
            j++;
            if (needle[j] == 0x00)
                return haystack + i;
        }
        i++;
    }

    return NULL;
}

static const char digits[] = "0123456789ABCDEF";

char* STR_int2str(int value, char *buffer, const int base) {
    if (!buffer) return NULL;
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    int isNegative = 0;
    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }

    int i = 0;
    do {
        buffer[i++] = digits[value % base];
        value /= base;
    } while (value > 0);

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // reverse
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = buffer[j];
        buffer[j] = buffer[k];
        buffer[k] = temp;
    }

    return buffer;
}

char* STR_64int2str(int64_t value, char *buffer, const int64_t base) {
    if (!buffer) return NULL;
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    int isNegative = 0;
    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }

    int i = 0;
    do {
        buffer[i++] = digits[value % base];
        value /= base;
    } while (value > 0);

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // reverse
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = buffer[j];
        buffer[j] = buffer[k];
        buffer[k] = temp;
    }

    return buffer;
}

char* STR_CEncode(char* str) {
    if (!str) return;

    int read_ptr = 0;
    int write_ptr = 0;

    while (str[read_ptr] != '\0') {
        // Look for the literal characters '\' followed by '5'
        if (str[read_ptr] == '\\' && str[read_ptr + 1] == '5') {
            str[write_ptr] = 0x05; // Replace with your raw special byte
            read_ptr += 2;         // Skip both input characters ('\' and '5')
            write_ptr++;           // Advance write pointer by one byte
        } else {
            // Standard character copy/shift
            str[write_ptr] = str[read_ptr];
            read_ptr++;
            write_ptr++;
        }
    }

    // Always null-terminate the shortened string
    str[write_ptr] = '\0';

    return str;
}
