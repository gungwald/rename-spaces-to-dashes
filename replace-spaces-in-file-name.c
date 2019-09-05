#include <ctype.h>      /* for tolower */
#include <dirent.h>     /* for DIR, opendir, readdir, closedir, struct dirent */
#include <errno.h>      /* for errno, ERANGE */
#include <getopt.h>	/* for struct option, required_arguent, getopt_long, */
#include <libgen.h>	/* for basename, dirname */
#include <stdbool.h>    /* for bool, TRUE, FALSE */
#include <stdio.h>      /* for rename */
#include <stdlib.h>     /* for getenv, EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>     /* for strlen, strcat, strcpy, strchr, strdup */
#include <sys/types.h>
#include <sys/stat.h>   /* for stat, S_ISDIR, struct stat */
#include <unistd.h>     /* for getcwd */

/* WIN32 is always defined for Windows regardless of word length. */
#ifdef WIN32
const char *FILE_SEPARATOR = "\\";
#else
const char *FILE_SEPARATOR = "/";
#endif

#define TRACE_ENTER(f,n,v)          if(debug)printf("Enter %s with %s=%s\n",f,n,v)
#define TRACE_ENTER2(f,n,v,n2,v2)   if(debug)printf("Enter %s with %s=%s %s=%s\n",f,n,v,n2,v2)
#define TRACE_RETURN(f,v)           if(debug)printf("Return from %s wth %s\n",f,v)
#define TRACE_RETURN_BOOL(f,v)      if(debug)printf("Return from %s wth %s\n",f,btoa(v))

#define MAX_LINE_LENGTH 1024

enum FileType {REGULAR_FILE, DIRECTORY};

const DIR *OPENDIR_FAILURE = NULL;
const struct dirent *READDIR_FAILURE = NULL;
const int RENAME_FAILURE = -1;
const int RENAME_SUCCESS = 0;
const int STAT_FAILURE = -1;
const int STAT_SUCCESS = 0;
const int GETOPT_FINISHED = -1;
const void *MALLOC_FAILURE = NULL;
const char *GETCWD_FAILURE = NULL;
const char *FGETS_FAILURE_OR_EOF = NULL;

char *btoa(bool b);
bool stringContains(const char *searchIn, char searchFor);
char *replaceAll(const char *stringToModify, char toReplace, char replacement);
void renameSpacesToDashes(const char *path);
void descendDirectoryTree(const char *path, void func(const char *path));
void renameFile(const char *from, const char *to);
bool isDirectory(const char *name);
char *buildPath(const char *dirName, const char *fileName);
void chomp(char *line);
char *allocateCharArray(size_t size);
char *getCurrentDirectory();

bool debug = false;
char replacementChar = '-';

/* TODO: Use getopt */
/* TODO: Make replacement character a command line option, defaults to dash */
/* TODO: Make "character to replace" a command line option, defaults to space */
/* TODO: Make program name plural */
/* TODO: Add no-ask option */
/* TODO: Ask for any options not given, showing defaults */
/* TODO: If a rename clashes, modify the name, like append "--2" */
/* TODO: Add option to simply find the problematic names, no rename */

/* Later, maybe a different program */
/* TODO: Identify all problematic characters in file names, not just spaces */
/* TODO: Think of a better name, like replace-problem-chars-in-file-names */
int main(int argc, char *argv[])
{
    int i;
    int longOptionIndex = 0;
    int exitStatus = EXIT_SUCCESS;
    char *cwd;
    int opt;

    struct option acceptedLongOptions[] = {
        {"replacement", required_argument, 0, 0},
        {0,		0,		   0, 0}
    };

    if (getenv("DEBUG") != NULL) {
    	debug = true;
    }

    while ((opt = getopt_long(argc, argv, "r:", acceptedLongOptions, &longOptionIndex)) != GETOPT_FINISHED) {
    	switch (opt) {
    	case 'r':
            if (strlen(optarg) > 0) {
                replacementChar = optarg[0];
            }
            break;
    	default:
            fprintf(stderr, "unrecognized option %c\n", opt);
    	}
    }

    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            descendDirectoryTree(argv[i], renameSpacesToDashes);
        }
    } else {
        cwd = getCurrentDirectory();
        descendDirectoryTree(cwd, renameSpacesToDashes);
        free(cwd);
    }
    return exitStatus;
}

bool stringContains(const char *searchIn, char searchFor)
{
    return strchr(searchIn, searchFor) != NULL;
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
    char *fn = "descendDirectoryTree";
    
    TRACE_ENTER(fn, "path", path);
    if (isDirectory(path)) {
        if ((d = opendir(path)) != OPENDIR_FAILURE) {
            while ((entry = readdir(d)) != READDIR_FAILURE) {
            	if (entry->d_name[0] != '.') { /* Not hidden, cwd, or parent */
                    dirEntryPath = buildPath(path, entry->d_name);
                    descendDirectoryTree(dirEntryPath, func);
                    free(dirEntryPath);
            	}
            }
            closedir(d);
        }
        else {
            perror(path);
        }
    }
    func(path);
    TRACE_RETURN(fn, "void");
}

/* TODO: Add a quit option */
void renameSpacesToDashes(const char *path)
{
    char *pathWithoutSpaces;
    char *pathBasenameWithoutSpaces;
    char answer[MAX_LINE_LENGTH] = "";
    char *pathDirname;
    char *pathBasename;

    pathBasename = basename(strdup(path));
    if (stringContains(pathBasename, ' ')) {
    	pathDirname = dirname(strdup(path));
        pathBasenameWithoutSpaces = replaceAll(pathBasename, ' ', replacementChar);
        while (strcmp(answer,"y") != 0 && strcmp(answer,"n") != 0) {
            printf("Rename '%s' to '%s'? (y/n) ", path, pathBasenameWithoutSpaces);
            if (fgets(answer, MAX_LINE_LENGTH, stdin) == FGETS_FAILURE_OR_EOF) {
                break;
            }
            chomp(answer);
            tolower(answer[0]);
        }
        if (answer[0] == 'y') {
            pathWithoutSpaces = buildPath(pathDirname, pathBasenameWithoutSpaces);
            renameFile(path, pathWithoutSpaces);
            free(pathWithoutSpaces);
        }
        free(pathBasenameWithoutSpaces);
        free(pathDirname);
    }
    free(pathBasename);
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
    char *fn = "isDirectory";

    if (stat(path, &fileProperties) == STAT_FAILURE)
        perror(path);
    else if (S_ISDIR(fileProperties.st_mode))
        isDirectory = true;

    TRACE_RETURN_BOOL(fn, isDirectory);
    return isDirectory;
}

char *buildPath(const char *dirName, const char *fileName)
{
    char *result;
    size_t dirNameLength = 0;
    size_t fileNameLength = 0;
    size_t resultLength;
    bool useFileSeparator;
    char *fn = "buildPath";

    TRACE_ENTER2(fn, "dirName", dirName, "fileName", fileName);
    if (dirName != NULL)
        dirNameLength = strlen(dirName);

    if (fileName != NULL)
        fileNameLength = strlen(fileName);

    if (dirNameLength > 0 && fileNameLength > 0)
        useFileSeparator = true;
    else
        useFileSeparator = false;

    resultLength = dirNameLength + (useFileSeparator?strlen(FILE_SEPARATOR):0) + fileNameLength;
    result = allocateCharArray(resultLength);

    strcpy(result, "");
    if (dirName != NULL)
        strcat(result, dirName);
    if (useFileSeparator)
        strcat(result, FILE_SEPARATOR);
    if (fileName != NULL)
        strcat(result, fileName);

    TRACE_RETURN(fn, result);
    return result;
}

void chomp(char *line)
{
    /* Because these are unsigned, if they're 0 and we subtract 1, we don't
       get -1 but a very large number. So they can't be allowed to go
       negative. The code guards against negative values in these variables. */
    size_t lastIndex;
    size_t lineLength;

    lineLength = strlen(line);
    if (lineLength > 0) {
        lastIndex = lineLength - 1;
        while (line[lastIndex] == '\r' || line[lastIndex] == '\n') {
            line[lastIndex] = '\0';
            if (lastIndex == 0) {
                break;
            } else {
                lastIndex--;
            }
        }
    }
}

/**
 * Converts a bool to a string (ascii).
 */
char *btoa(bool b)
{
    return b ? "true" : "false";
}

char *allocateCharArray(size_t size)
{
    char *array;

    if ((array = (char *) malloc(size * sizeof(char))) == MALLOC_FAILURE) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return array;
}

char *getCurrentDirectory()
{
    size_t bufferSize = 256;
    char *buffer;
    bool stillTrying = true;
    int getcwdErrorCode;

    while (stillTrying) {
        buffer = allocateCharArray(bufferSize);
        if (getcwd(buffer, bufferSize) == GETCWD_FAILURE) {
            getcwdErrorCode = errno; /* Save it for later */
            free(buffer);
            if (getcwdErrorCode == ERANGE) {
                /* A bigger buffer is needed. */
                bufferSize *= 2; /* Double the size */
                buffer = allocateCharArray(bufferSize);
            } else {
                perror("getcwd");
                exit(EXIT_FAILURE);
            }
        } else {
            stillTrying = false;
        }
    }
    return buffer;
}
