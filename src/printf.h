/*
Copyright (c) 2023 Suhas Pai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct printf_spec_info {
    bool add_one_space_for_sign : 1;
    bool left_justify : 1;
    bool add_pos_sign : 1;
    bool add_base_prefix : 1;
    bool leftpad_zeros : 1;

    uint32_t width;
    int precision;

    const char *length_info;
    char spec;
};

/*
 * All callbacks should return length written-out.
 * should_continue_out is initialized to true.
 */

typedef uint64_t
(*printf_write_char_callback_t)(const struct printf_spec_info *const spec_info,
                                void *info,
                                char ch,
                                uint64_t times,
                                bool *should_continue_out);

typedef uint64_t
(*printf_write_string_callback_t)(const struct printf_spec_info *const spec_info,
                                  void *info,
                                  const char *string,
                                  uint64_t length,
                                  bool *should_continue_out);

uint64_t
parse_printf_format(printf_write_char_callback_t char_cb,
                    void *char_cb_info,
                    printf_write_string_callback_t string_cb,
                    void *string_cb_info,
                    const char *c_str,
                    va_list list);