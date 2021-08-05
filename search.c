#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#define MAX_CHARS 40
#define EXACT "-exact"
#define PREFIX "-prefix"
#define ANYWHERE "-anywhere"
#define SORT "-sort"
#define DEFAULT_FILEPATH "/usr/share/dict/words"

bool isSortPresent = false;  //Global variable for easy detection of sort 
                            //argument

typedef enum {
    AT_EXACT = 11,
    AT_PREFIX = 12,
    AT_ANYWHERE = 13,
    AT_NONE = 15
} ArgumentTypes;

typedef enum {
    NONE = 0,
    USAGE_ERR = 1,
    FILE_ERR = 2,
    PATTERN_SEARCH_ERR = 3,
    NO_MATCH = 4
} ExitStatus;

typedef struct {
    ArgumentTypes searchTypes;
    char *argument;
    // bool sortArg;
} Arguments;

typedef struct {
    char *patternInput;
    char *filePath;
    Arguments args;
} Pattern;

/*
 * Search for input file name.
 *
 * return's true if file is found else return's false
*/
bool search_file(char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

/*
 * Count's and return's number of question marks in
 * a given pattern.
 *
*/
int get_number_of_wild_cards(char *patterInput) {
    int count = 0;
    for (int i = 0; i < strlen(patterInput); ++i) {
        if (patterInput[i] == '?') {
            count++;
        }
        if (!isalpha(patterInput[i]) && patterInput[i] != '?') {
            fprintf(stderr, "search: pattern should only contain question"
                    " marks and letters\n");
            fflush(stderr);
            exit(PATTERN_SEARCH_ERR);
        }
    }
    return count;
}

/*
 * Normalises the case of string for easy comparison
 *
 * return's a string in lower case.
*/
char *normalise_case(char *string) {
    if (strlen(string) <= 0) {
        return NULL;
    }
    char *caseChange = malloc(sizeof(char) * strlen(string));
    for (int i = 0; i < strlen(string); ++i) {
        if (string[i] != '?') {
            caseChange[i] = tolower(string[i]);
        } else {
            caseChange[i] = string[i];
        }
    }
    return caseChange;
}

/*
 * Scan's the file for words till End Of File
 *
 * return's a null terminated word or 0 if it encounter's EOF.
 */
char *get_words_from_file(FILE *file) {
    char *str = malloc(sizeof(char) * MAX_CHARS);
    char *result;
    if ((result = fgets(str, MAX_CHARS, file)) != NULL) {
        str[strlen(str) - 1] = '\0';
        return str;
    }
    return NULL;
}

/*
 * Check's if a word from the dictionary is valid or not.
 *
 * return's true if word is valid else false.
 */
bool is_valid_word(char *word) {
    //printf("%s\n", word);
    for (int i = 0; i < strlen(word); ++i) {
        if (!isalpha(word[i]) && word[i] != '?') {
            return false;
        }
    }
    return true;
} 

/*
 * Compare's two words
 *
 * return's nothing
*/
int compare_words(const void *w1, const void *w2) {
    char *word1 = *((char **) w1);
    char *word2 = *((char **) w2);
    return strcmp(word1, word2);
}

/*
 * Print's sorted words using qsort function
 *
 * return's nothing
*/
void print_sorted_words(int numOfWords, char **matchingWords) {
    qsort(matchingWords, (size_t) numOfWords, sizeof(char *), compare_words);
    for (int i = 0; i < numOfWords; ++i) {
        fprintf(stdout, "%s\n", matchingWords[i]);
        fflush(stdout);
    }
}

/*
 * Print's word in unsorted manner
 *
 * return's nothing.
*/
void print_words(int numOfWords, char **matchingWords) {
    for (int i = 0; i < numOfWords; ++i) {
        fprintf(stdout, "%s\n", matchingWords[i]);
        fflush(stdout);
    }
}

/*
 * Compares the string if "-exact" argument is given
 *
 * return's true if string matches else false
*/
bool is_argument_exact(char *argument) {
    if (strcmp(EXACT, argument) == 0) {
        return true;
    }
    return false;
}

/*
 * Compares the string if "-prefix" argument is given
 *
 * return's true if string matches else false
*/
bool is_argument_prefix(char *argument) {
    if (strcmp(PREFIX, argument) == 0) {
        return true;
    }
    return false;
}

/*
 * Compares the string if "-anywhere" argument is given
 *
 * return's true if string matches else false
*/
bool is_argument_anywhere(char *argument) {
    if (strcmp(ANYWHERE, argument) == 0) {
        return true;
    }
    return false;
}

/*
 * Compares the string if "-sort" argument is given
 *
 * return's true if string matches else false
*/
bool is_argument_sort(char *argument) {
    if (strcmp(SORT, argument) == 0) {
        return true;
    }
    return false;
}

/*
 * Check's if a pattern is exactly matching a word
 *
 * return's true if match found else return's false.
 */
bool is_exact_match(char *pattern, char *word) {
    int length = strlen(pattern);
    char build[length];
    for (int i = 0; i < strlen(pattern); ++i) {
        if (pattern[i] == '?') {
            build[i] = word[i];
        } else {
            build[i] = pattern[i];
        }
    }
    if (strcmp(build, word) == 0) {
        return true;
    } else {
        return false;
    }
}

/*
 * Run's the logic for to implement exact search
 *
 * return's nothing.
*/
void run_exact_search(char *toSearch, char *fromFileName, int wildCards) {
    FILE *file = fopen(fromFileName, "r");
    char *word, *searchString, *fileString;
    int numOfWords = 0, found = 0;
    char **matchingWords = (char **) malloc(sizeof(char *));
    while ((word = get_words_from_file(file)) != 0) {
        if (wildCards == 0) {
            searchString = normalise_case(toSearch);
            fileString = normalise_case(word);
            if (strcmp(searchString, fileString) == 0) {
                found = 1;
            }
        } else {
            searchString = normalise_case(toSearch);
            fileString = normalise_case(word);
            if (is_valid_word(fileString)) {
                if (strlen(searchString) == strlen(fileString)) {
                    if (is_exact_match(searchString, fileString)) {
                        found = 1;
                    }
                } 
            }
        }
        if (found == 1) {
            matchingWords = (char **) realloc(matchingWords, (numOfWords + 1) 
                    * sizeof(char *));
            matchingWords[numOfWords++] = word;
            found = 0;
        }
    }
    if (isSortPresent) {
        print_sorted_words(numOfWords, matchingWords);
    } else {
        print_words(numOfWords, matchingWords);
    }
}

/*
 * check's the string by using the strncmp function
 * 
 * return's true if prefix match
*/
bool is_prefix(char *pattern, char *word) {
    return strncmp(pattern, word, strlen(pattern)) == 0;
}

/*
 * check's if a given pattern with question marks is a prefix 
 * or not
 *
 * return's true if a word with same prefix is found else 
 * return's false
*/
bool is_prefix_match(char *pattern, char *word) {
    int length = strlen(word);
    char build[length], ch;
    for (int i = 0; i < strlen(word); ++i) {
        ch = word[i];
        if (pattern[i] == '?') {
            build[i] = ch; 
        } else {
            build[i] = pattern[i];
        }
    }
    if (is_prefix(build, word) && strlen(word) > strlen(pattern)) {
        return true;
    } else {
        return false;   
    }
} 

/*
 * Run's the logic for to implement "prefix" search
 *
 * return's nothing.
*/
void run_prefix_search(char *toSearch, char *fromFileName, int wildCards) {
    FILE *file = fopen(fromFileName, "r");
    char *searchString, *fileString, *word;
    int numOfWords = 0, found = 0;
    char **matchingWords = (char **) malloc(sizeof(char *));
    while ((word = get_words_from_file(file)) != 0) {
        if (wildCards == 0) {
            searchString = normalise_case(toSearch);
            fileString = normalise_case(word);
            if (is_valid_word(fileString)) {
                if (is_prefix(searchString, fileString)) {
                    found = 1;
                }  
            }
        } else {
            searchString = normalise_case(toSearch);
            fileString = normalise_case(word);
            if (is_valid_word(fileString)) {
                if (is_prefix_match(searchString, fileString)) {
                    found = 1;
                }
            }
        }
        if (found == 1) {
            matchingWords = (char **) realloc(matchingWords, (numOfWords + 1)
                    * sizeof(char *));
            matchingWords[numOfWords++] = word;
            found = 0;
        }
    }
    if (isSortPresent) {
        print_sorted_words(numOfWords, matchingWords);
    } else {
        print_words(numOfWords, matchingWords);
    }
}

/*
 * Check's if a pattern is in a word anywhere
 *
 * return's true if the pattern exists else
 * return's false
*/
bool is_substring(char *pattern, char *word) {
    char *dup = strdup(word);
    for (int i = 0; i < strlen(word); ++i) {
        dup[i] = tolower(word[i]);
    }
    if (strstr(dup, pattern) != NULL) {
        return true;
    }
    return false;
}

/*
 * check's if a given pattern with question marks is anywhere 
 * in a given word or not
 *
 * return's true if a word with same prefix is found else 
 * return's false
*/
bool is_substring_match(char *pattern, char *word) {
    int length = strlen(word);
    char *dup = strdup(word);
    for (int i = 0; i < strlen(word); ++i) {
        dup[i] = tolower(word[i]);
    }
    char build[length];
    for (int i = 0; i < strlen(dup); ++i) {
        if (pattern[i] == dup[i]) {
            build[i] = dup[i];
        } 
        if (pattern[i] == '?') {
            build[i] = dup[i];
        }
    }
    
   // printf("%s %s\n", dup, build);
    /*if (strstr(dup, build) != NULL && strlen(build) >= strlen(pattern)) {
        return true;
    } else {
        return false;
    }*/
    return false;
}

/*
 * Run's the logic for to implement "anywhere" search
 *
 * return's nothing.
*/
void run_anywhere_search(char *toSearch, char *fromFileName, int wildCards) {
    FILE *file = fopen(fromFileName, "r");
    char *word;
    int numOfWords = 0, found = 0;
    char **matchingWords = (char **) malloc(sizeof(char)); 
    while ((word = get_words_from_file(file)) != 0) {
        if (wildCards == 0) {
            //fileString = normalise_case(word);
            if (is_valid_word(word)) {
                if (is_substring(toSearch, word)) {
                    found = 1;
                }
            }
        } else {
            //searchString = normalise_case(toSearch);
            //fileString = normalise_case(word);
            if (is_valid_word(word)) {
                if (is_substring_match(toSearch, word)) {
                    found = 1;
                }
            }
        }
        if (found == 1) {
            matchingWords = (char **) realloc(matchingWords, (numOfWords + 1)
                    * sizeof(char *));
            matchingWords[numOfWords++] = word;
            found = 0;
        }
    }
    if (isSortPresent) {
        print_sorted_words(numOfWords, matchingWords);
    } else {
        print_words(numOfWords, matchingWords);
    }
} 

/*
 * Set's the variables necessary to performing matching and starts
 * pattern matching logic using switch cases 
 *
 * return's nothing
*/
void search_pattern_match(Pattern *pattern) {
    Pattern localSearch;
    memset(&localSearch, 0, sizeof(Pattern));
    localSearch.args = pattern->args;
    localSearch.filePath = pattern->filePath;
    localSearch.patternInput = pattern->patternInput;
    int wildCards = get_number_of_wild_cards(localSearch.patternInput);
    if (localSearch.filePath == NULL) {
        localSearch.filePath = DEFAULT_FILEPATH;
    }
    if (!search_file(localSearch.filePath)) {
        fprintf(stderr, "search: file \"%s\" can not be opened\n",
                localSearch.filePath);
        exit(FILE_ERR);
    }
    
    switch (localSearch.args.searchTypes) {
        case AT_EXACT:
            run_exact_search(localSearch.patternInput, localSearch.filePath,
                wildCards);
            break;
        case AT_PREFIX:
            run_prefix_search(localSearch.patternInput, localSearch.filePath,
                wildCards);
            break;
        case AT_ANYWHERE:
            run_anywhere_search(localSearch.patternInput, 
                localSearch.filePath, wildCards);
            break;
        case AT_NONE:
            run_exact_search(localSearch.patternInput, localSearch.filePath,
                wildCards);
            break;
    }
}

/*
 * Function to compare the search arguments input by the user.
 *
 * return's Arguments after performing comparison on each argument.
*/
Arguments get_search_arguments(char *argument) {
    Arguments args;
    args.searchTypes = AT_NONE;
    if (is_argument_exact(argument)) {
        args.searchTypes = AT_EXACT;
        args.argument = EXACT;
        return args;
    } else if (is_argument_prefix(argument)) {
        args.searchTypes = AT_PREFIX;
        args.argument = PREFIX;
        return args;
    } else if (is_argument_anywhere(argument)) {
        args.searchTypes = AT_ANYWHERE;
        args.argument = ANYWHERE;
        return args;
    }
    if (is_argument_sort(argument)) {
        isSortPresent = true;        
    }
    return args;
}

/*
 * check's if the second argument is present.
 *
 * return's true if it is a "-sort" argument else return's false
*/
bool check_second_argument(char **argument, Arguments *args) {
    Arguments arg;
    memset(&arg, 0, sizeof(Arguments));
    if (is_argument_sort(argument[0]) && !isSortPresent) {
        isSortPresent = true;
        return true;
    } else {
        if (args->searchTypes == AT_NONE) {
            arg = get_search_arguments(argument[0]);
            args->searchTypes = arg.searchTypes;
            args->argument = arg.argument;
            return false;
        }
    }
    return false;
}

/*
 * Check's the argument for errors or to get the type of
 * search argument input by user
 *
 * return's the arguments stored with necessary information.
*/
Arguments check_argument(char **argument) {
    Arguments arguments;
    char beginc = *argument[0];
    memset(&arguments, 0, sizeof(Arguments));
    switch (beginc) {
        case '/':
            fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                "[-sort] pattern [filename]\n");
            exit(USAGE_ERR);
        case '-':
            arguments = get_search_arguments(argument[0]);
            return arguments;
        default:
            arguments.searchTypes = AT_NONE;
            return arguments;
    }
    return arguments;
}

int main(int argc, char *argv[]) {
    int numberOfArgs = argc;
    Pattern input;
    memset(&input, 0, sizeof(Pattern));
    char firstc;
    switch (numberOfArgs) {
        case 1:
            fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                    " [-sort] pattern [filename]\n");
            exit(USAGE_ERR);
            break;
        case 2:
            if (strlen(argv[1]) <= 0) {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            firstc = *argv[1];
            if (firstc == '-' || firstc == '/') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            input.args.searchTypes = AT_EXACT;
            input.args.argument = EXACT;
            input.filePath = DEFAULT_FILEPATH;
            char *pattern = argv[1];
            input.patternInput = malloc(sizeof(char) * strlen(pattern));
            strcpy(input.patternInput, pattern);
            search_pattern_match(&input);
            break;
        case 3:
            if (strlen(argv[1]) <= 0 || strlen(argv[2]) <= 0) {
                fprintf(stderr, "Usage: search[-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            input.args = check_argument(&argv[1]);
            firstc = *argv[2];
            if (firstc == '/' || firstc == '-') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            if (isSortPresent == true || input.args.searchTypes != AT_NONE) {
                pattern = argv[2];
                input.patternInput = malloc(sizeof(char) * strlen(pattern));
                strcpy(input.patternInput, pattern);
                search_pattern_match(&input);
            } else if (input.args.searchTypes == AT_NONE) {
                pattern = argv[1];
                input.patternInput = malloc(sizeof(char) * strlen(pattern));
                strcpy(input.patternInput, pattern);
                input.filePath = argv[2];
                search_pattern_match(&input);
            }
            break;
        case 4:
            if (strlen(argv[1]) <= 0 || strlen(argv[2]) <= 0 || 
                    strlen(argv[3]) <= 0) {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            if (*argv[1] != '-') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]");
                exit(USAGE_ERR);
            }
            input.args = check_argument(&argv[1]);
            if (*argv[2] == '/') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            if (!check_second_argument(&argv[2], &input.args)) {
                pattern = argv[2];
                input.patternInput = malloc(sizeof(char) * strlen(pattern));
                strcpy(input.patternInput, pattern);
                input.filePath = argv[3];
                search_pattern_match(&input);
            } else {
                pattern = argv[3];
                input.patternInput = malloc(sizeof(char) * strlen(pattern));
                strcpy(input.patternInput, pattern);
                search_pattern_match(&input);
            }
            break;
        case 5:
            if (strlen(argv[1]) <= 0 || strlen(argv[2]) <= 0 || 
                    strlen(argv[3]) <= 0 || strlen(argv[4]) <= 0) {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            if (*argv[1] != '-') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            input.args = check_argument(&argv[1]);
            if (*argv[2] != '-') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            if (!check_second_argument(&argv[2], &input.args)) {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            pattern = argv[3];
            input.patternInput = malloc(sizeof(char) * strlen(pattern));
            strcpy(input.patternInput, pattern);
            if (*argv[4] == '-') {
                fprintf(stderr, "Usage: search [-exact|-prefix|-anywhere]"
                        " [-sort] pattern [filename]\n");
                exit(USAGE_ERR);
            }
            input.filePath = argv[4];
            search_pattern_match(&input);
            break;
    }
    free(input.patternInput);
}
