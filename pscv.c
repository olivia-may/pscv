/* 
 * pscv - Easily use pascal strings in C
 * Copyright (C) 2023  Olivia May
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>

char *insert_string(char *insertee_string, const int index, const char *inserter_string) {
    char *new_string = 
        (char *)malloc((strlen(insertee_string) + strlen(inserter_string) + 2)
        * sizeof(char));
    int i, j;

    if (!new_string) {
        fprintf(stderr, "Failed to allocate memory to insert %s into stream\n",
            inserter_string);
        exit(2);
    }

    for (i = 0; i < index; i++) {
        new_string[i] = insertee_string[i];
    }
    for (j = 0; j < strlen(inserter_string); j++) {
        new_string[i + j] = inserter_string[j];
    }
    for (i; i < strlen(insertee_string) + 1; i++) {
        new_string[i + j] = insertee_string[i];
    }

    free(insertee_string);
    return new_string;
}

// beginning of stream
void remove_chars(char *stream, const int amount) {
    int i;
    for (i = 0; i < strlen(stream); i++) {
        stream[i] = stream[i + amount];
    }
}

char *convert_to_octal_char_array(unsigned int number) {

    char *converted_int = NULL;
    int i;

    // + 1 for null char
    converted_int = (char *)malloc(4 * sizeof(char));

    i = 2;
    while (true) {
        if (i < 0) break;

        // + 48 to convert to unicode/ascii number
        converted_int[i] = (number % 8) + 48;

        // divide by 8
        number = number >> 3;
        
        i--;
    }
    converted_int[3] = '\0';

    return converted_int;
}

int main(int argc, char **argv) {
    

    if (argc < 2) {
        fputs("usage: pscv FILE\n", stdout);
        return 1;
    }

    if (access(argv[1], F_OK)) {

        fprintf(stderr, "pscv: error: file not found '%s'\n", argv[1]);
        return 1;
    }

    char *stream;
    FILE *file;
    int i, ch, char_count = 0;
    struct stat statinfo;
    bool is_newline_escaped;

    stat(argv[1], &statinfo);

    // + 1 for null char
    stream = (char *)malloc((statinfo.st_size + 1) * sizeof(char));
    if (!stream) {
        fprintf(stderr, "pscv: error: Failed to allocate memory to read %s\n",
            argv[1]);
        return 2;
    }

    file = fopen(argv[1], "r");
    
    i = 0;
    while (true) {
        ch = fgetc(file);
        if (ch < 0) break;
        stream[i] = ch;
        i++;
    }
    stream[i] = '\0';

#define COUNT_CHARS \
    while (true) { \
\
        if (stream[i + char_count] == '"') break; \
        if (!stream[i + char_count]) break; \
        char_count++; \
    }


    i = 0;
    while (stream[i]) {
       
        char_count = 0;

        // C++ style comments and macros
        if (!strncmp(&stream[i], "//", 2)) {
            
            is_newline_escaped = false;
            
            while (stream[i]) {

                if (is_newline_escaped) {
                    if (stream[i] == '\n') { i++; is_newline_escaped = false; }
                    else if (stream[i] != ' ') is_newline_escaped = false;
                    
                }
                else {
                    if (stream[i] == '\\') is_newline_escaped = true;
                    if (stream[i] == '\n') break;
                }

                i++;
            }
        }

        // C style comments
        if (!strncmp(&stream[i], "/*", 2)) {

            while (stream[i]) {

                if (!strncmp(&stream[i], "*/", 2)) {
                 
                    i += 2;
                    break;
                }
 
                i++;
            }
        }

        if (!strncmp(&stream[i], "\"\\p", 3)) {

            i += 3;
            
            COUNT_CHARS

            i--;
            
            // if there's no chars after \nnn, then dont do anything
            if (char_count) {
                remove_chars(&stream[i], 1);
                stream = insert_string(stream, i,
                    convert_to_octal_char_array(char_count));
                char_count = 0;
            }
        }
        if (!strncmp(&stream[i], "\"\\", 2)) {
            
            i += 2;
            
            // count octal digits already there, ex. \2 \22 or \222
            while (true) {
                
                if (char_count > 2) break;
                if (!stream[i + char_count]) break;
                
                // if not ascii 0 to 9
                if (stream[i + char_count] < 48 ||
                    stream[i + char_count] > 57) break;

                char_count++;
            }
            
            // if there's no chars after \nnn, \nn or \n, then dont do anything
            if (stream[i + char_count] != '"') {
            
                remove_chars(&stream[i], char_count);
                
                char_count = 0;

                COUNT_CHARS

                stream = insert_string(stream, i,
                    convert_to_octal_char_array(char_count));
                char_count = 0;
            }
        }

        i++;
    }

    fclose(file);

    file = fopen(argv[1], "w");
    fputs(stream, file);
    
    free(stream);
    fclose(file);

    return 0;
}
