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

#include <stdint.h>
#include <stdarg.h>

__attribute__((format(printf, 3, 4)))
uint64_t
format_to_buffer(char *buffer_in,
                 uint64_t buffer_len,
                 const char *format,
                 ...);

uint64_t
vformat_to_buffer(char *buffer_in,
                  uint64_t buffer_len,
                  const char *format,
                  va_list list);

__attribute__((format(printf, 1, 2)));
uint64_t get_length_of_printf_format(const char *fmt, ...);

uint64_t get_length_of_printf_vformat(const char *fmt, va_list list);
