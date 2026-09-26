#ifndef PATH_H
#define PATH_H

/*
 * Finds the executable for a command name (Part 4: $PATH search).
 *
 * If cmd contains a '/', it is used as-is and no search is done.
 * Otherwise each directory in $PATH is tried in order, and the first
 * dir/cmd that is an executable regular file wins.
 *
 * Returns a malloc'd path the caller must free, or NULL if the command
 * was not found (the caller reports "command not found").
 */
char *search_path(const char *cmd);

#endif