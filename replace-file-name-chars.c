#define _GNU_SOURCE     /* to activate asprintf in stdio.h */
#include <ctype.h>      /* for tolower */
#include <dirent.h>     /* for DIR, opendir, readdir, closedir, struct dirent */
#include <errno.h>      /* for errno, ERANGE */
#include <getopt.h>     /* for struct option, required_argument, getopt_long, optind, optarg */
#include <libgen.h>     /* for basename, dirname */
#include <stdbool.h>    /* for bool, TRUE, FALSE */
#include <stdio.h>      /* for rename */
#include <stdlib.h>     /* for getenv, EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>     /* for strlen, strcat, strcpy, strchr, strdup, strerror */
#include <sys/types.h>  /* ??? */
#include <sys/stat.h>   /* for stat, S_ISDIR, struct stat */
#include <unistd.h>     /* for getcwd */

/* WIN32 is always defined for Windows regardless of word length. */
#ifdef WIN32
const char *FILE_SEPARATOR = "\\";
#else
const char *FILE_SEPARATOR = "/";
#endif

#define TRACE_ENTER(f,n,v)          if(g_debug)printf("Enter %s with %s=%s\n",f,n,v)
#define TRACE_ENTER2(f,n,v,n2,v2)   if(g_debug)printf("Enter %s with %s=%s %s=%s\n",f,n,v,n2,v2)
#define TRACE_RETURN(f,v)           if(g_debug)printf("Return from %s with %s\n",f,v)
#define TRACE_RETURN_BOOL(f,v)      if(g_debug)printf("Return from %s with %s\n",f,btoa(v))

#define MAX_LINE_LENGTH 1024

enum FileType {REGULAR_FILE, DIRECTORY};
enum UserResponse {YES_RESPONSE, NO_RESPONSE, QUIT_RESPONSE, ALL_RESPONSE};

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
const int ASPRINTF_FAILURE = -1;

char *btoa(bool b);
bool stringContains(const char *searchIn, char searchFor);
char *replaceAll(const char *stringToModify, char searchFor, char replaceWith);
void replaceInFileName(const char *path);
void descendDirectoryTree(const char *path, void func(const char *path));
void renameFile(const char *from, const char *to);
bool isDirectory(const char *name);
char *buildPath(const char *dirName, const char *fileName);
void chomp(char *line);
char *allocateCharArray(size_t size);
char *getCurrentDirectory();
char *toLowerCase(char *s);
enum UserResponse askUser(const char *question);
bool fileExists(const char *path);
void die(const char *message);
void dieWithSystemError(const char *message, int systemErrorCode);
char *findAvailableName(const char *dirname, const char *basename);
char *getBasename(const char *path);
char *getDirname(const char *path);
void usage();

bool g_debug = false;
char *g_programName;
char g_replaceWith = '-';
char g_searchFor = ' ';
bool g_autoApprove = false;
bool g_searchOnly = false;

int main(int argc, char *argv[])
{
    int i;
    int longOptionIndex = 0;
    int exitStatus = EXIT_SUCCESS;
    int opt;

    g_programName = getBasename(argv[0]);

    struct option acceptedLongOptions[] = {
        {"replace-with", required_argument, NULL, 'r'},
        {"search-for", required_argument, NULL, 's'},
        {"auto-approve", no_argument, NULL, 'y'},
        {"search-only", no_argument, NULL, 'o'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    if (getenv("DEBUG") != NULL) {
        g_debug = true;
    }

    while ((opt = getopt_long(argc, argv, "s:r:yoh", acceptedLongOptions, &longOptionIndex)) != GETOPT_FINISHED) {
        switch (opt) {
        case 'r':
            if (strlen(optarg) > 0) {
                g_replaceWith = optarg[0];
            }
            break;
        case 's':
            if (strlen(optarg) > 0) {
                g_searchFor = optarg[0];
            }
            break;
        case 'y':
            g_autoApprove = true;
            break;
        case 'o':
            g_searchOnly = true;
            break;
        case 'h':
        	usage();
        	break;
        default:
            fprintf(stderr, "%s: unrecognized option: %c\n", g_programName, opt);
            usage();
            break;
        }
    }
    /* optind is now the index of the first non-option argument. */

    if (argc > optind) {
        for (i = optind; i < argc; i++) {
            descendDirectoryTree(argv[i], replaceInFileName);
        }
    }
    else {
    	usage();
    }
    free(g_programName);
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
char *replaceAll(const char *s, char searchFor, char replaceWith)
{
    char *charFoundPointer;
    char *searchStart;
    char *result;

    result = searchStart = strdup(s);
    while ((charFoundPointer = strchr(searchStart, searchFor)) != NULL) {
        *charFoundPointer = replaceWith;
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

void replaceInFileName(const char *path)
{
    char *pathWithoutSpaces;
    char *pathBasenameWithoutSpaces;
    char *pathDirname;
    char *pathBasename;
    char *question;
    enum UserResponse answer;

    pathBasename = getBasename(path);
    if (stringContains(pathBasename, g_searchFor)) {
        if (g_searchOnly) {
            puts(path);
        }
        else {
            pathDirname = getDirname(path);
            pathBasenameWithoutSpaces = replaceAll(pathBasename, g_searchFor, g_replaceWith);
            pathWithoutSpaces = findAvailableName(pathDirname, pathBasenameWithoutSpaces);
            if (g_autoApprove) {
                answer = YES_RESPONSE;
            }
            else {
                /* TODO - Should be: In dirname, rename %s to %s? */
                if (asprintf(&question, "Rename '%s' to '%s'?", path, pathWithoutSpaces) == ASPRINTF_FAILURE) {
                    die("Failed to allocate memory");
                }
                answer = askUser(question);
            }
            switch (answer) {
            case ALL_RESPONSE:
                g_autoApprove = true;
            /* no break */
            case YES_RESPONSE:
                renameFile(path, pathWithoutSpaces);
                break;
            case NO_RESPONSE:
                break;
            case QUIT_RESPONSE:
                exit(EXIT_SUCCESS);
                break;
            }
            free(pathWithoutSpaces);
            free(pathBasenameWithoutSpaces);
            free(pathDirname);
        }
    }
    free(pathBasename);
}

void renameFile(const char *from, const char *to)
{
    if (rename(from, to) == RENAME_FAILURE) {
        perror(from);
    }
}

bool isDirectory(const char *path)
{
    struct stat fileProperties;
    bool isDirectory = false;
    char *fn = "isDirectory";

    if (stat(path, &fileProperties) == STAT_FAILURE) {
        perror(path);
    }
    else if (S_ISDIR(fileProperties.st_mode)) {
        isDirectory = true;
    }

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
    if (dirName != NULL) {
        dirNameLength = strlen(dirName);
    }

    if (fileName != NULL) {
        fileNameLength = strlen(fileName);
    }

    if (dirNameLength > 0 && fileNameLength > 0) {
        useFileSeparator = true;
    }
    else {
        useFileSeparator = false;
    }

    resultLength = dirNameLength + (useFileSeparator?strlen(FILE_SEPARATOR):0) + fileNameLength;
    result = allocateCharArray(resultLength);

    strcpy(result, "");
    if (dirName != NULL) {
        strcat(result, dirName);
    }
    if (useFileSeparator) {
        strcat(result, FILE_SEPARATOR);
    }
    if (fileName != NULL) {
        strcat(result, fileName);
    }

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
            }
            else {
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
        dieWithSystemError("malloc", errno);
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
            }
            else {
                dieWithSystemError("getcwd", errno);
            }
        }
        else {
            stillTrying = false;
        }
    }
    return buffer;
}

/**
 * This version modifies its argument.
 */
char *toLowerCase(char *s)
{
    size_t len;
    size_t i;

    len = strlen(s);
    for (i = 0; i < len; i++) {
        s[i] = tolower(s[i]);
    }
    return s;
}

enum UserResponse askUser(const char *question)
{
    enum UserResponse response;
    char answer[MAX_LINE_LENGTH] = "";

    while (strcmp(answer,"y")!=0 && strcmp(answer,"n")!=0 && strcmp(answer,"q")!=0 && strcmp(answer,"a")!=0) {
        /* It is a security issue to printf(question) directly. */
        printf("%s", question);
        printf(" (y/n/q/a) ");
        if (fgets(answer, MAX_LINE_LENGTH, stdin) == FGETS_FAILURE_OR_EOF) {
            if (feof(stdin)) {
                exit(EXIT_SUCCESS);
            }
            else if (ferror(stdin)) {
                dieWithSystemError("stdin", errno);
            }
            else {
                die("Unknown standard input error");
            }
        }
        chomp(answer);
        toLowerCase(answer);
    }
    if (strcmp(answer,"y") == 0) {
        response = YES_RESPONSE;
    }
    else if (strcmp(answer,"n") == 0) {
        response = NO_RESPONSE;
    }
    else if (strcmp(answer,"a") == 0) {
        response = ALL_RESPONSE;
    }
    else if (strcmp(answer,"q") == 0) {
        response = QUIT_RESPONSE;
    }
    else {
        response = NO_RESPONSE;
    }
    return response;
}

bool fileExists(const char *path)
{
    struct stat fileProperties;
    bool fileExists;
    char *fn = "fileExists";

    if (stat(path, &fileProperties) == STAT_FAILURE)
        if (errno == ENOENT) {
            fileExists = false;
        }
        else {
            dieWithSystemError(path, errno);
        }
    else {
        fileExists = true;
    }

    TRACE_RETURN_BOOL(fn, fileExists);
    return fileExists;
}

void die(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

void dieWithSystemError(const char *message, int systemErrorCode)
{
    fprintf(stderr, "%s: %s\n", message, strerror(systemErrorCode));
    exit(EXIT_FAILURE);
}

char *findAvailableName(const char *dirname, const char *basename)
{
    char *path;
    int n = 2;

    path = buildPath(dirname, basename);
    while (fileExists(path)) {
        free(path);
        if (n > 1024) {
            die("Unable to find a free file name");
        }
        if (asprintf(&path, "%s/%s-%d", dirname, basename, n++) == ASPRINTF_FAILURE) {
            die("Failed to allocate memory for asprintf");
        }
    }
    return path;
}

char *getBasename(const char *path)
{
    char *pathCopy;
    char *basenameCopy;

    /* basename modifies path so a copy must be created. */
    pathCopy = strdup(path);

    /* When pathCopy is freed, we lose the memory that the
       result of basename is stored in. So we need to use
       strdup to create our own copy of the result. */
    basenameCopy = strdup(basename(pathCopy));
    free(pathCopy);
    return basenameCopy;
}

char *getDirname(const char *path)
{
    char *pathCopy;
    char *dirnameCopy;

    /* dirname modifies path so a copy must be created. */
    pathCopy = strdup(path);

    /* When pathCopy is freed, we lose the memory that the
       result of dirname is stored in. So we need to use
       strdup to create our own copy of the result. */
    dirnameCopy = strdup(dirname(pathCopy));
    free(pathCopy);
    return dirnameCopy;
}

void usage()
{
	puts("Replaces characters in file names");
	printf("Usage: %s [options] file ...\n", g_programName);
	puts("  If file is a directory the program will recursively rename all files in the");
	puts("  directory and its subdirectories.");
	puts("  -s, --search-for=CHAR    Specify the character to search for; default=space");
	puts("  -r, --replace-with=CHAR  Specify the replacement character;   default=dash");
	puts("  -y, --auto-approve       Don't ask to rename each file");
	puts("  -o, --search-only        Don't rename, just search");
}
