/*
 * src/printf.c
 * © suhas pai
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "printf.h"

struct string_view {
    const char *string;
    uint64_t length;
};

struct va_list_struct {
    va_list list;
};

#define OCTAL_BUFFER_LENGTH  26
#define DECIMAL_BUFFER_LENGTH 22
#define HEXADECIMAL_BUFFER_LENGTH 20
#define LARGEST_BUFFER_LENGTH OCTAL_BUFFER_LENGTH

#define add_mask_value(store, mask) ((*(store)) |= (mask))
#define has_mask_value(x, mask) (((x) & (mask)) != 0)
#define remove_mask_value(store, mask) ((*(store)) &= (~(mask)))

/******* PRIVATE FUNCTIONS *******/

static inline struct string_view
sv_create_from_c_str_and_len(const char *string, const uint64_t length) {
    const struct string_view sv = {
        .string = string,
        .length = length
    };

    return sv;
}

static inline struct string_view
sv_create_from_begin_and_end(const char *const begin, const char *const end) {
    return sv_create_from_c_str_and_len(begin, (end - begin));
}

static inline enum printf_flag _get_printf_flag_from_char(const char ch) {
    switch (ch) {
        case '-':
            return PRINTF_FLAG_LEFT_JUSTIFY;
        case '+':
            return PRINTF_FLAG_INCLUDE_POS_SIGN;
        case '#':
            return PRINTF_FLAG_INCLUDE_PREFIX;
        case '0':
            return PRINTF_FLAG_LEFTPAD_ZEROS;
    }

    return PRINTF_FLAG_NONE;
}

static inline
uintptr_t _parse_printf_flags(const char *iter, const char **const iter_out) {
    uintptr_t result = 0;

    {
        enum printf_flag flag = PRINTF_FLAG_NONE;
        char ch = *iter;

        do {
            flag = _get_printf_flag_from_char(ch);
            if (flag == PRINTF_FLAG_NONE) {
                break;
            }

            add_mask_value(&result, flag);
            ch = *(++iter);
        } while (true);
    }

    if (result != 0) {
        *iter_out = iter;
    }

    return result;
}

static inline
bool _ch_is_within_range(const char ch, const char front, const char back) {
    return ((uint8_t)(ch - front) <= (back - front));
}

static const char *const lower_alphadigit_string = "0123456789abcdef";
static const char *const upper_alphadigit_string = "0123456789ABCDEF";

static inline struct string_view
_unsigned_to_string_view(uint64_t number,
                         const int base,
                         char buffer_in[static const LARGEST_BUFFER_LENGTH],
                         const bool uppercase)
{
    /*
     * Subtract one from the buffer-size to convert ordinal to index.
     */

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

    /* Make end point to the null-terminator */
    const char *const end = buffer_in + (LARGEST_BUFFER_LENGTH - 1);
    return sv_create_from_begin_and_end(buffer_in + i, end);
}

static struct string_view
_convert_neg_64int_to_string(int64_t number,
                             const int base,
                             char buffer_in[static const LARGEST_BUFFER_LENGTH],
                             const bool uppercase)
{
    /*
     * Subtract one from the buffer-size to convert ordinal to index.
     */

    int i = (LARGEST_BUFFER_LENGTH - 1);
    const char *const alphadigit_string =
        (uppercase) ? upper_alphadigit_string : lower_alphadigit_string;

    buffer_in[i] = '\0';
    i--;

    do {
        buffer_in[i] = alphadigit_string[0 - (number % base)];
        if (number > (0 - base)) {
            break;
        }

        i--;
        number /= base;
    } while (true);

    /* Make end point to the null-terminator */
    const char *const end = buffer_in + (LARGEST_BUFFER_LENGTH - 1);
    return sv_create_from_begin_and_end(buffer_in + i, end);
}

static inline struct string_view
_signed_to_string_view(const int64_t number,
                       const int base,
                       char buffer_in[static const LARGEST_BUFFER_LENGTH],
                       const bool uppercase)
{
    struct string_view result = {};
    if (number > 0) {
        result =
            _unsigned_to_string_view(number, base, buffer_in, uppercase);
    } else {
        result =
            _convert_neg_64int_to_string(number, base, buffer_in, uppercase);
    }

    return result;
}

static inline int
_read_int_from_printf_c_str(const char *const c_str,
                            const char **const iter_out)
{
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

static inline
int _read_positive_int_from_va_list(struct va_list_struct *const list_struct) {
    const int result = va_arg(list_struct->list, int);
    if (result > 0) {
        return result;
    }

    return -1;
}

static inline int
_parse_printf_width(const char *const iter,
                    struct va_list_struct *const list_struct,
                    const char **const iter_out)
{
    const char front = *iter;
    if (front == '*') {
        *iter_out = iter + 1;
        return _read_positive_int_from_va_list(list_struct);
    }

    if (!_ch_is_within_range(front, '0', '9')) {
        return -1;
    }

    return _read_int_from_printf_c_str(iter, iter_out);
}

static inline int
_parse_printf_precision(const char *iter,
                        struct va_list_struct *const list_struct,
                        const char **const iter_out)
{
    if (*iter != '.') {
        return -1;
    }

    iter++;
    if (*iter == '*') {
        *iter_out = iter + 1;
        return _read_positive_int_from_va_list(list_struct);
    }

    /* Add one to skip the '.' */
    return _read_int_from_printf_c_str(iter, iter_out);
}

static inline enum printf_length
_parse_printf_length(const char *const iter, const char **const iter_out) {
    switch (*iter) {
        case 'h': {
            if (iter[1] == 'h') {
                *iter_out = iter + 2;
                return PRINTF_LENGTH_SIGNED_CHAR;
            }

            *iter_out = iter + 1;
            return PRINTF_LENGTH_SHORT_INT;
        }
        case 'l': {
            if (iter[1] == 'l') {
                *iter_out = iter + 2;
                return PRINTF_LENGTH_LONG_LONG_INT;
            }

            *iter_out = iter + 1;
            return PRINTF_LENGTH_LONG_INT;
        }
        case 'j':
            *iter_out = iter + 1;
            return PRINTF_LENGTH_INT_MAX_T;
        case 'z':
            *iter_out = iter + 1;
            return PRINTF_LENGTH_SIZE_T;
        case 't':
            *iter_out = iter + 1;
            return PRINTF_LENGTH_PTRDIFF_T;
        case 'L':
            *iter_out = iter + 1;
            return PRINTF_LENGTH_LONG_DOUBLE;
    }

    return PRINTF_LENGTH_NONE;
}

static inline struct string_view
_convert_int_to_string_view(const uint64_t number,
                            char buffer_in[static const LARGEST_BUFFER_LENGTH],
                            const int base,
                            const bool is_signed,
                            const bool is_uppercase)
{
    struct string_view result = {};
    if (is_signed) {
        result =
            _signed_to_string_view((int64_t)number,
                                   /*base=*/base,
                                   /*buffer_in=*/buffer_in,
                                   /*uppercase=*/is_uppercase);
    } else {
        result =
            _unsigned_to_string_view(number,
                                     /*base=*/base,
                                     /*buffer_in=*/buffer_in,
                                     /*uppercase=*/is_uppercase);
    }

    return result;
}

static inline int64_t
_get_signed_integer_with_length(struct va_list_struct *const list_struct,
                                const enum printf_length length)
{
    int64_t result = 0;
    switch (length) {
        case PRINTF_LENGTH_NONE:
            result = va_arg(list_struct->list, int);
            break;
        case PRINTF_LENGTH_SIGNED_CHAR:
            result = (signed char)va_arg(list_struct->list, int);
            break;
        case PRINTF_LENGTH_SHORT_INT:
            result = (short int)va_arg(list_struct->list, int);
            break;
        case PRINTF_LENGTH_LONG_INT:
            result = va_arg(list_struct->list, long int);
            break;
        case PRINTF_LENGTH_LONG_LONG_INT:
            result = va_arg(list_struct->list, long long int);
            break;
        case PRINTF_LENGTH_INT_MAX_T:
            result = va_arg(list_struct->list, intmax_t);
            break;
        case PRINTF_LENGTH_SIZE_T:
            result = va_arg(list_struct->list, size_t);
            break;
        case PRINTF_LENGTH_PTRDIFF_T:
            result = va_arg(list_struct->list, ptrdiff_t);
            break;
        case PRINTF_LENGTH_LONG_DOUBLE:
            /* TODO: Should something be done here? */
            break;
    }

    return result;
}

static inline uint64_t
_get_unsigned_integer_with_length(struct va_list_struct *const list_struct,
                                  const enum printf_length length)
{
    uint64_t result = 0;
    switch (length) {
        case PRINTF_LENGTH_NONE:
            result = va_arg(list_struct->list, unsigned int);
            break;
        case PRINTF_LENGTH_SIGNED_CHAR:
            result = (unsigned char)va_arg(list_struct->list, unsigned int);
            break;
        case PRINTF_LENGTH_SHORT_INT:
            result =
                (unsigned short int)va_arg(list_struct->list, unsigned int);
            break;
        case PRINTF_LENGTH_LONG_INT:
            result = va_arg(list_struct->list, unsigned long int);
            break;
        case PRINTF_LENGTH_LONG_LONG_INT:
            result = va_arg(list_struct->list, unsigned long long int);
            break;
        case PRINTF_LENGTH_INT_MAX_T:
            result = va_arg(list_struct->list, uintmax_t);
            break;
        case PRINTF_LENGTH_SIZE_T:
            result = va_arg(list_struct->list, size_t);
            break;
        case PRINTF_LENGTH_PTRDIFF_T:
            result = va_arg(list_struct->list, ptrdiff_t);
            break;
        case PRINTF_LENGTH_LONG_DOUBLE:
            /* TODO: Should something be done here? */
            break;
        default:
            result = va_arg(list_struct->list, unsigned int);
            break;
    }

    return result;
}

static inline bool _is_integer_specifier(const enum printf_specifier spec) {
    switch (spec) {
        case PRINTF_SPECIFIER_DECIMAL:
        case PRINTF_SPECIFIER_INTEGER:
        case PRINTF_SPECIFIER_OCTAL:
        case PRINTF_SPECIFIER_UNSIGNED:
        case PRINTF_SPECIFIER_HEX_LOWER:
        case PRINTF_SPECIFIER_HEX_UPPER:
            return true;
        case PRINTF_SPECIFIER_CHAR:
        case PRINTF_SPECIFIER_PERCENT:
        case PRINTF_SPECIFIER_POINTER:
        case PRINTF_SPECIFIER_STRING:
        case PRINTF_SPECIFIER_WRITE_COUNT:
            break;
    }

    return false;
}

static inline struct string_view
_handle_printf_specifier(
    char buffer_in[static const OCTAL_BUFFER_LENGTH],
    struct va_list_struct *const list_struct,
    uintptr_t *const flags_in,
    const uintptr_t written_out,
    const enum printf_specifier specifier,
    const enum printf_length length)
{
    struct string_view sv = {};
    bool is_uppercase = false;
    const uintptr_t flags = *flags_in;

    switch (specifier) {
        case PRINTF_SPECIFIER_DECIMAL:
        case PRINTF_SPECIFIER_INTEGER: {
            const int64_t arg =
                _get_signed_integer_with_length(list_struct, length);

            sv =
                _convert_int_to_string_view(/*number=*/(uint64_t)arg,
                                            /*buffer_in=*/buffer_in,
                                            /*base=*/10,
                                            /*is_signed=*/true,
                                            /*is_uppercase=*/is_uppercase);
            if (arg < 0) {
                add_mask_value(flags_in, PRINTF_FLAG_INCLUDE_NEG_SIGN);
            }

            break;
        }
        case PRINTF_SPECIFIER_UNSIGNED: {
            const uint64_t arg =
                _get_unsigned_integer_with_length(list_struct, length);
            sv =
                _convert_int_to_string_view(/*number=*/arg,
                                            /*buffer_in=*/buffer_in,
                                            /*base=*/10,
                                            /*is_signed=*/false,
                                            /*is_uppercase=*/is_uppercase);

            break;
        }
        case PRINTF_SPECIFIER_OCTAL: {
            const uint64_t arg =
                _get_unsigned_integer_with_length(list_struct, length);
            sv =
                _convert_int_to_string_view(/*number=*/arg,
                                            /*buffer_in=*/buffer_in,
                                            /*base=*/8,
                                            /*is_signed=*/false,
                                            /*is_uppercase=*/is_uppercase);

            if (has_mask_value(flags, PRINTF_FLAG_INCLUDE_PREFIX)) {
                add_mask_value(flags_in, PRINTF_FLAG_OCTAL_PREFIX);
            }

            break;
        }
        case PRINTF_SPECIFIER_HEX_UPPER:
            is_uppercase = true;
            /* fallthrough */
        case PRINTF_SPECIFIER_HEX_LOWER: {
            const uint64_t arg =
                _get_unsigned_integer_with_length(list_struct, length);
            sv =
                _convert_int_to_string_view(/*number=*/arg,
                                            /*buffer_in=*/buffer_in,
                                            /*base=*/16,
                                            /*is_signed=*/false,
                                            /*is_uppercase=*/is_uppercase);

            if (has_mask_value(flags, PRINTF_FLAG_INCLUDE_PREFIX)) {
                const enum printf_flag mask =
                    (is_uppercase) ?
                        PRINTF_FLAG_HEXDEC_UPPER_PREFIX :
                        PRINTF_FLAG_HEXDEC_LOWER_PREFIX;

                add_mask_value(flags_in, mask);
            }

            break;
        }
        case PRINTF_SPECIFIER_CHAR:
            buffer_in[0] = (char)va_arg(list_struct->list, int);
            sv = sv_create_from_c_str_and_len(buffer_in, 1);

            break;
        case PRINTF_SPECIFIER_PERCENT:
            buffer_in[0] = '%';
            sv = sv_create_from_c_str_and_len(buffer_in, 1);

            break;
        case PRINTF_SPECIFIER_POINTER: {
            const uintptr_t arg = (uintptr_t)va_arg(list_struct->list, void *);
            if (arg != 0) {
                sv =
                    _convert_int_to_string_view(
                        /*number=*/arg,
                        /*buffer_in=*/buffer_in,
                        /*base=*/16,
                        /*is_signed=*/false,
                        /*is_uppercase=*/is_uppercase);

                add_mask_value(flags_in, PRINTF_FLAG_HEXDEC_LOWER_PREFIX);
            } else {
                static const char nil[] = "(nil)";
                sv = sv_create_from_c_str_and_len(nil, 5);
            }

            break;
        }
        case PRINTF_SPECIFIER_STRING: {
            const char *const arg = va_arg(list_struct->list, const char *);
            if (arg != NULL) {
                sv = sv_create_from_c_str_and_len(arg, 0);
            } else {
                static const char null[] = "(null)";
                sv = sv_create_from_c_str_and_len(null, 6);
            }

            break;
        }
        case PRINTF_SPECIFIER_WRITE_COUNT:
            *va_arg(list_struct->list, int *) = written_out;
            sv = sv_create_from_c_str_and_len(buffer_in, 0);

            break;
    }

    return sv;
}

static inline struct string_view
_write_signs_for_integer_specifier(struct string_view sv,
                                   bool *const should_continue_in,
                                   uintptr_t *const written_out,
                                   struct printf_spec_info *const spec_info,
                                   const printf_write_char_callback_t char_cb,
                                   void *const char_cb_info)
{
    if (has_mask_value(spec_info->flags, PRINTF_FLAG_INCLUDE_POS_SIGN)) {
        *written_out +=
            char_cb(spec_info, char_cb_info, '+', 1, should_continue_in);

        remove_mask_value(&spec_info->flags, PRINTF_FLAG_INCLUDE_POS_SIGN);
        if (!*should_continue_in) {
            return sv;
        }
    }

    const char spec = spec_info->spec;
    if (spec == PRINTF_SPECIFIER_DECIMAL || spec == PRINTF_SPECIFIER_INTEGER) {
        if (*sv.string == '-') {
            *written_out +=
                char_cb(spec_info, char_cb_info, '-', 1, should_continue_in);

            sv.string += 1;
        }
    }

    return sv;
}

static inline uintptr_t
_get_length_accounting_for_flags(struct string_view sv, const uintptr_t flags) {
    if (has_mask_value(flags, PRINTF_FLAG_INCLUDE_NEG_SIGN)) {
        sv.length += 1;
    } else if (has_mask_value(flags, PRINTF_FLAG_INCLUDE_POS_SIGN)) {
        sv.length += 1;
    }

    if (has_mask_value(flags, PRINTF_FLAG_OCTAL_PREFIX)) {
        return (sv.length + 1);
    } else {
        if (has_mask_value(flags, PRINTF_FLAG_HEXDEC_LOWER_PREFIX) ||
            has_mask_value(flags, PRINTF_FLAG_HEXDEC_UPPER_PREFIX))
        {
            return (sv.length + 2);
        }
    }

    return sv.length;
}

static inline uintptr_t
_handle_integer_flags(struct printf_spec_info *const spec_info,
                      bool *const should_cont_in,
                      const printf_write_char_callback_t char_cb,
                      void *const char_cb_info,
                      const printf_write_string_callback_t sv_cb,
                      void *const sv_cb_info)
{
    if (has_mask_value(spec_info->flags, PRINTF_FLAG_OCTAL_PREFIX)) {
        remove_mask_value(&spec_info->flags, PRINTF_FLAG_OCTAL_PREFIX);
        return char_cb(spec_info, char_cb_info, '0', 1, should_cont_in);
    }

    if (has_mask_value(spec_info->flags, PRINTF_FLAG_HEXDEC_LOWER_PREFIX)) {
        remove_mask_value(&spec_info->flags, PRINTF_FLAG_HEXDEC_LOWER_PREFIX);
        return sv_cb(spec_info, sv_cb_info, "0x", 2, should_cont_in);
    }

    if (has_mask_value(spec_info->flags, PRINTF_FLAG_HEXDEC_UPPER_PREFIX)) {
        remove_mask_value(&spec_info->flags,  PRINTF_FLAG_HEXDEC_UPPER_PREFIX);
        return sv_cb(spec_info, sv_cb_info, "0X", 2, should_cont_in);
    }

    return 0;
}

static inline struct string_view
_handle_printf_qualities(struct printf_spec_info *const spec_info,
                         struct string_view sv,
                         uintptr_t *const written_out,
                         bool *const should_cont_in,
                         const printf_write_char_callback_t char_cb,
                         void *const char_cb_info,
                         const printf_write_string_callback_t sv_cb,
                         void *const sv_cb_info)
{
    /*
     * We calculate the string-length at the last moment, because the string may
     * not have a null-terminator.
     */

    if (spec_info->spec == PRINTF_SPECIFIER_STRING) {
        const char *const begin = sv.string;

        /*
         * For string specifiers, the precision encodes the *maximum* amount
         * of characters allowed.
         */

        if (spec_info->precision != -1) {
            const uintptr_t length = strnlen(begin, spec_info->precision);
            sv = sv_create_from_c_str_and_len(begin, length);
        } else {
            sv = sv_create_from_c_str_and_len(begin, strlen(begin));
        }
    }

    const int width = spec_info->width;
    if (width != -1) {
        /*
         * width encodes the minimum amount of characters to be printed.
         *
         * If the minimum is not met, the difference must be made up with a
         * padding.
         *
         * The padding-char is a space, unless LEFTPAD_ZEROS has been set, in
         * which case the padding-char is '0'.
         */

        /*
         * For a padding-char of '0', the number -3 should look like "-003",
         * while a space padding-char would result in "  -3"
         */

        const uintptr_t flags = spec_info->flags;
        const uintptr_t length = _get_length_accounting_for_flags(sv, flags);

        if ((uintptr_t)width <= length) {
            return sv;
        }

        const uintptr_t amt = ((uintptr_t)width - length);
        if (has_mask_value(flags, PRINTF_FLAG_LEFTPAD_ZEROS)) {
            /*
             * Write out the integer-signs first (if we have any) so we can
             * write leading zeroes first.
             */

            if (_is_integer_specifier(spec_info->spec)) {
                sv =
                    _write_signs_for_integer_specifier(sv,
                                                       should_cont_in,
                                                       written_out,
                                                       spec_info,
                                                       char_cb,
                                                       char_cb_info);
            }

            /*
             * Integer flags are also added to PRINTF_SPECIFIER_POINTER, so
             * this function must be called outside _is_integer_specifier()
             * scope.
             */

            *written_out +=
                _handle_integer_flags(spec_info,
                                      should_cont_in,
                                      char_cb,
                                      char_cb_info,
                                      sv_cb,
                                      sv_cb_info);

            *written_out +=
                char_cb(spec_info, char_cb_info, '0', amt, should_cont_in);

            if (!*should_cont_in) {
                return sv;
            }
        } else {
            *written_out +=
                char_cb(spec_info, char_cb_info, ' ', amt, should_cont_in);

            if (!*should_cont_in) {
                return sv;
            }

            *written_out +=
                _handle_integer_flags(spec_info,
                                      should_cont_in,
                                      char_cb,
                                      char_cb_info,
                                      sv_cb,
                                      sv_cb_info);
        }

        return sv;
    }

    const enum printf_specifier spec = spec_info->spec;
    if (_is_integer_specifier(spec)) {
        const int precision = spec_info->precision;
        if (precision != -1) {
            /*
             * For integer-specifiers, precision encodes the *minimum* amount of
             * digits to be printed.
             *
             * If minimum is not met, then the string must be padded with
             * leading zeroes.
             */

            uintptr_t digit_count = sv.length;

            /*
             * The number of digits doesn't include the negative sign.
             */

            if (sv.string[0] == '-') {
                digit_count -= 1;
            }

            if (digit_count >= (uintptr_t)precision) {
                return sv;
            }

            const uintptr_t amt = ((uintptr_t)precision - digit_count);
            sv =
                _write_signs_for_integer_specifier(sv,
                                                   should_cont_in,
                                                   written_out,
                                                   spec_info,
                                                   char_cb,
                                                   char_cb_info);

            *written_out +=
                char_cb(spec_info, char_cb_info, '0', amt, should_cont_in);
        }

        *written_out +=
            _handle_integer_flags(spec_info,
                                  should_cont_in,
                                  char_cb,
                                  char_cb_info,
                                  sv_cb,
                                  sv_cb_info);
    }

    return sv;
}

static inline uintptr_t
_call_callback(const struct printf_spec_info *const spec_info,
               const struct string_view sv,
               bool *const should_cont_in,
               const printf_write_char_callback_t char_cb,
               void *const char_cb_info,
               const printf_write_string_callback_t string_cb,
               void *const sv_cb_info)
{
    if (sv.length == 0) {
        return 0;
    }

    if (sv.length != 1) {
        const uintptr_t result =
             string_cb(spec_info,
                       sv_cb_info,
                       sv.string,
                       sv.length,
                       should_cont_in);
        return result;
    }

    const char front = sv.string[0];
    return char_cb(spec_info, char_cb_info, front, 1, should_cont_in);
}

/******* PUBLIC FUNCTIONS *******/

uintptr_t
parse_printf_format(const printf_write_char_callback_t char_cb,
                    void *const char_cb_info,
                    const printf_write_string_callback_t string_cb,
                    void *const string_cb_info,
                    const char *const c_str,
                    va_list list)
{
    /*
     * Keep a buffer to store conversions to string that can be passed to
     * callback.
     */

    char conv_buffer[LARGEST_BUFFER_LENGTH] = {};

    /*
     * We need to store va_list in a struct to pass it by reference.
     * Source: https://stackoverflow.com/a/8047513
     */

    struct va_list_struct list_struct = {};
    va_copy(list_struct.list, list);

    /*
     * Store a range (pointer and length) to a portion of the string that is
     * unformatted and can just be passed to the callback.
     */

    const char *unformat_buffer_ptr = c_str;
    bool should_continue = true;

    uintptr_t unformat_buffer_length = 0;
    uintptr_t written_out = 0;

    const char *iter = c_str;
    struct printf_spec_info spec_info = {};

    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        if (ch != '%') {
            unformat_buffer_length += 1;
            continue;
        }

        const struct string_view unformat_buffer_sv =
            sv_create_from_c_str_and_len(unformat_buffer_ptr,
                                         unformat_buffer_length);

        written_out +=
            _call_callback(&spec_info,
                           unformat_buffer_sv,
                           &should_continue,
                           char_cb,
                           char_cb_info,
                           string_cb,
                           string_cb_info);

        if (!should_continue) {
            goto done;
        }

        /*
         * Keep a pointer to the start of the format, because if this
         * format-specifier is invalid, we'll need to treat it as an ordinary
         * unformatted string.
         */

        const char *const orig_iter = iter;

        /* Skip past the '%' char */
        iter++;

        spec_info.flags = _parse_printf_flags(iter, &iter);
        spec_info.width = _parse_printf_width(iter, &list_struct, &iter);
        spec_info.precision =
            _parse_printf_precision(iter, &list_struct, &iter);

        spec_info.length = _parse_printf_length(iter, &iter);
        spec_info.spec = *iter;

        struct string_view sv =
            _handle_printf_specifier(conv_buffer,
                                     &list_struct,
                                     &spec_info.flags,
                                     written_out,
                                     spec_info.spec,
                                     spec_info.length);

        sv =
            _handle_printf_qualities(&spec_info,
                                     sv,
                                     &written_out,
                                     &should_continue,
                                     char_cb,
                                     char_cb_info,
                                     string_cb,
                                     string_cb_info);

        if (!should_continue) {
            goto done;
        }

        /*
         * The string pointer of the string-view will be set to NULL if the
         * format-specifier was invalid.
         */

        if (sv.string != NULL) {
            written_out +=
                _call_callback(&spec_info,
                               sv,
                               &should_continue,
                               char_cb,
                               char_cb_info,
                               string_cb,
                               string_cb_info);

            if (!should_continue) {
                goto done;
            }

            /*
            * Reset the unformat-buffer having written out a
            * formatted-string.
            */

            unformat_buffer_ptr = iter + 1;
            unformat_buffer_length = 0;
        } else {
            /*
             * Invalid format-specifiers are treated as otherwise unformatted
             * strings.
             */

            unformat_buffer_ptr = orig_iter;
            unformat_buffer_length = (iter - orig_iter) + 1;
        }
    }

    if (unformat_buffer_length != 0) {
        const struct string_view sv =
            sv_create_from_c_str_and_len(unformat_buffer_ptr,
                                         unformat_buffer_length);

        written_out +=
            _call_callback(&spec_info,
                           sv,
                           &should_continue,
                           char_cb,
                           char_cb_info,
                           string_cb,
                           string_cb_info);
    }

done:
    return written_out;
}

struct callback_info {
    char *buffer_in;
    uintptr_t buffer_used;
    uintptr_t buffer_size;
};

uintptr_t
format_to_string(char *const buffer_in,
                 const uintptr_t buffer_len,
                 const char *const format,
                 ...)
{
    va_list list;
    va_start(list, format);

    const uintptr_t result =
        vformat_to_string(buffer_in, buffer_len, format, list);

    va_end(list);
    return result;
}

static uintptr_t
_format_to_string_write_ch_callback(
    __unused const struct printf_spec_info *const spec_info,
    __unused void *const info,
    const char ch,
    uintptr_t amount,
    bool *const should_continue_out)
{
    struct callback_info *const cb_info = (struct callback_info *)info;
    const uintptr_t old_used = cb_info->buffer_used;
    uintptr_t new_used = old_used;

    if (__builtin_add_overflow(new_used, amount, &new_used)) {
        *should_continue_out = false;
        return 0;
    }

    const uintptr_t buffer_size = cb_info->buffer_size;
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

static uintptr_t
_format_to_string_write_string_callback(
    __unused const struct printf_spec_info *const spec_info,
    __unused void *const info,
    const char *const string,
    const uintptr_t length,
    bool *const should_continue_out)
{
    struct callback_info *const cb_info = (struct callback_info *)info;
    const uintptr_t old_used = cb_info->buffer_used;

    uintptr_t new_used = old_used;
    if (__builtin_add_overflow(new_used, length, &new_used)) {
        *should_continue_out = false;
        return 0;
    }

    const uintptr_t buffer_size = cb_info->buffer_size;
    uintptr_t amount = length;

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

uintptr_t
vformat_to_string(char *const buffer_in,
                  const uintptr_t buffer_len,
                  const char *const format,
                  va_list list)
{
    if (buffer_len == 0) {
        return 0;
    }

    /*
     * Subtract one so that buffer can contain a null-terminator.
     */

    struct callback_info cb_info = {
        .buffer_in = buffer_in,
        .buffer_used = 0,
        .buffer_size = buffer_len - 1
    };

    const uintptr_t length =
        parse_printf_format(_format_to_string_write_ch_callback,
                            &cb_info,
                            _format_to_string_write_string_callback,
                            &cb_info,
                            format,
                            list);

    cb_info.buffer_in[cb_info.buffer_used] = '\0';
    return length;
}

static uintptr_t
_get_length_ch_callback(
    __unused const struct printf_spec_info *const spec_info,
    __unused void *const cb_info,
    __unused const char ch,
    __unused const uintptr_t times,
    __unused bool *const should_continue_out)
{
    return times;
}

static uintptr_t
_get_length_string_callback(
    __unused const struct printf_spec_info *const spec_info,
    __unused void *const cb_info,
    __unused const char *const string,
    const uintptr_t length,
    __unused bool *const should_continue_out)
{
    return length;
}

uintptr_t get_length_of_printf_format(const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    const uintptr_t result = get_length_of_printf_vformat(fmt, list);

    va_end(list);
    return result;
}

uintptr_t get_length_of_printf_vformat(const char *const fmt, va_list list) {
    const uintptr_t length =
        parse_printf_format(_get_length_ch_callback,
                            NULL,
                            _get_length_string_callback,
                            NULL,
                            fmt,
                            list);
    return length;
}