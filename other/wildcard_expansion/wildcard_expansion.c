#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include <sys/types.h>
#include <dirent.h>


bool is_valid_match(char *pattern, char *candidate) {
    // Base case: both strings empty -> match
    if (*pattern == '\0' && *candidate == '\0') {
        return true;
    }

    if (*pattern == '*') {
        // '*' matches zero characters: try skipping '*'
        if (is_valid_match(pattern + 1, candidate)) {
            return true;
        }
        // '*' matches one or more characters: consume one char from candidate
        if (*candidate != '\0' && is_valid_match(pattern, candidate + 1)) {
            return true;
        }
        return false;
    }

    if (*pattern == '?' || *pattern == *candidate) {
        if (*candidate == '\0') {
            return false; // Expected a char but found end of string
        }
        return is_valid_match(pattern + 1, candidate + 1);
    }

    return false;
}

char** get_matches(char **dir_paths, char* pattern, unsigned char d_type){
    size_t i=0;
    char **matches = malloc(sizeof(char*)*(i+1));
    *matches = (char*)NULL;
    char **p;
    for (p=dir_paths; *p != NULL; p++){
        DIR *dir = opendir(*p);
        if (!dir){
            fprintf(stderr, "Can't open directory: %s\n", *p);
            continue;
        }
        struct dirent *entry = readdir(dir);
        if (!entry){
            fprintf(stderr, "Can't read directory: %s\n", *p);
            continue;
        }

        while (entry){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                entry = readdir(dir);
                continue;
            }
            if (entry->d_type == d_type && is_valid_match(pattern, entry->d_name)){
                matches[i] = strdup(*p);
                matches[i] = realloc(matches[i], strlen(*p) + strlen(entry->d_name) + 1);
                strcat(matches[i], entry->d_name);
                i++;
                matches = realloc(matches, sizeof(char*)*(i+1));
                matches[i] = (char*)NULL;
            }
            entry = readdir(dir);
        }
        closedir(dir);

    }
    return matches; 
}

void append_path(char **dir_paths, char *rest){
    char **p;
    for (p=dir_paths; *p!= NULL; p++){
        *p= realloc(*p, strlen(*p) + strlen(rest)+1);
        strcat(*p, rest);
    }
}

char **glob(char *path){
    // path is something like custom*/*.c, will give in matches all c files in custom_shell_in_c

    // init result 
    // heap allocated array of strings
    char **matches = malloc(sizeof(char **));
    *matches = (char*)NULL;
    size_t count = 0;
    // start and end are used to determine the patter one at a time
    char *start = path;
    char *end = NULL;
    // dir_paths is the one we have as opendir argument
    char *special_occ = strpbrk(start, "*?");
    if (!special_occ){
        *matches = strdup(start);
        matches = realloc(matches, sizeof(char*)*2);
        matches[1] = NULL;
        return matches;
    }
    free(matches);
    char **dir_paths = malloc(sizeof(char*) * 2);
    dir_paths[1] = (char*)NULL;
    char c = *special_occ;
    *special_occ  = '\0';
    char *slash_occ = strrchr(start, '/');
    *special_occ = c;
    if (slash_occ){
        c = slash_occ[1];
        slash_occ[1] = '\0';  
        *dir_paths=  strdup(start);
        slash_occ[1] = c;
        start = slash_occ;
    } else{
        *dir_paths=  strdup("./");
    }

    char *next_slash = strchr(special_occ, '/');

    unsigned char d_type = (next_slash == NULL) ? DT_REG | DT_DIR : DT_DIR;
    char *pattern=slash_occ ? slash_occ+1 : start;
    end = next_slash;
    if (!end){
        // No more path segments, return final matches
        return get_matches(dir_paths, pattern, d_type);
    }
    *end = '\0';
    char **new = get_matches(dir_paths, pattern, d_type);
    // loop to free old dir_paths
    for (char **p = dir_paths; *p != NULL; p++){
        free(*p);
    }
    free(dir_paths);
    dir_paths = new;
    *end = '/';
    while (next_slash && dir_paths && *dir_paths != NULL){
        start = next_slash ;
        char *special_occ = strpbrk(start, "*?");
        if (!special_occ){
            append_path(dir_paths, start);
            return dir_paths;
        }
        char c = *special_occ;
        *special_occ  = '\0';
        char *slash_occ = strrchr(start, '/');
        *special_occ = c;
        c = slash_occ[1];
        slash_occ[1] = '\0';  
        append_path(dir_paths, start);
        slash_occ[1] = c;
        start = slash_occ;

        next_slash = strchr(special_occ, '/');  // Update outer next_slash, don't shadow it
        unsigned char d_type = (next_slash == NULL) ? DT_REG | DT_DIR : DT_DIR;
        char *pattern=slash_occ ? slash_occ+1 : start;
        end = next_slash;
        if (end){
            *end='\0';
        }
        char **new = get_matches(dir_paths, pattern, d_type);
        // loop to free old dir_paths
        for (char **p = dir_paths; *p != NULL; p++){
            free(*p);
        }
        free(dir_paths);
        dir_paths = new;
        if (end){
            *end='/';
        }
    }
    return dir_paths;
}

// Helper to free glob results
void free_glob_result(char **matches) {
    if (matches) {
        for (int i = 0; matches[i] != NULL; i++) {
            free(matches[i]);
        }
        free(matches);
    }
}

// Helper to count and print glob results
int print_glob_results(char **matches, const char *test_name) {
    int count = 0;
    printf("\n=== %s ===\n", test_name);
    if (matches) {
        for (int i = 0; matches[i] != NULL; i++) {
            printf("  [%d]: %s\n", i, matches[i]);
            count++;
        }
    }
    printf("  Total matches: %d\n", count);
    return count;
}

int test_main() {
    int passed = 0;
    int failed = 0;

    printf("========================================\n");
    printf("    COMPREHENSIVE GLOB FUNCTION TESTS   \n");
    printf("========================================\n");


    // Test 9: Two-level nested wildcard
    {
        char path[] = "test_glob/deep/*/*.c";
        char **result = glob(path);
        int count = print_glob_results(result, "Test 9: test_glob/deep/*/*.c (2-level)");
        bool pass = (count == 1);  // level1/file1.c
        printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
        pass ? passed++ : failed++;
        free_glob_result(result);
    }

    // Test 10: Three-level nested wildcard
    {
        char path[] = "test_glob/deep/*/*/*.c";
        char **result = glob(path);
        int count = print_glob_results(result, "Test 10: test_glob/deep/*/*/*.c (3-level)");
        bool pass = (count == 1);  // level1/level2/deep_file.c
        printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
        pass ? passed++ : failed++;
        free_glob_result(result);
    }

    // Test 1: No wildcard - should return exact path
    // {
    //     char path[] = "test_glob/a.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 1: No wildcard (test_glob/a.txt)");
    //     bool pass = (result && result[0] && strcmp(result[0], path) == 0 && result[1] == NULL);
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 2: Basic * wildcard for .txt files
    // {
    //     char path[] = "test_glob/*.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 2: test_glob/*.txt");
    //     bool pass = (count == 3);  // a.txt, b.txt, abc.txt
    //     printf("  Expected: 3 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 3: Single ? wildcard
    // {
    //     char path[] = "test_glob/?.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 3: test_glob/?.txt (single char)");
    //     bool pass = (count == 2);  // a.txt, b.txt
    //     printf("  Expected: 2 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 4: Multiple ? wildcards
    // {
    //     char path[] = "test_glob/???.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 4: test_glob/???.txt (3 chars)");
    //     bool pass = (count == 1);  // abc.txt
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 5: * with prefix
    // {
    //     char path[] = "test_glob/a*.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 5: test_glob/a*.txt (prefix)");
    //     bool pass = (count == 2);  // a.txt, abc.txt
    //     printf("  Expected: 2 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 6: Different extension
    // {
    //     char path[] = "test_glob/*.dat";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 6: test_glob/*.dat");
    //     bool pass = (count == 1);  // a1b2c3.dat
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 7: Subdirectory pattern
    // {
    //     char path[] = "test_glob/single/*.c";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 7: test_glob/single/*.c");
    //     bool pass = (count == 1);  // only.c
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 8: Empty directory (should return 0 matches)
    // {
    //     char path[] = "test_glob/empty/*";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 8: test_glob/empty/* (empty dir)");
    //     bool pass = (count == 0);
    //     printf("  Expected: 0 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    

    // // Test 11: Single char extension with ?
    // {
    //     char path[] = "test_glob/multi_match/*.?";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 11: test_glob/multi_match/*.? (1-char ext)");
    //     bool pass = (count == 4);  // foo.c, bar.c, baz.c, foo.h
    //     printf("  Expected: 4 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 12: 3-char filename with ???
    // {
    //     char path[] = "test_glob/multi_match/???.c";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 12: test_glob/multi_match/???.c");
    //     bool pass = (count == 3);  // foo.c, bar.c, baz.c
    //     printf("  Expected: 3 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 13: Wildcard dir then files
    // {
    //     char path[] = "test_glob/*/*.c";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 13: test_glob/*/*.c (any subdir)");
    //     bool pass = (count >= 4);  // single/only.c, deep/file0.c, multi_match/foo,bar,baz.c
    //     printf("  Expected: >= 4 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 14: Pattern with dashes
    // {
    //     char path[] = "test_glob/special/*-*-*.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 14: test_glob/special/*-*-*.txt");
    //     bool pass = (count == 1);  // x-y-z.txt
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 15: Non-existent directory
    // {
    //     char path[] = "nonexistent/*.xyz";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 15: nonexistent/*.xyz");
    //     bool pass = (count == 0);
    //     printf("  Expected: 0 matches | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 16: ? in middle of name
    // {
    //     char path[] = "test_glob/a?c.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 16: test_glob/a?c.txt (? in middle)");
    //     bool pass = (count == 1);  // abc.txt
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    // // Test 17: Pattern with underscores
    // {
    //     char path[] = "test_glob/special/*_*_*.txt";
    //     char **result = glob(path);
    //     int count = print_glob_results(result, "Test 17: test_glob/special/*_*_*.txt");
    //     bool pass = (count == 1);  // a_b_c.txt
    //     printf("  Expected: 1 match | Result: %s\n", pass ? "PASS" : "FAIL");
    //     pass ? passed++ : failed++;
    //     free_glob_result(result);
    // }

    printf("\n========================================\n");
    printf("           SUMMARY                     \n");
    printf("========================================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("========================================\n");

    return failed > 0 ? 1 : 0;
}
