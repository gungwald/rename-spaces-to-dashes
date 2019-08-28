#include <stdbool.h>    /* for bool, TRUE, FALSE */
#include <stdio.h>      /* for rename */
#include <stdlib.h>     /* for EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>   /* for stat, S_ISDIR, struct stat */
#include <unistd.h>
#include <dirent.h>     /* for DIR, opendir, readdir, closedir, struct dirent */
#include <limits.h>     /* for PATH_MAX */

enum FileType {REGULAR_FILE, DIRECTORY};

const int RENAME_FAILURE = -1;
const int RENAME_SUCCESS = 0;
const int STAT_FAILURE = -1;
const int STAT_SUCCESS = 0;

bool stringContains(char *toSearch, char toSearchFor);
char *replaceAll(char *stringToModify, char toReplace, char replacement);
void renameSpacesToDashes(char *file, enum FileType type);
bool descendDirectoryTree(char *file, void func(char *name, enum FileType type));
void renameFile(char *from, char *to);
bool isDirectory(char *name);

int main(int argc, char *argv[])
{
    int i;
    int exitStatus = EXIT_SUCCESS;
    char cwd[PATH_MAX];

    if (argc > 1)
        for (i = 1; i < argc; i++)
            recurse(argv[i], renameSpacesToDashes);
    else
        recurse(getcwd(cwd, PATH_MAX), renameSpacesToDashes);
    
    return exitStatus;
}

bool stringContains(char *toSearch, char toSearchFor)
{
    return strchr(toSearch, toSearchFor) != NULL;
}

char *replaceAll(char *s, char searchChar, char replacementChar)
{
    char *charFoundPointer;
    char *searchStart;
    char *result;

    result = searchStart = strdup(s);
    while ((charFoundPointer = strchr(searchStart, searchChar)) != NULL) {
        *charFoundPointer = replacementChar;
        searchStart = charFoundPointer++;
    }
    return result;
}

bool descendDirectoryTree(char *file, void func(char *name, enum FileType type))
{
    bool ok = false;
    DIR *d;
    struct dirent *entry;
    
    if (isDirectory(file)) 
    {
        if ((d = opendir(file)) != NULL) 
        {
            while ((entry = readdir(d)) != NULL) 
            {
                descendDirectoryTree(entry->d_name, func);
            }
            closedir(d);
        }
        else
        {
            perror(file);
        }
    }
    func(file);
    return ok;
}

void renameSpacesToDashes(char *fileName)
{
    char *nameWithoutSpaces;

    if (contains(fileName, ' ')) {
        nameWithoutSpaces = replaceAll(file, ' ', '-');
        renameFile(fileName, nameWithoutSpaces);
        free(nameWithoutSpaces);
    }
}

void renameFile(char *from, char *to)
{
    if (rename(from, to) == RENAME_FAILURE)
        perror(from);
}

bool isDirectory(char *name)
{
    struct stat fileInfo;
    bool isDirectory = false;

    if (stat(name, &fileInfo) == STAT_FAILURE)
        perror(name);
    else if (S_ISDIR(fileInfo.st_mode))
        isDirectory = true;

    return isDirectory;
}
