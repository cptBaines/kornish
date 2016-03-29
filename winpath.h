#ifndef WINPATH_H
#define WINPATH_H
/* returns a pointer to a new string with correct 
 * windows namespaces and \ separators.
 * returned string must be freed by caller calling free();
 **/
char *shell_2_win_path(const char* shpath);
#endif
