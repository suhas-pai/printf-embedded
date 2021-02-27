/*
 * printf.h
 * © suhas pai
 */

#ifndef SUHAS_PRINTF_H
#define SUHAS_PRINTF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/******* DEFINITIONS *******/

enum printf_flag {
    PRINTF_FLAG_NONE,
    PRINTF_FLAG_LEFT_JUSTIFY     = (1 << 0),
    PRINTF_FLAG_INCLUDE_POS_SIGN = (1 << 1),
    PRINTF_FLAG_INCLUDE_PREFIX   = (1 << 2),
    PRINTF_FLAG_LEFTPAD_ZEROS    = (1 << 3),

    /*
     * Flags not derived from specifier-format.
     */

    PRINTF_FLAG_OCTAL_PREFIX        = (1 << 4),
    PRINTF_FLAG_HEXDEC_LOWER_PREFIX = (1 << 5),
    PRINTF_FLAG_HEXDEC_UPPER_PREFIX = (1 << 6),
    PRINTF_FLAG_INCLUDE_NEG_SIGN    = (1 << 7)
};

enum printf_length {
    PRINTF_LENGTH_NONE,
    PRINTF_LENGTH_SIGNED_CHAR,
    PRINTF_LENGTH_SHORT_INT,
    PRINTF_LENGTH_LONG_INT,
    PRINTF_LENGTH_LONG_LONG_INT,
    PRINTF_LENGTH_INT_MAX_T,
    PRINTF_LENGTH_SIZE_T,
    PRINTF_LENGTH_PTRDIFF_T,
    PRINTF_LENGTH_LONG_DOUBLE
};

enum printf_specifier {
    PRINTF_SPECIFIER_CHAR        = 'c',
    PRINTF_SPECIFIER_DECIMAL     = 'd',
    PRINTF_SPECIFIER_INTEGER     = 'i',
    PRINTF_SPECIFIER_OCTAL       = 'o',
    PRINTF_SPECIFIER_WRITE_COUNT = 'n',
    PRINTF_SPECIFIER_POINTER     = 'p',
    PRINTF_SPECIFIER_STRING      = 's',
    PRINTF_SPECIFIER_UNSIGNED    = 'u',
    PRINTF_SPECIFIER_HEX_LOWER   = 'x',
    PRINTF_SPECIFIER_HEX_UPPER   = 'X',

    PRINTF_SPECIFIER_PERCENT = '%'
};

struct printf_spec_info {
    uintptr_t flags;

    int width;
    int precision;

    enum printf_length length;
    enum printf_specifier spec;
};


/*
 * All callbacks should return length written-out.
 * should_continue_out will by default store true.
 */

typedef uintptr_t
(*printf_write_char_callback_t)(const struct printf_spec_info *const spec_info,
                                void *info,
                                char ch,
                                uintptr_t times,
                                bool *should_continue_out);

typedef uintptr_t
(*printf_write_string_callback_t)(const struct printf_spec_info *const spec_info,
                                  void *info,
                                  const char *string,
                                  uintptr_t length,
                                  bool *should_continue_out);

uintptr_t
parse_printf_format(printf_write_char_callback_t char_cb,
                    void *char_cb_info,
                    printf_write_string_callback_t string_cb,
                    void *string_cb_info,
                    const char *c_str,
                    va_list list);


uintptr_t
format_to_string(char *buffer_in,
                 uintptr_t buffer_len,
                 const char *format,
                 ...) __attribute__((format(printf, 3, 4)));;

uintptr_t
vformat_to_string(char *buffer_in,
                  uintptr_t buffer_len,
                  const char *format,
                  va_list list);

uintptr_t
get_length_of_printf_format(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));;

uintptr_t get_length_of_printf_vformat(const char *fmt, va_list list);

#endif /* SUHAS_PRINTF_H */