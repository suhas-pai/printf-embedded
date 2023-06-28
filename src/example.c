/*
*/

#include "example.h"
#include "printf.h"

struct callback_info {
    char *buffer_in;
    uint64_t buffer_used;
    uint64_t buffer_size;
};

uint64_t
format_to_buffer(char *const buffer_in,
                 const uint64_t buffer_len,
                 const char *const format,
                 ...)
{
    va_list list;
    va_start(list, format);

    const uint64_t result =
        vformat_to_buffer(buffer_in, buffer_len, format, list);

    va_end(list);
    return result;
}

static uint64_t
format_to_buffer_write_ch_callback(
    const struct printf_spec_info *const spec_info,
    void *const info,
    const char ch,
    uint64_t amount,
    bool *const should_continue_out)
{
    (void)spec_info;

    struct callback_info *const cb_info = (struct callback_info *)info;
    const uint64_t old_used = cb_info->buffer_used;
    uint64_t new_used = old_used;

    if (__builtin_add_overflow(new_used, amount, &new_used)) {
        *should_continue_out = false;
        return 0;
    }

    const uint64_t buffer_size = cb_info->buffer_size;
    if (new_used >= buffer_size) {
        /*
         * Truncate amount to just the space left if we don't have enough space
         * in the buffer to copy the whole string.
         */

        amount = (buffer_size - old_used);
        *should_continue_out = false;

        cb_info->buffer_used = old_used + amount;
    } else {
        cb_info->buffer_used = new_used;
    }

    memset(cb_info->buffer_in + old_used, ch, amount);
    return amount;
}

static uint64_t
format_to_buffer_write_string_callback(
    const struct printf_spec_info *const spec_info,
    void *const info,
    const char *const string,
    const uint64_t length,
    bool *const should_continue_out)
{
    (void)spec_info;

    struct callback_info *const cb_info = (struct callback_info *)info;
    const uint64_t old_used = cb_info->buffer_used;

    uint64_t new_used = old_used;
    if (__builtin_add_overflow(new_used, length, &new_used)) {
        *should_continue_out = false;
        return 0;
    }

    const uint64_t buffer_size = cb_info->buffer_size;
    uint64_t amount = length;

    if (new_used >= buffer_size) {
        /*
         * Truncate amount to just the space left if we don't have enough space
         * in the buffer to copy the whole string.
         */

        amount = (buffer_size - old_used);
        *should_continue_out = false;

        cb_info->buffer_used = old_used + amount;
    } else {
        cb_info->buffer_used = new_used;
    }

    memcpy(cb_info->buffer_in + old_used, string, amount);
    return amount;
}

uint64_t
vformat_to_buffer(char *const buffer_in,
                  const uint64_t buffer_len,
                  const char *const format,
                  va_list list)
{
    if (buffer_len == 0) {
        return 0;
    }

    // Subtract one so that buffer can contain a null-terminator.
    struct callback_info cb_info = {
        .buffer_in = buffer_in,
        .buffer_used = 0,
        .buffer_size = buffer_len - 1
    };

    const uint64_t length =
        parse_printf_format(format_to_buffer_write_ch_callback,
                            &cb_info,
                            format_to_buffer_write_string_callback,
                            &cb_info,
                            format,
                            list);

    cb_info.buffer_in[cb_info.buffer_used] = '\0';
    return length;
}

static uint64_t
get_length_ch_callback(const struct printf_spec_info *const spec_info,
                       void *const cb_info,
                       const char ch,
                       const uint64_t times,
                       bool *const should_continue_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)ch;
    (void)times;
    (void)should_continue_out;

    return times;
}

static uint64_t
get_length_string_callback(const struct printf_spec_info *const spec_info,
                           void *const cb_info,
                           const char *const string,
                           const uint64_t length,
                           bool *const should_continue_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)string;
    (void)length;
    (void)should_continue_out;

    return length;
}

uint64_t get_length_of_printf_format(const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    const uint64_t result = get_length_of_printf_vformat(fmt, list);

    va_end(list);
    return result;
}

uint64_t get_length_of_printf_vformat(const char *const fmt, va_list list) {
    const uint64_t length =
        parse_printf_format(get_length_ch_callback,
                            /*char_cb_info=*/NULL,
                            get_length_string_callback,
                            /*string_cb_info=*/NULL,
                            fmt,
                            list);
    return length;
}