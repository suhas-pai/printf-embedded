#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "example.h"

#define check_strings(buffer, expected)                                        \
    do {                                                                       \
        if (strcmp(buffer, expected) != 0) {                                   \
            printf("MISMATCH: \"%s\" vs expected \"%s\"\n", buffer, expected); \
            exit(1);                                                           \
        }                                                                      \
    } while (false);

#define test_format_to_buffer(buffer_len, expected, str, ...)                  \
    do {                                                                       \
        int count = 0;                                                         \
        const uint64_t length =                                                \
            format_to_buffer(buffer,                                           \
                             buffer_len,                                       \
                             str "%n",                                         \
                             ##__VA_ARGS__,                                    \
                             &count);                                          \
                                                                               \
        check_strings(buffer, expected);                                       \
        assert(length == (sizeof(expected) - 1));                              \
        assert(count == (int)(sizeof(expected) - 1));                          \
        memset(buffer, '\0', sizeof(buffer));                                  \
    } while (false)

#define test_format_to_buffer_no_count(buffer_len, expected, str, ...)         \
    do {                                                                       \
        int count = 0;                                                         \
        const uint64_t length =                                                \
            format_to_buffer(buffer,                                           \
                             buffer_len,                                       \
                             str,                                              \
                             ##__VA_ARGS__,                                    \
                             &count);                                          \
                                                                               \
        assert(strcmp(buffer, expected) == 0);                                 \
        assert(length == (sizeof(expected) - 1));                              \
        memset(buffer, '\0', sizeof(buffer));                                  \
    } while (false)

int main(const int argc, const char *const argv[]) {
    (void)argc;
    (void)argv;

    char buffer[1024] = {};
    // Test invalid formats
    {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat"
            format_to_buffer(buffer, sizeof(buffer), "%");
            format_to_buffer(buffer, sizeof(buffer), "%y");
            format_to_buffer(buffer, sizeof(buffer), "%.");
            format_to_buffer(buffer, sizeof(buffer), "%.4");
            format_to_buffer(buffer, sizeof(buffer), "%.4l");
            format_to_buffer(buffer, sizeof(buffer), "%.*", 0);
            format_to_buffer(buffer, sizeof(buffer), "%.*l", 0);
            format_to_buffer(buffer, sizeof(buffer), "%5");
            format_to_buffer(buffer, sizeof(buffer), "%5l");
            format_to_buffer(buffer, sizeof(buffer), "%5ll");
            format_to_buffer(buffer, sizeof(buffer), "%l");
            format_to_buffer(buffer, sizeof(buffer), "%*", 0);
        #pragma GCC diagnostic pop "-Wformat"
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%s", "Hello, World");
        assert(strcmp(buffer, "Hello, World") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%.5s", "Hello, World");
        assert(strcmp(buffer, "Hello") == 0);
        assert(strlen(buffer) == 5);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%12s", "Hello");

        assert(strcmp(buffer, "       Hello") == 0);
        assert(strlen(buffer) == 12);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%c", 'A');
        assert(strcmp(buffer, "A") == 0);
        assert(strlen(buffer) == 1);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%b", 890134);
        assert(strcmp(buffer, "11011001010100010110") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%d", 890134);
        assert(strcmp(buffer, "890134") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%3d", 1);
        assert(strcmp(buffer, "  1") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%3b", 1);
        assert(strcmp(buffer, "  1") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%03d", 1);
        assert(strcmp(buffer, "001") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%03d", -1);
        assert(strcmp(buffer, "-01") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%03b", 1);
        assert(strcmp(buffer, "001") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%03b", -1);
        assert(strcmp(buffer, "11111111111111111111111111111111") == 0);
    }
    {
        format_to_buffer(buffer, sizeof(buffer), "%3d", -1);
        assert(strcmp(buffer, " -1") == 0);
    }

    test_format_to_buffer(sizeof(buffer), "test", "test");
    test_format_to_buffer(sizeof(buffer), " test", " %s", "test");
    test_format_to_buffer(sizeof(buffer), " test ", " %s ", "test");
    test_format_to_buffer(sizeof(buffer),
                          " test test ",
                          " %s %s ",
                          "test",
                          "test");

    test_format_to_buffer(sizeof(buffer), "5", "%d", 5);
    test_format_to_buffer(sizeof(buffer), " 42141 ", " %d ", 42141);
    test_format_to_buffer(sizeof(buffer), "    3", "%5d", 3);
    test_format_to_buffer(sizeof(buffer), "   -3", "%5d", -3);
    test_format_to_buffer(sizeof(buffer), "3    ", "%-5d", 3);
    test_format_to_buffer(sizeof(buffer), "-3   ", "%-5d", -3);
    test_format_to_buffer(sizeof(buffer), "-0003", "%05d", -3);
    test_format_to_buffer(sizeof(buffer), "00003", "%05d", 3);
    test_format_to_buffer(sizeof(buffer), "    3", "%5u", 3);
    test_format_to_buffer(sizeof(buffer), "00003", "%05u", 3);
    test_format_to_buffer(sizeof(buffer), "00003", "%.5u", 3);
    test_format_to_buffer(sizeof(buffer), "H", "%.1s", "Hi");
    test_format_to_buffer(sizeof(buffer), "   Hi", "%5s", "Hi");
    test_format_to_buffer(sizeof(buffer), "Hi   ", "%-5s", "Hi");
    test_format_to_buffer(sizeof(buffer), "    H", "%5c", 'H');
    test_format_to_buffer(sizeof(buffer), "H    ", "%-5c", 'H');
    test_format_to_buffer(sizeof(buffer), "f", "%x", 15);
    test_format_to_buffer(sizeof(buffer), "F", "%X", 15);
    test_format_to_buffer(sizeof(buffer), "0xf", "%#x", 15);
    test_format_to_buffer(sizeof(buffer), "0XF", "%#X", 15);
    test_format_to_buffer(sizeof(buffer), "0000f", "%05x", 15);
    test_format_to_buffer(sizeof(buffer), " 0000f", " %05x", 15);
    test_format_to_buffer(sizeof(buffer), " 0000F", " %05X", 15);
    test_format_to_buffer(sizeof(buffer), "0000F", "%05X", 15);
    test_format_to_buffer(sizeof(buffer), "0x00f", "%#05x", 15);
    test_format_to_buffer(sizeof(buffer), "0X00F", "%#05X", 15);
    test_format_to_buffer(sizeof(buffer), "7", "%o", 7);
    test_format_to_buffer(sizeof(buffer), "07", "%#o", 7);
    test_format_to_buffer(sizeof(buffer), "    7", "%5o", 7);
    test_format_to_buffer(sizeof(buffer), "   07", "%#5o", 7);
    test_format_to_buffer(sizeof(buffer), "h", "%c", 'h');
    test_format_to_buffer(sizeof(buffer), "    h", "%5c", 'h');
    test_format_to_buffer(sizeof(buffer), "Hello", "%s", "Hello");
    test_format_to_buffer(sizeof(buffer), " Hello ", " %s ", "Hello");
    test_format_to_buffer(sizeof(buffer), "(null)", "%s", (char *)NULL);
    test_format_to_buffer(sizeof(buffer), "(null)", "%5s", (char *)NULL);
    test_format_to_buffer(sizeof(buffer), "(nil)", "%p", NULL);
    test_format_to_buffer(sizeof(buffer), "(nil)", "%5p", NULL);
    test_format_to_buffer(sizeof(buffer), "0x1", "%p", (void *)0x1);
    test_format_to_buffer(sizeof(buffer), "0xff", "0x%x", 0xff);
    test_format_to_buffer(sizeof(buffer), " 0005", "%5.4d", 5);
    test_format_to_buffer(sizeof(buffer), "00000005", "%5.8d", 5);
    test_format_to_buffer(sizeof(buffer), "     ", "%5.0d", 0);
    test_format_to_buffer(sizeof(buffer), "-4", "% d", -4);
    test_format_to_buffer(sizeof(buffer), "+4", "%+d", 4);
    test_format_to_buffer(sizeof(buffer), "-4", "%+d", -4);
    test_format_to_buffer(sizeof(buffer), " 4", "% d",  4);
    test_format_to_buffer(sizeof(buffer), "Hel", "%.*s", 3, "Hello");
    test_format_to_buffer(sizeof(buffer), " Hel", " %.*s", 3, "Hello");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
    test_format_to_buffer(sizeof(buffer), "H    ", "%-05c", 'H');
    test_format_to_buffer(sizeof(buffer), "a", "%+x", 10);
    test_format_to_buffer(sizeof(buffer), "A", "%+X", 10);
    test_format_to_buffer(sizeof(buffer), "fffffff6", "%+x", -10);
    test_format_to_buffer(sizeof(buffer), "FFFFFFF6", "%+X", -10);
    test_format_to_buffer(sizeof(buffer), "(nil)", "%+p", NULL);
    test_format_to_buffer_no_count(sizeof(buffer), "%", "%  %",  4);
    test_format_to_buffer_no_count(sizeof(buffer), "", "%", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%0", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%#", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%#0", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%-0", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%-#0", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%-#0.", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%-#+0.", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%+", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%0.", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%0.*", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%.0", 0, "");
    test_format_to_buffer_no_count(sizeof(buffer), "", "%*.0", 0, "");
#pragma GCC diagnostic pop

    const char buffer2[] = "Hello, There";
    test_format_to_buffer(sizeof(buffer), "Hel", "%.*s", 3, buffer2);

    printf("All tests passed!\n");
}