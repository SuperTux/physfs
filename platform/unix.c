/*
 * Unix support routines for PhysicsFS.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if (defined __STRICT_ANSI__)
#define __PHYSFS_DOING_STRICT_ANSI__
#endif

/*
 * We cheat a little: I want the symlink version of stat() (lstat), and
 *  GCC/Linux will not declare it if compiled with the -ansi flag.
 *  If you are really lacking symlink support on your platform,
 *  you should #define __PHYSFS_NO_SYMLINKS__ before compiling this
 *  file. That will open a security hole, though, if you really DO have
 *  symlinks on your platform; it renders PHYSFS_permitSymbolicLinks(0)
 *  useless, since every symlink will be reported as a regular file/dir.
 */
#if (defined __PHYSFS_DOING_STRICT_ANSI__)
#undef __STRICT_ANSI__
#endif
#include <stdio.h>
#if (defined __PHYSFS_DOING_STRICT_ANSI__)
#define __STRICT_ANSI__
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>


#define __PHYSICSFS_INTERNAL__
#include "physfs_internal.h"


const char *__PHYSFS_platformDirSeparator = "/";

char **__PHYSFS_platformDetectAvailableCDs(void)
{
    /* !!! write me. */
    char **retval = malloc(sizeof (char *));
    if (retval != NULL)
        *retval = NULL;

    return(retval);
} /* __PHYSFS_detectAvailableCDs */


static char *copyEnvironmentVariable(const char *varname)
{
    const char *envr = getenv(varname);
    char *retval = NULL;

    if (envr != NULL)
    {
        retval = malloc(strlen(envr) + 1);
        if (retval != NULL)
            strcpy(retval, envr);
    } /* if */

    return(retval);
} /* copyEnvironmentVariable */


/* !!! this is ugly. */
char *__PHYSFS_platformCalcBaseDir(const char *argv0)
{
    /* If there isn't a path on argv0, then look through the $PATH for it. */

    char *retval = NULL;
    char *envr;
    char *start;
    char *ptr;
    char *exe;

    if (strchr(argv0, '/') != NULL)   /* default behaviour can handle this. */
        return(NULL);

    envr = copyEnvironmentVariable("PATH");
    BAIL_IF_MACRO(!envr, NULL, NULL);

    start = envr;
    do
    {
        ptr = strchr(start, ':');
        if (ptr)
            *ptr = '\0';

        exe = (char *) malloc(strlen(start) + strlen(argv0) + 2);
        if (!exe)
        {
            free(envr);
            BAIL_IF_MACRO(1, ERR_OUT_OF_MEMORY, NULL);
        } /* if */
        strcpy(exe, start);
        if (start[strlen(start) - 1] != '/')
            strcat(start, "/");
        strcat(start, argv0);
        if (access(exe, X_OK) != 0)
            free(exe);
        else
        {
            retval = exe;
            strcpy(retval, start);  /* i'm lazy. piss off. */
            break;
        } /* else */

        start = ptr + 1;
    } while (ptr != NULL);

    free(envr);
    return(retval);
} /* __PHYSFS_platformCalcBaseDir */


static char *getUserNameByUID(void)
{
    uid_t uid = getuid();
    struct passwd *pw;
    char *retval = NULL;

    pw = getpwuid(uid);
    if ((pw != NULL) && (pw->pw_name != NULL))
    {
        retval = malloc(strlen(pw->pw_name) + 1);
        if (retval != NULL)
            strcpy(retval, pw->pw_name);
    } /* if */
    
    return(retval);
} /* getUserNameByUID */


static char *getUserDirByUID(void)
{
    uid_t uid = getuid();
    struct passwd *pw;
    char *retval = NULL;

    pw = getpwuid(uid);
    if ((pw != NULL) && (pw->pw_dir != NULL))
    {
        retval = malloc(strlen(pw->pw_dir) + 1);
        if (retval != NULL)
            strcpy(retval, pw->pw_dir);
    } /* if */
    
    return(retval);
} /* getUserDirByUID */


char *__PHYSFS_platformGetUserName(void)
{
    char *retval = getUserNameByUID();
    if (retval == NULL)
        retval = copyEnvironmentVariable("USER");
    return(retval);
} /* __PHYSFS_platformGetUserName */


char *__PHYSFS_platformGetUserDir(void)
{
    char *retval = copyEnvironmentVariable("HOME");
    if (retval == NULL)
        retval = getUserDirByUID();
    return(retval);
} /* __PHYSFS_platformGetUserDir */


int __PHYSFS_platformGetThreadID(void)
{
    return((int) pthread_self());
} /* __PHYSFS_platformGetThreadID */


/* -ansi and -pedantic flags prevent use of strcasecmp() on Linux. */
int __PHYSFS_platformStricmp(const char *x, const char *y)
{
    int ux, uy;

    do
    {
        ux = toupper((int) *x);
        uy = toupper((int) *y);
        if (ux > uy)
            return(1);
        else if (ux < uy)
            return(-1);
        x++;
        y++;
    } while ((ux) && (uy));

    return(0);
} /* __PHYSFS_platformStricmp */


int __PHYSFS_platformExists(const char *fname)
{
    struct stat statbuf;
    return(stat(fname, &statbuf) == 0);
} /* __PHYSFS_platformExists */


int __PHYSFS_platformIsSymLink(const char *fname)
{
#if (defined __PHYSFS_NO_SYMLINKS__)
    return(0);
#else

    struct stat statbuf;
    int retval = 0;

    if (lstat(fname, &statbuf) == 0)
    {
        if (S_ISLNK(statbuf.st_mode))
            retval = 1;
    } /* if */
    
    return(retval);

#endif
} /* __PHYSFS_platformIsSymlink */


int __PHYSFS_platformIsDirectory(const char *fname)
{
    struct stat statbuf;
    int retval = 0;

    if (stat(fname, &statbuf) == 0)
    {
        if (S_ISDIR(statbuf.st_mode))
            retval = 1;
    } /* if */
    
    return(retval);
} /* __PHYSFS_platformIsDirectory */


char *__PHYSFS_platformCvtToDependent(const char *prepend,
                                      const char *dirName,
                                      const char *append)
{
    int len = ((prepend) ? strlen(prepend) : 0) +
              ((append) ? strlen(append) : 0) +
              strlen(dirName) + 1;
    char *retval = malloc(len);

    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    /* platform-independent notation is Unix-style already.  :)  */

    if (prepend)
        strcpy(retval, prepend);
    else
        retval[0] = '\0';

    strcat(retval, dirName);

    if (append)
        strcat(retval, append);

    return(retval);
} /* __PHYSFS_platformCvtToDependent */


/* Much like my college days, try to sleep for 10 milliseconds at a time... */
void __PHYSFS_platformTimeslice(void)
{
    struct timespec napTime;
    napTime.tv_sec = 0;
    napTime.tv_nsec = 10 * 1000 * 1000;  /* specified in nanoseconds. */
    nanosleep(&napTime, NULL);           /* don't care if it fails. */
} /* __PHYSFS_platformTimeslice */


LinkedStringList *__PHYSFS_platformEnumerateFiles(const char *dirname)
{
    LinkedStringList *retval = NULL;
    LinkedStringList *l = NULL;
    LinkedStringList *prev = NULL;
    DIR *dir;
    struct dirent *ent;

    errno = 0;
    dir = opendir(dirname);
    BAIL_IF_MACRO(dir == NULL, strerror(errno), NULL);

    while (1)
    {
        ent = readdir(dir);
        if (ent == NULL)   /* we're done. */
            break;

        l = (LinkedStringList *) malloc(sizeof (LinkedStringList));
        if (l != NULL)
            break;

        l->str = (char *) malloc(strlen(ent->d_name) + 1);
        if (l->str == NULL)
        {
            free(l);
            break;
        } /* if */

        if (retval == NULL)
            retval = l;
        else
            prev->next = l;

        prev = l;
        l->next = NULL;
    } /* while */

    closedir(dir);
    return(retval);
} /* __PHYSFS_platformEnumerateFiles */


/* end of unix.c ... */

