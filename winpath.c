//#include "sh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include "winpath.h"

static int has_namespace(char *sp);

#define WN_NA      0 /* Not set assume file */
#define WN_FILE    1 /* \\?\<drive>:\path */
#define WN_DEVICE  2 /* \\.\<physicaldrive> */
#define WN_UNC     3 /* \\UNC\<server>\<share> */
#define WN_NUM     4 /* number of entries in array */	

char *win_namespaces[WN_NUM] = {"", "\\\\?\\", "\\\\.\\", "\\\\UNC\\"};


static int has_namespace(char *sp)
{
	char buf[7]; /* \\\\UNC\\ */
	char c;

	/* change any '/' to '\\' to ease comparation */
	for (int i = 0; i < 6; i++) {
		if (sp[i] == '/') 
			buf[i] = '\\';
		else
			buf[i] = sp[i];
	}
	buf[6] = 0; 
	
	if (buf[0] != '\\' && buf[1] != '\\') {
		return 0;
	}

	c = buf[2];
	if ((c == '.' || c == '?') && buf[3] == '\\') {
		/* \\.\ or \\?\ matched */
	       return 4;	
	}

	if (!( c == 'U')) {
		return 0;
	}

	if (buf[3] == 'N' && buf[4] == 'C' && buf[5] == '\\') {
		/* \\UNC\  matched */
		return 6;
	}	

	return 0;
}

/* shell paths are mounted at root and accepts both '/'
 * and '\' as separators
 *
 * /c/<paht>/file | \c\<path>\file | c:\<path>\<file> | c:/<path>/file
 * should all be translated into: \\.\c:\<path>\<file> for windows
 * paths starting with // or \\ will be assumed to mean UCN paths 
 * if not part of an actual namespace
 *
 * No path expansion will be performed after this point so all paths
 * should be absolute
 */
char *shell_2_win_path(const char* shpath)
{
	int len = 0;
	char *newpath = NULL;
	char *np, *sp;
	char *fns; 
	int nslen;

	newpath = malloc(strlen(shpath) + 12);
	memset(newpath, 0, strlen(shpath) + 12);

	if (newpath == NULL)
		return NULL;

	sp = (char*)shpath;
	np = newpath;

	/* check if we have namespace */
	nslen = has_namespace(sp);
	if (nslen == 0) {

		/* if pat starts with "//" or "\\" assumen UNC */
		if ((sp[0] == '\\' || sp[0] == '/')
		       && (sp[1] == '\\' || sp[1] == '/')) {
			fns = win_namespaces[WN_UNC];
		} else {
			fns = win_namespaces[WN_FILE];
		}

		/* setup namespace and dest pointer */ 
		len =  strlen(fns);
		strncpy(newpath, fns, len);
		np = newpath + len;
	} else {
		/* copy namespace to new buffer */
		while(sp < (shpath + nslen)) {
			if (*sp == '/') {
		 		*np++ = '\\';
				sp++;
			} else {
				*np++ = *sp++;
			}
		}
	} 

	/* skip any leading slashes */
	while (*sp != 0 && (*sp == '\\' || *sp == '/')) {
		sp++;
	}

	/* fix drive letter variants ie \c\ and /c/ 
	 * this means that one letter servernames will 
	 * not be handled */
	if (isalpha((int)*sp)) {
		*np++ = *sp++;
		if (*sp != 0 && (*sp == '/' || *sp == '\\')) {
			*np++ = ':';
			*np++ = '\\';		
			*sp++;
		}
	}

	while (*sp != 0) {
		if (*sp == '/') {
	 		*np++ = '\\';
			sp++;
		} else {
			*np++ = *sp++;
		}
	}

	return  newpath;
}
