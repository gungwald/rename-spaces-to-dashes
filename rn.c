#include <ctype.h>      /* for tolower */
#include <dirent.h>     /* for DIR, opendir, readdir, closedir, struct dirent */
#include <limits.h>     /* for PATH_MAX */
#include <stdbool.h>    /* for bool, TRUE, FALSE */
#include <stdio.h>      /* for rename */
#include <stdlib.h>     /* for EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>     /* for strlen, strcat, strcpy */
#include <sys/types.h>
#include <sys/stat.h>   /* for stat, S_ISDIR, struct stat */
#include <unistd.h>

/* WIN32 is always defined for Windows regardless of word length. */
#ifdef WIN32
const char *FILE_SEPARATOR = "\\";
#else
const char *FILE_SEPARATOR = "/";
#endif

enum FileType {REGULAR_FILE, DIRECTORY};

const DIR *OPENDIR_FAILURE = NULL;
const DIR *READDIR_FAILURE = NULL;
const int RENAME_FAILURE = -1;
const int RENAME_SUCCESS = 0;
const int STAT_FAILURE = -1;
const int STAT_SUCCESS = 0;

bool stringContains(const char *toSearch, char toSearchFor);
char *replaceAll(const char *stringToModify, char toReplace, char replacement);
void renameSpacesToDashes(const char *file, enum FileType type);
bool descendDirectoryTree(const char *path, void func(const char *path));
void renameFile(const char *from, const char *to);
bool isDirectory(const char *name);
char *buildPath(const char *dirName, const char *fileName);

int main(int argc, char *argv[])
{
    int i;
    int exitStatus = EXIT_SUCCESS;
    char cwd[PATH_MAX];

    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            descendDirectoryTree(argv[i], renameSpacesToDashes);
        }
    } else {
        descendDirectoryTree(getcwd(cwd, PATH_MAX), renameSpacesToDashes);
    }
    return exitStatus;
}

bool stringContains(const char *toSearch, char toSearchFor)
{
    return strchr(toSearch, toSearchFor) != NULL;
}

/**
 * Creates a new string with all instances of searchChar replaced with 
 * replacementChar.
 *
 * @param s The string in which to search for searchChar
 * @param searchChar The character to search for
 * @param replacementChar The character to replace searchChar with
 * @return A new string with the characters replaced. It must be freed
 *         by the caller
 */
char *replaceAll(const char *s, char searchChar, char replacementChar)
{
    char *charFoundPointer;
    char *searchStart;
    char *result;

    result = searchStart = strdup(s);
    while ((charFoundPointer = strchr(searchStart, searchChar)) != NULL) {
        *charFoundPointer = replacementChar;
        searchStart = charFoundPointer + 1;
    }
    return result;
}

void descendDirectoryTree(const char *path, void func(const char *path))
{
    DIR *d;
    struct dirent *entry;
    char *dirEntryPath;
    
    if (isDirectory(path)) {
        if ((d = opendir(path)) != OPENDIR_FAILURE) {
            while ((entry = readdir(d)) != READDIR_FAILURE) {
                dirEntryPath = buildPath(path, entry->d_name);
                descendDirectoryTree(dirEntryPath, func);
                free(dirEntryPath);
            }
            closedir(d);
        }
        else {
            perror(path);
        }
    }
    func(path);
}

void renameSpacesToDashes(const char *path)
{
    char *pathWithoutSpaces;
    int answer = ' ';

    if (contains(path, ' ')) {
        pathWithoutSpaces = replaceAll(path, ' ', '-');
        while (answer != 'y' && answer != 'n') {
            printf("Rename '%s' to '%s'? (y/n) ", path, pathWithoutSpaces);
            answer = tolower(getchar());
        }
        if (answer == 'y') {
            renameFile(path, pathWithoutSpaces);
        }
        free(pathWithoutSpaces);
    }
}

void renameFile(const char *from, const char *to)
{
    if (rename(from, to) == RENAME_FAILURE)
        perror(from);
}

bool isDirectory(const char *path)
{
    struct stat fileProperties;
    bool isDirectory = false;

    if (stat(path, &fileProperties) == STAT_FAILURE)
        perror(path);
    else if (S_ISDIR(fileProperties.st_mode))
        isDirectory = true;

    return isDirectory;
}

char *buildPath(const char *dirName, const char *fileName)
{
    char *result;
    size_t dirNameLength = 0;
    size_t fileNameLength = 0;
    size_t resultLength;
    bool useFileSeparator;

    if (dirName != NULL)
        dirNameLength = strlen(dirName);

    if (fileName != NULL)
        fileNameLength = strlen(fileName);

    if (dirNameLength > 0 && fileNameLength > 0)
        useFileSeparator = true;
    else
        useFileSeparator = false;

    resultLength = dirNameLength + (useFileSeparator?strlen(FILE_SEPARATOR):0) + fileNameLength;
    result = (char *) malloc(resultLength * sizeof(char));

    strcat(result, "");
    if (dirName != NULL)
        strcat(result, dirName);
    if (useFileSeparator)
        strcat(result, FILE_SEPARATOR);
    if (fileName != NULL)
        strcat(result, fileName);
    return result;
}

