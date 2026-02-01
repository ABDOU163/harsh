#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"


bool matches_pattern(char *pattern, char *candidate) {
    // Base case: both strings empty -> match
    if (*pattern == '\0' && *candidate == '\0') {
        return true;
    }

    if (*pattern == '*') {
        // '*' matches zero characters: try skipping '*'
        if (matches_pattern(pattern + 1, candidate)) {
            return true;
        }
        // '*' matches one or more characters: consume one char from candidate
        if (*candidate != '\0' && matches_pattern(pattern, candidate + 1)) {
            return true;
        }
        return false;
    }

    if (*pattern == '?' || *pattern == *candidate) {
        if (*candidate == '\0') {
            return false; // Expected a char but found end of string
        }
        return matches_pattern(pattern + 1, candidate + 1);
    }

    return false;
}


int main(){
    printf("Recursive:\n");
    printf("%d\n", matches_pattern("*.c", "test.c")); // Should be 1 (true)
    printf("%d\n", matches_pattern("*.c", "test.h")); // Should be 0 (false)
    printf("%d\n", matches_pattern("test.?", "test.c")); // Should be 1 (true)
    printf("%d\n", matches_pattern("a*b*.c", "abwejrhwliuhrfe.c")); // Should be 1 (true)

    return 0;
}