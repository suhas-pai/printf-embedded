#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "parse_printf.h"

struct string_view {
    const char *begin;
    uint64_t length;
};

struct va_list_struct {
    va_list list;
};

#define OCTAL_BUFFER_LENGTH  26
#define DECIMAL_BUFFER_LENGTH 22
#define HEXADECIMAL_BUFFER_LENGTH 20
#define LARGEST_BUFFER_LENGTH OCTAL_BUFFER_LENGTH

/******* PRIVATE FUNCTIONS *******/

static inline
struct string_view sv_create_length(const char *string, const uint64_t length) {
    const struct string_view sv = {
        .begin = string,
        .length = length
    };

    return sv;
}

static inline struct string_view
sv_create_end(const char *const begin, const char *const end) {
    return sv_create_length(begin, (uint64_t)(end - begin));
}

#define SV_STATIC(c_str) sv_create_length(c_str, sizeof(c_str) - 1)

static inline struct string_view sv_create_empty() {
    return (struct string_view){ .begin = NULL, .length = 0 };
}

static struct string_view sv_drop_front(const struct string_view sv) {
    if (sv.length != 0) {
        return sv_create_length(sv.begin + 1, sv.length - 1);
    }

    return sv_create_empty();
}

static const char *const lower_alphadigit_string = "0123456789abcdef";
static const char *const upper_alphadigit_string = "0123456789ABCDEF";

enum numeric_base {
    NUMERIC_BASE_2 = 2,
    NUMERIC_BASE_8 = 8,
    NUMERIC_BASE_10 = 10,
    NUMERIC_BASE_16 = 16,
};

static inline struct string_view
unsigned_to_string_view(uint64_t number,
                        const enum numeric_base base,
                        char buffer_in[static const LARGEST_BUFFER_LENGTH],
                        const bool uppercase,
                        const bool include_prefix,
                        const bool add_pos_sign)
{
    // Subtract one from the buffer-size to convert ordinal to index.

    int i = (LARGEST_BUFFER_LENGTH - 1);
    const char *const alphadigit_string =
        (uppercase) ? upper_alphadigit_string : lower_alphadigit_string;

    buffer_in[i] = '\0';
    i--;

    do {
        buffer_in[i] = alphadigit_string[number % base];
        if (number < base) {
            break;
        }

        i--;
        number /= base;
    } while (true);

    if (include_prefix) {
        buffer_in[i - 2] = '0';
        switch (base) {
            case NUMERIC_BASE_2:
                buffer_in[i - 1] = 'b';
                break;
            case NUMERIC_BASE_8:
                buffer_in[i - 1] = 'o';
                break;
            case NUMERIC_BASE_10:
                break;
            case NUMERIC_BASE_16:
                buffer_in[i - 1] = 'x';
                break;
        }

        i -= 2;
    }

    if (add_pos_sign) {
        i -= 1;
        buffer_in[i] = '+';
    }

    /* Make end point to the null-terminator */
    const char *const end = buffer_in + (LARGEST_BUFFER_LENGTH - 1);
    return sv_create_end(buffer_in + i, end);
}

static struct string_view
convert_neg_64int_to_string(int64_t number,
                            const enum numeric_base base,
                            char buffer_in[static const LARGEST_BUFFER_LENGTH],
                            const bool uppercase,
                            const bool include_prefix)
{
    // Subtract one from the buffer-size to convert ordinal to index.

    int i = (LARGEST_BUFFER_LENGTH - 1);
    const char *const alphadigit_string =
        (uppercase) ? upper_alphadigit_string : lower_alphadigit_string;

    buffer_in[i] = '\0';
    i--;

    const int64_t neg_base = 0 - (int64_t)base;
    do {
        buffer_in[i] = alphadigit_string[0 - (number % base)];
        if (number > neg_base) {
            break;
        }

        i--;
        number /= base;
    } while (true);

    if (include_prefix) {
        buffer_in[i - 2] = '0';
        switch (base) {
            case NUMERIC_BASE_2:
                buffer_in[i - 1] = 'b';
                break;
            case NUMERIC_BASE_8:
                buffer_in[i - 1] = 'o';
                break;
            case NUMERIC_BASE_10:
                break;
            case NUMERIC_BASE_16:
                buffer_in[i - 1] = 'x';
                break;
        }

        i -= 2;
    }

    buffer_in[i - 1] = '-';
    i -= 1;

    /* Make end point to the null-terminator */
    const char *const end = buffer_in + (LARGEST_BUFFER_LENGTH - 1);
    return sv_create_end(buffer_in + i, end);
}

static inline struct string_view
signed_to_string_view(const int64_t number,
                      const enum numeric_base base,
                      char buffer_in[static const LARGEST_BUFFER_LENGTH],
                      const bool uppercase,
                      const bool include_prefix,
                      const bool add_pos_sign)
{
    struct string_view result = {};
    if (number > 0) {
        result =
            unsigned_to_string_view(number,
                                    base,
                                    buffer_in,
                                    uppercase,
                                    include_prefix,
                                    add_pos_sign);
    } else {
        result =
            convert_neg_64int_to_string(number,
                                        base,
                                        buffer_in,
                                        uppercase,
                                        include_prefix);
    }

    return result;
}

static bool
parse_flags(struct printf_spec_info *const curr_spec,
            const char *iter,
            const char **const iter_out)
{
    do {
        switch (*iter) {
            case ' ':
                curr_spec->add_one_space_for_sign = true;
                goto check_iter;
            case '-':
                curr_spec->left_justify = true;
                goto check_iter;
            case '+':
                curr_spec->add_pos_sign = true;
                goto check_iter;
            case '#':
                curr_spec->add_base_prefix = true;
                goto check_iter;
            case '0':
                curr_spec->leftpad_zeros = true;
                goto check_iter;
            check_iter:
                iter++;
                if (*iter == '\0') {
                    return false;
                }

                continue;
        }

        break;
    } while (true);

    *iter_out = iter;
    return true;
}

static inline int
read_int_from_fmt_string(const char *const c_str, const char **const iter_out) {
    int result = 0;
    const char *iter = c_str;

    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        const uint8_t digit = (ch - '0');
        if (digit >= 10) {
            *iter_out = iter;
            break;
        }

        if (__builtin_mul_overflow(result, 10, &result)) {
            *iter_out = iter;
            return -1;
        }

        if (__builtin_add_overflow(result, digit, &result)) {
            *iter_out = iter;
            return -1;
        }
    }

    return result;
}

static bool
parse_width(struct printf_spec_info *const curr_spec,
            struct va_list_struct *const list_struct,
            const char *iter,
            const char **const iter_out)
{
    if (*iter != '*') {
        const int width = read_int_from_fmt_string(iter, &iter);
        if (width == -1) {
            return false;
        }

        curr_spec->width = (uint32_t)width;
    } else {
        const int value = va_arg(list_struct->list, int);
        curr_spec->width = value >= 0 ? (uint32_t)value : 0;

        iter++;
    }

    if (*iter == '\0') {
        // If we have an incomplete spec, then we exit without writing
        // anything.

        return false;
    }

    *iter_out = iter;
    return true;
}

static bool
parse_precision(struct printf_spec_info *const curr_spec,
                const char *iter,
                struct va_list_struct *const list_struct,
                const char **const iter_out)
{
    curr_spec->precision = -1;
    if (*iter != '.') {
        goto done;
    }

    iter++;
    switch (*iter) {
        case '\0':
            return false;
        case '*':
            // Don't bother reading if we have an incomplete spec
            if (iter[1] == '\0') {
                return false;
            }

            curr_spec->precision = va_arg(list_struct->list, int);
            iter++;

            break;
        default: {
            curr_spec->precision = read_int_from_fmt_string(iter, &iter);
            if (curr_spec->precision == -1) {
                return false;
            }

            if (*iter == '\0') {
                return false;
            }

            break;
        }
    }

done:
    *iter_out = iter;
    return true;
}

static bool
parse_length(struct printf_spec_info *const curr_spec,
             const char *iter,
             const char **const iter_out,
             struct va_list_struct *const list_struct,
             uint64_t *const number_out,
             bool *const is_zero_out)
{
    switch (*iter) {
        case '\0':
            // If we have an incomplete spec, then we exit without writing
            // anything.
            return false;
        case 'h': {
            iter++;
            switch (*iter) {
                case '\0':
                    // If we have an incomplete spec, then we exit without
                    // writing anything.
                    return false;
                case 'h': {
                    const uint64_t number =
                        (uint64_t)(signed char)va_arg(list_struct->list, int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_info = iter - 1;
                    iter++;
                    break;
                }
                default: {
                    const uint64_t number =
                        (uint64_t)(short int)va_arg(list_struct->list, int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_info = iter - 1;
                    break;
                }
            }

            break;
        }
        case 'l':
            iter++;
            switch (*iter) {
                case '\0':
                    return false;
                case 'l': {
                    const uint64_t number =
                        (uint64_t)va_arg(list_struct->list, long long int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_info = iter - 1;
                    iter++;

                    break;
                }
                default: {
                    const uint64_t number =
                        (uint64_t)va_arg(list_struct->list, long int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_info = iter - 1;
                    break;
                }
            }

            break;
        case 'j': {
            const uint64_t number =
                (uint64_t)va_arg(list_struct->list, intmax_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_info = iter;
            iter++;

            break;
        }
        case 'z': {
            const uint64_t number = (uint64_t)va_arg(list_struct->list, size_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_info = iter;
            iter++;

            break;
        }
        case 't': {
            const uint64_t number =
                (uint64_t)va_arg(list_struct->list, ptrdiff_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_info = iter;
            iter++;

            break;
        }
        default:
            curr_spec->length_info = NULL;
            break;
    }

    *iter_out = iter;
    return true;
}

enum handle_spec_result {
    E_HANDLE_SPEC_OK,
    E_HANDLE_SPEC_REACHED_END,
    E_HANDLE_SPEC_CONTINUE
};

static enum handle_spec_result
handle_spec(struct printf_spec_info *const curr_spec,
            const char *const iter,
            char *const buffer,
            uint64_t number,
            struct va_list_struct *const list_struct,
            const uint64_t written_out,
            const char **const unformatted_start_out,
            struct string_view *const parsed_out,
            bool *const is_zero_out,
            bool *const is_null_out)
{
    switch (curr_spec->spec) {
        case '\0':
            return E_HANDLE_SPEC_REACHED_END;
        case 'd':
        case 'i':
            if (curr_spec->length_info == NULL) {
                number = (uint64_t)va_arg(list_struct->list, int);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                signed_to_string_view((int64_t)number,
                                      NUMERIC_BASE_10,
                                      buffer,
                                      /*uppercase=*/false,
                                      /*include_prefix=*/false,
                                      curr_spec->add_pos_sign);
            break;
        case 'u':
            if (curr_spec->length_info == NULL) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_10,
                                        buffer,
                                        /*uppercase=*/false,
                                        /*include_prefix=*/false,
                                        /*add_pos_sign=*/false);
            break;
        case 'o':
            if (curr_spec->length_info == NULL) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = (number == 0);
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_8,
                                        buffer,
                                        /*uppercase=*/false,
                                        /*include_prefix=*/false,
                                        /*add_pos_sign=*/false);
            break;
        case 'x':
            if (curr_spec->length_info == NULL) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_16,
                                        buffer,
                                        /*uppercase=*/false,
                                        /*include_prefix=*/false,
                                        /*add_pos_sign=*/false);
            break;
        case 'X': {
            if (curr_spec->length_info == NULL) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_16,
                                        buffer,
                                        /*uppercase=*/true,
                                        /*include_prefix=*/false,
                                        /*add_pos_sign=*/false);
            break;
        }
        case 'c':
            buffer[0] = (char)va_arg(list_struct->list, int);
            *parsed_out = sv_create_length(buffer, 1);

            break;
        case 's': {
            const char *const str = va_arg(list_struct->list, const char *);
            if (str != NULL) {
                uint64_t length = 0;
                if (curr_spec->precision != -1) {
                    length = strnlen(str, (size_t)curr_spec->precision);
                } else {
                    length = strlen(str);
                }

                *parsed_out = sv_create_length(str, length);
            } else {
                *parsed_out = SV_STATIC("(null)");
                *is_null_out = true;
            }

            break;
        }
        case 'p': {
            const void *const arg = va_arg(list_struct->list, const void *);
            if (arg != NULL) {
                *parsed_out =
                    unsigned_to_string_view((uint64_t)arg,
                                            /*base=*/16,
                                            buffer,
                                            /*uppercase=*/true,
                                            /*include_prefix=*/true,
                                            /*add_pos_sign=*/false);
            } else {
                *parsed_out = SV_STATIC("(nil)");
                *is_null_out = true;
            }

            break;
        }
        case 'n':
            if (curr_spec->length_info == NULL) {
                *va_arg(list_struct->list, int *) = written_out;
                return E_HANDLE_SPEC_CONTINUE;
            }

            switch (*curr_spec->length_info) {
                case 'h':
                    // case 'hh'
                    if (curr_spec->length_info[1] == 'h') {
                        *va_arg(list_struct->list, signed char *) =
                            written_out;
                        return E_HANDLE_SPEC_CONTINUE;
                    } else {
                        *va_arg(list_struct->list, short int *) = written_out;
                        return E_HANDLE_SPEC_CONTINUE;
                    }

                    break;
                case 'l':
                    // case 'll'
                    if (curr_spec->length_info[1] == 'l') {
                        *va_arg(list_struct->list, signed char *) =
                            written_out;

                        return E_HANDLE_SPEC_CONTINUE;
                    } else {
                        *va_arg(list_struct->list, long int *) =
                            (long int)written_out;

                        return E_HANDLE_SPEC_CONTINUE;
                    }

                    break;
                case 'j':
                    *va_arg(list_struct->list, intmax_t *) =
                        (intmax_t)written_out;
                    return E_HANDLE_SPEC_CONTINUE;
                case 'z':
                    *va_arg(list_struct->list, size_t *) = written_out;
                    return E_HANDLE_SPEC_CONTINUE;
                case 't':
                    *va_arg(list_struct->list, ptrdiff_t *) =
                        (ptrdiff_t)written_out;
                    return E_HANDLE_SPEC_CONTINUE;
            }

            break;
        case '%':
            buffer[0] = '%';
            *parsed_out = sv_create_length(buffer, 1);

            break;
        default:
            if (curr_spec->spec >= '0' && curr_spec->spec <= '9') {
                *unformatted_start_out = iter + 1;
                return E_HANDLE_SPEC_CONTINUE;
            }

            break;
    }

    return E_HANDLE_SPEC_OK;
}

static inline bool is_int_specifier(const char spec) {
    switch (spec) {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            return true;
    }

    return false;
}

static inline uint64_t
write_prefix_for_spec(struct printf_spec_info *const info,
                      const printf_write_char_callback_t write_ch_cb,
                      void *const ch_cb_info,
                      const printf_write_string_callback_t write_sv_cb,
                      void *const sv_cb_info,
                      bool *const cont_out)
{
    uint64_t out = 0;
    if (!info->add_base_prefix) {
        return out;
    }

    switch (info->spec) {
        case 'o':
            out += write_ch_cb(info, ch_cb_info, '0', /*times=*/1, cont_out);
            break;
        case 'x':
            out += write_sv_cb(info, sv_cb_info, "0x", /*length=*/2, cont_out);
            break;
        case 'X':
            out += write_sv_cb(info, sv_cb_info, "0X", /*length=*/2, cont_out);
            break;
    }

    return out;
}

static uint64_t
pad_with_lead_zeros(struct printf_spec_info *const info,
                    struct string_view *const parsed,
                    const uint64_t zero_count,
                    const bool is_null,
                    const printf_write_char_callback_t write_char_cb,
                    void *const cb_info,
                    const printf_write_string_callback_t write_sv_cb,
                    void *const write_sv_cb_info,
                    bool *const cont_out)
{
    uint64_t out = 0;
    if (!is_null) {
        out +=
            write_prefix_for_spec(info,
                                  write_char_cb,
                                  cb_info,
                                  write_sv_cb,
                                  write_sv_cb_info,
                                  cont_out);
    }

    if (zero_count == 0) {
        return out;
    }

    const char front = *parsed->begin;
    if (front == '+') {
        // We only have a positive sign when one is requested.
        out += write_char_cb(info, cb_info, '+', /*amount=*/1, cont_out);
        if (!cont_out) {
            return out;
        }

        *parsed = sv_drop_front(*parsed);
    } else if (front == '-') {
        out += write_char_cb(info, cb_info, '-', /*amount=*/1, cont_out);
        if (!cont_out) {
            return out;
        }

        *parsed = sv_drop_front(*parsed);
    }

    out += write_char_cb(info, cb_info, '0', zero_count, cont_out);
    if (!cont_out) {
        return out;
    }

    return out;
}

static inline uintptr_t
call_cb(const struct printf_spec_info *const spec_info,
        const struct string_view sv,
        const printf_write_char_callback_t char_cb,
        void *const char_cb_info,
        const printf_write_string_callback_t string_cb,
        void *const sv_cb_info,
        bool *const should_cont_in)
{
    if (sv.length == 0) {
        return 0;
    }

    if (sv.length != 1) {
        const uintptr_t result =
             string_cb(spec_info,
                       sv_cb_info,
                       sv.begin,
                       sv.length,
                       should_cont_in);
        return result;
    }

    const char front = sv.begin[0];
    return char_cb(spec_info, char_cb_info, front, 1, should_cont_in);
}

/******* PUBLIC FUNCTIONS *******/

uint64_t
parse_printf_format(const printf_write_char_callback_t write_char_cb,
                    void *const write_char_cb_info,
                    const printf_write_string_callback_t write_string_cb,
                    void *const write_string_cb_info,
                    const char *const fmt,
                    va_list list)
{
    struct va_list_struct list_struct = {};
    va_copy(list_struct.list, list);

    // Add 2 for a int-prefix, and one for a sign.
    char buffer[LARGEST_BUFFER_LENGTH] = {};

    struct printf_spec_info curr_spec = {};
    const char *unformatted_start = fmt;

    uint64_t written_out = 0;
    bool should_continue = true;

    for (const char *iter = strchr(fmt, '%');
         iter != NULL;
         iter = strchr(iter, '%'))
    {
        const struct string_view unformatted =
            sv_create_end(unformatted_start, iter);

        curr_spec = (struct printf_spec_info){};
        written_out +=
            call_cb(&curr_spec,
                    unformatted,
                    write_char_cb,
                    write_char_cb_info,
                    write_string_cb,
                    write_string_cb_info,
                    &should_continue);

        if (!should_continue) {
            break;
        }

        iter++;
        if (*iter == '\0') {
            // If we only got a percent sign, then we don't print anything
            return written_out;
        }

        // Format is %[flags][width][.precision][length]specifier
        if (!parse_flags(&curr_spec, iter, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            return written_out;
        }

        if (!parse_width(&curr_spec, &list_struct, iter, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            return written_out;
        }

        if (!parse_precision(&curr_spec, iter, &list_struct, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            return written_out;
        }

        // Parse length
        uint64_t number = 0;
        bool is_zero = false;

        if (!parse_length(&curr_spec,
                          iter,
                          &iter,
                          &list_struct,
                          &number,
                          &is_zero))
        {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            return written_out;
        }

        // Parse specifier
        struct string_view parsed = {};
        bool is_null = false;

        curr_spec.spec = *iter;
        unformatted_start = iter + 1;

        const enum handle_spec_result handle_spec_result =
            handle_spec(&curr_spec,
                        iter,
                        buffer,
                        number,
                        &list_struct,
                        written_out,
                        &unformatted_start,
                        &parsed,
                        &is_zero,
                        &is_null);

        switch (handle_spec_result) {
            case E_HANDLE_SPEC_OK:
                break;
            case E_HANDLE_SPEC_REACHED_END:
                return written_out;
            case E_HANDLE_SPEC_CONTINUE:
                continue;
        }


        /* Move past specifier */
        iter++;

        uint32_t padded_zero_count = 0;
        uint8_t parsed_length = parsed.length;

        // is_zero being true implies spec is an integer.
        // We don't write anything if we have a '0' and precision is 0.

        const bool should_write_parsed = !(is_zero && curr_spec.precision == 0);
        if (!should_write_parsed) {
            parsed_length = 0;
        } else if (curr_spec.add_base_prefix) {
            switch (curr_spec.spec) {
                case 'o':
                    parsed_length += 1;
                    break;
                case 'x':
                case 'X':
                    parsed_length += 2;
                    break;
            }
        }

        // If we're not wider than the specified width, we have to pad with
        // either spaces or zeroes.

        uint32_t space_pad_count = 0;
        if (is_int_specifier(curr_spec.spec)) {
            if (curr_spec.precision != -1) {
                // The case for the string-spec was already handled above
                // Total digit count doesn't include the sign/prefix.

                uint8_t total_digit_count = parsed.length;
                if (*parsed.begin == '-' || *parsed.begin == '+') {
                    total_digit_count -= 1;
                }

                if (total_digit_count < curr_spec.precision) {
                    padded_zero_count =
                        (uint32_t)curr_spec.precision - total_digit_count;

                    parsed_length += padded_zero_count;
                }
            }

            if (parsed_length != 0 && curr_spec.add_one_space_for_sign) {
                // Only add a sign if we have neither a '+' or '-'
                if (*parsed.begin != '+' && *parsed.begin != '-') {
                    space_pad_count += 1;
                }
            }
        }

        if (parsed_length < curr_spec.width) {
            const bool pad_with_zeros =
                curr_spec.leftpad_zeros &&
                is_int_specifier(curr_spec.spec) &&
                curr_spec.precision == -1 &&
                !curr_spec.left_justify; // Zeros are never left-justified

            if (pad_with_zeros) {
                // We're always resetting padded_zero_count if it was set before
                padded_zero_count = curr_spec.width - parsed_length;
            } else {
                space_pad_count += curr_spec.width - parsed_length;
            }
        }

        if (curr_spec.left_justify) {
            written_out +=
                pad_with_lead_zeros(&curr_spec,
                                    &parsed,
                                    padded_zero_count,
                                    is_null,
                                    write_char_cb,
                                    write_char_cb_info,
                                    write_string_cb,
                                    write_string_cb_info,
                                    &should_continue);

            if (!should_continue) {
                return written_out;
            }

            if (should_write_parsed) {
                written_out +=
                    call_cb(&curr_spec,
                            parsed,
                            write_char_cb,
                            write_char_cb_info,
                            write_string_cb,
                            write_string_cb_info,
                            &should_continue);

                if (!should_continue) {
                    return written_out;
                }
            }

            if (space_pad_count != 0) {
                written_out +=
                    write_char_cb(&curr_spec,
                                  write_char_cb_info,
                                  ' ',
                                  space_pad_count,
                                  &should_continue);

                if (!should_continue) {
                    return written_out;
                }
            }
        } else {
            if (space_pad_count != 0) {
                written_out +=
                    write_char_cb(&curr_spec,
                                  write_char_cb_info,
                                  ' ',
                                  space_pad_count,
                                  &should_continue);

                if (!should_continue) {
                    return written_out;
                }
            }

            written_out +=
                pad_with_lead_zeros(&curr_spec,
                                    &parsed,
                                    padded_zero_count,
                                    is_null,
                                    write_char_cb,
                                    write_char_cb_info,
                                    write_string_cb,
                                    write_string_cb_info,
                                    &should_continue);

            if (!should_continue) {
                return written_out;
            }

            if (should_write_parsed) {
                written_out +=
                    call_cb(&curr_spec,
                            parsed,
                            write_char_cb,
                            write_char_cb_info,
                            write_string_cb,
                            write_string_cb_info,
                            &should_continue);

                if (!should_continue) {
                    return written_out;
                }
            }
        }
    }

    if (*unformatted_start != '\0') {
        const struct string_view unformatted =
            sv_create_length(unformatted_start, strlen(unformatted_start));

        curr_spec = (struct printf_spec_info){};
        written_out +=
            call_cb(&curr_spec,
                    unformatted,
                    write_char_cb,
                    write_char_cb_info,
                    write_string_cb,
                    write_string_cb_info,
                    &should_continue);
    }

    return written_out;
}