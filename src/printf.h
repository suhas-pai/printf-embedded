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

/*
 * All callbacks should return length written-out.
 * should_continue_out will by default store true.
 */

typedef uintptr_t
(*printf_write_char_callback_t)(void *info,
                                char ch,
                                uintptr_t times,
                                bool *should_continue_out);

typedef uintptr_t
(*printf_write_string_callback_t)(void *info,
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