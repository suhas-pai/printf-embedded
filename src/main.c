/*
 * src/main.c
 * © suhas pai
 */

#include <assert.h>
#include <string.h>
#include "printf.h"

int main(const int argc, const char *const argv[]) {
    char buffer[1024] = {};
    {
        format_to_string(buffer, 1024, "%s", "Hello, World");
        assert(strcmp(buffer, "Hello, World") == 0);
    }
    {
        format_to_string(buffer, 1024, "%.5s", "Hello, World");
        assert(strcmp(buffer, "Hello") == 0);
        assert(strlen(buffer) == 5);
    }
    {
        format_to_string(buffer, 1024, "%12s", "Hello");
        assert(strcmp(buffer, "       Hello") == 0);
        assert(strlen(buffer) == 12);
    }
    {
        format_to_string(buffer, 1024, "%c", 'A');
        assert(strcmp(buffer, "A") == 0);
        assert(strlen(buffer) == 1);
    }
    {
        format_to_string(buffer, 1024, "%d", 890134);
        assert(strcmp(buffer, "890134") == 0);
    }
    {
        format_to_string(buffer, 1024, "%3d", 1);
        assert(strcmp(buffer, "  1") == 0);
    }
    {
        format_to_string(buffer, 1024, "%03d", 1);
        assert(strcmp(buffer, "001") == 0);
    }
    {
        format_to_string(buffer, 1024, "%03d", -1);
        assert(strcmp(buffer, "-01") == 0);
    }
    {
        format_to_string(buffer, 1024, "%3d", -1);
        assert(strcmp(buffer, " -1") == 0);
    }
}