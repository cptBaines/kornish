#if __linux__
#elif _WIN32
#include <windows.h>
#endif
#include <limits.h>
#include "sh.h"
#include "winpath.h"
#include "utf8.h"
#include "errno.h"
#include "printf/xprintf.h"


#define EB_READSW	0x01	/* about to switch to reading */
#define EB_GROW		0x02	/* grow buf if needed (STRING+DYNAMIC) */

#define HASFLAG(v, f) (((v) & (f)) == (f))

static int shf_fillbuf(struct shf *shf);
static int shf_emptybuf(struct shf *shf, int flags);

static void interal_error(int i, char *msg);

extern void *alloc(size_t size, Area *ap);
//extern void *aresize(unsigned char *buf, size_t size, Area *ap);

#if __linux__
#  define INVALID_FD(shf) ((shf)->fd < 0)
#  define SET_INVALID_FD(shf) (shf)->fd = -1;
#elif _WIN32
#  define INVALID_FD(shf) ((shf)->fd == INVALID_HANDLE_VALUE)
#  define SET_INVALID_FD(shf) (shf)->fd = INVALID_HANDLE_VALUE;
#endif

#if _WIN32
static int gio2disp(int gflags);

static void internal_error(int i, char *msg)
{
		fprintf(stderr, "%s\n", msg);
}

static void internal_errorf(int i, char *msg, ...)
{
	internal_error(i, msg);
}

/* get correct CreationDiposition flag from gio flags */
static int gio2disp(int gflags)
{
	if (HASFLAG(gflags, GIO_CREATE))  {
		if (HASFLAG(gflags, GIO_EXCL))
	       		return CREATE_NEW;   /* fail if file exists */
	
		if (HASFLAG(gflags, GIO_TRUNC))
			return CREATE_ALWAYS;	/* create and overwrite */

		if (HASFLAG(gflags, GIO_APPEND))
			return OPEN_ALWAYS;	/* open or create */
	}	

	if (HASFLAG(gflags, GIO_TRUNC))
		return TRUNCATE_EXISTING;   /* fails if not exists */

	if (HASFLAG(gflags, GIO_APPEND))
		return OPEN_EXISTING; 	    /* failse if not exists */

	return OPEN_EXISTING;	/* default */
}

static int gio2acces(int gflags) 
{
	return (gflags & GIO_ACCMODE) == GIO_RD ? GENERIC_READ
		  : ((gflags & GIO_ACCMODE) == GIO_WR ? GENERIC_WRITE
 			: GENERIC_READ | GENERIC_WRITE);
}

static int gio2share(int gflags)
{
	return (gflags & GIO_ACCMODE) == GIO_RD ? FILE_SHARE_READ
		  : ((gflags & GIO_ACCMODE) == GIO_WR ? FILE_SHARE_WRITE
 			: FILE_SHARE_READ | FILE_SHARE_WRITE);
}
#endif

struct shf * shf_open(const char *name, int oflags, int mode, int sflags)
{
	shfd fd;

#if __linux__
	fd = fopen(name, oflags, mode);
	if (fd < 0)
		return NULL;

	if ((sflags & SHF_MAPHI) && fd < FDBASE) {
		int nfd;

		nfd = fcntl(fd, F_DUPFD, FDBASE);
		close(fd);
		if (nfd < 0)
			return NULL;
		fd = nfd;
	}	


#elif _WIN32
	unsigned char buf[USHRT_MAX];
	char *p = shell_2_win_path(name);
	if (p == NULL)
		return NULL;
	int nwrite = utf8_to_utf16le(buf, USHRT_MAX, (unsigned char*)p); 
	free(p);

	if (nwrite == -1)
		return NULL;

	wchar_t *winpath = (wchar_t*)&buf;

	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	fd = CreateFileW(winpath
		, gio2acces(oflags)
		, gio2share(oflags) 
		, &sa
		, gio2disp(oflags) 
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	
	if (fd == INVALID_HANDLE_VALUE)
		return NULL;

	/* SHF_MAPHI  Not implemented in windows */
	/* Need some method to differentiate shell and user handles */

#endif
	sflags &= ~SHF_ACCMODE;
	sflags |= (oflags & GIO_ACCMODE) == GIO_RD ? SHF_RD
		  : ((oflags & GIO_ACCMODE) == GIO_WR ? SHF_WR
 			: SHF_RDWR);

	return shf_fdopen(fd, sflags, (struct shf *) 0);
}

/* setup the shf structure for a shf filedescriptor. Does not fail */
struct shf * shf_fdopen(shfd fd, int sflags, struct shf *shf)
{
	int bsize = sflags & SHF_UNBUF 
		? (sflags & SHF_RD ? 1 : 0)
		: SHF_BSIZE;

	if (sflags & SHF_GETFL) {
#if __linux__
		int flags = fnctl(fd, F_GETFL, 0);

		if (flags < 0) {
			/* will get an error on first read/write */
			sflags |= SHF_RDWR;
		} else {
			switch (flags & O_ACCMODE) {
				case O_RDONLY: sflags |= SHF_RD; break;
				case O_RWONLY: sflags |= SHF_WR; break;
				case O_RDWR: sflags |= SHF_RDWR; break;
			}
		}
#elif _WIN32
		/* Possibly use GetSecurityInfo(...) */
#endif
	}

	if (!(sflags & (SHF_RD | SHF_WR))) {
		internal_error(1, "shf_fdopen: missing read/write");
	}

	if (shf) {
		if (bsize) {
			printf("alloc buf only\n");
			shf->buf = (unsigned char *) alloc(bsize, ATEMP);
			sflags |= SHF_ALLOCB;
		} else {
			shf->buf = (unsigned char *) 0;
		}
	} else {
		printf("alloc struct and buf\n");
		shf = (struct shf *) alloc(sizeof(struct shf) + bsize , ATEMP);
		shf->buf = (unsigned char *) &shf[1];
		sflags |= SHF_ALLOCS;
		printf("alloced\n");
	}

	shf->areap = ATEMP;
	shf->fd = fd;
	shf->rp = shf->wp = shf->buf;
	shf->rnleft = 0;
	shf->rbsize = bsize;
	shf->wnleft = 0;
	shf->wbsize = sflags & SHF_UNBUF ? 0 : bsize;
	shf->flags = sflags;
	shf->errno_ = 0;
	shf->bsize = bsize;
#if __linux__
	if (sflags & SHF_CLEXEC)
		fd_clexec(fd);
#elif _WIN32
		/* not implemented */
#endif
	return shf;
}

/* Open a string for readint or writing.  If reading, bsize is the numer
 * of bytes that can be read.  If writing, bsize is the maximum number of
 * bytes that can be written.  If shf is not null, it is filled in and 
 * returned, if it is null, shf is allocated,  If writing and buf is null
 * and SHF_DYNAMIC is set, the buffer is allocated (if bsize > 0, it is
 * used for the initial size).  Doesn't fail.
 * When writing, a byte is reserved for trailing null - see shf_sclose().
 */ 
struct shf * shf_sopen(char *buf, size_t bsize, int sflags, struct shf *shf)
{
	if (!(sflags & (SHF_RD | SHF_WR))
		|| (sflags & (SHF_RD | SHF_WR)) == (SHF_RD | SHF_WR))
		//internal_error(1, "shf_open: flags 0x%x", sflags);
		internal_error(1, "shf_open: flags 0x");

	if (!shf) {
		shf = (struct shf*) alloc(sizeof(struct shf), ATEMP);
		sflags |= SHF_ALLOCS;
	}

	shf->areap = ATEMP;
	if (!buf && (sflags & SHF_WR) && (sflags & SHF_DYNAMIC)) {
		if (bsize <= 0)
			bsize = 64;
		sflags |= SHF_ALLOCB;
		buf = alloc(bsize, shf->areap);
	}
	SET_INVALID_FD(shf);
	shf->buf = shf->rp = shf->wp = (unsigned char *)buf;
	shf->rnleft = bsize;
	shf->rbsize = bsize;
	shf->wnleft = bsize - 1; /* reserve space for '\0' */
	shf->wbsize = bsize;
	shf->flags = sflags | SHF_STRING;
	shf->errno_ = 0;
	shf->bsize = bsize;

	return shf;
}

/* Flush and free file structure, don't close file descriptor */
int shf_finish(struct shf *shf)
{
	int ret = 0;

	if (!INVALID_FD(shf)) 
		ret = shf_flush(shf);
	if (shf->flags & SHF_ALLOCS)
		afree(shf, shf->areap);
	else if (shf->flags & SHF_ALLOCB)
		afree(shf->buf, shf->areap);

	return ret;	
}

/* Un-read what has been read bu not examined, or write what has 
 * been bufferd. Return 0 for success, EOF for (write) error.
 */
int shf_flush(struct shf *shf)
{
	if (shf->flags & SHF_STRING)
		return (shf->flags & SHF_WR) ? EOF : 0;

	if (INVALID_FD(shf)) {
		internal_error(1, "shf_fdopen: missing read/write");
	}
#if __linux__
	if (shf->flags & SHF_ERROR) {
		errno = shf->errno_;
		return EOF;
	}
	
	if (shf->flags & SHF_READING) {
		shf->flags &= ~(SHF_EOF | SHF_READING);
		if (shf->rnleft > 0) {
			lseek(shf->fd, (off_t) -shf->rnleft, 1);
			shf->rnleft = 0;
			shf-rp = shf->buf;
		}
		return 0;
#elif _WIN32
	if (shf->flags & SHF_ERROR) {
		/* maybe this should set errno as well?? */
		//SetLastError(shf->errno_);
		errno = shf->errno_;
		return EOF;
	}
	
	if (shf->flags & SHF_READING) {
		shf->flags &= ~(SHF_EOF | SHF_READING);
		if (shf->rnleft > 0) {
			SetFilePointer(shf->fd
				, -shf->rnleft
				, NULL, FILE_CURRENT);
			shf->rnleft = 0;
			shf->rp = shf->buf;
		}
		return 0;
#endif
	} else if (shf->flags & SHF_WRITING)
		return shf_emptybuf(shf, 0);
	return 0;

}

/* Write out any bufferd data. If currently readin. flushes the
 * read buffer. Returns 0 for success, EOF for (write) error
 */
static int shf_emptybuf(struct shf *shf, int flags)
{
	int ret = 0;

	if ((shf->flags & SHF_STRING) && INVALID_FD(shf)) {
		internal_error(1, "shf_emptybuf: no fd");
	}

	if (shf->flags & SHF_ERROR) {
		errno = shf->errno_;
		return EOF;
	}

	if (shf->flags & SHF_READING) {
		if (flags & EB_READSW)  /* does not happen */
			return 0;
		ret = shf_flush(shf);
		shf->flags &= ~SHF_READING;
	}
	if (shf->flags & SHF_STRING) {
		unsigned char *nbuf;

		/* Note that we assume SHF_ALLOCS is not set if 
		 * SHF_ALLOCB is set... (changing the shf pointer
		 * could cause problems)
		 */
		if (!(flags & EB_GROW) || !(shf->flags & SHF_DYNAMIC)
		    || !(shf->flags & SHF_ALLOCB))
			return EOF;

		nbuf = (unsigned char *) aresize(shf->buf
					, shf->wbsize * 2, shf->areap);
		shf->rp = nbuf + (shf->rp - shf->buf);
		shf->wp = nbuf + (shf->wp - shf->buf);
		shf->rbsize += shf->wbsize;
		shf->wbsize += shf->wbsize;
		shf->wnleft += shf->wbsize;
		shf->wbsize *= 2;
	      	shf->buf = nbuf;
	} else {
		if (shf->flags & SHF_WRITING) {
			int ntowrite = shf->wp - shf->buf;
			unsigned char *buf = shf->buf;
#if __linux__
			int n;
#elif _WIN32
			unsigned long int n;
#endif

			printf("empty_buf: writing: %d bytes\n", ntowrite);
			while (ntowrite > 0) {
#if __linux__
		 		n = write(shf->fd, buf, ntowrite);
				if (n < 0) {
						if (errno == EINTR
						    && !(shf->flags & SHF_INTERRUPT))
								continue;
					shf->flags |= SHF_ERROR;
					shf->errno_ = errno; 
					shf->wnleft = 0;
					if (buf != shf->buf) {
						/* allow a secod flush to work */
						memmove(shf->buf, buf, ntowrite);
						shf->wp = shf->buf + ntowrite;
					}
					return EOF;
				}
#elif _WIN32
			/* this call does not return until all data
			 * has been written */
				if (!WriteFile(shf->fd, buf, ntowrite, &n, NULL)) {
					shf->flags |= SHF_ERROR;
					shf->errno_ = GetLastError();
					shf->wnleft = 0;
					if (buf != shf->buf) {
						/* allow a secod flush to work */
						memmove(shf->buf, buf, ntowrite);
						shf->wp = shf->buf + ntowrite;
					}
					return EOF;
				}
#endif
			printf("wrote %d\n bytes", n);
			buf += n;
			ntowrite -= n;
			printf("ntowrite = %d\n", ntowrite);
			}
		}	
		if (flags & EB_READSW) {
				shf->wp = shf->buf;
				shf->wnleft = 0;
				shf->flags &= SHF_WRITING;
				return 0;
		}
		shf->wp = shf->buf;
		shf->wnleft = shf->wbsize;
	}
	shf->flags |= SHF_WRITING;

	return ret;
}

/* Fill up a buffer.  Return s EOF for a read error, 0 otherwise */
static int shf_fillbuf(struct shf *shf) 
{
	if (shf->flags & SHF_STRING)
		return 0;

	if (INVALID_FD(shf))
		internal_error(1, "shf_fillbuf: no fd\n");

	if (shf->flags & (SHF_EOF | SHF_ERROR)) {
		if (shf->flags & SHF_ERROR) 
			errno = shf->errno_;
		return EOF;
	}

	if ((shf->flags & SHF_WRITING) && shf_emptybuf(shf, EB_READSW) == EOF)
		return EOF;

	shf->flags |= SHF_READING;

	shf->rp = shf->buf;
#if __linux__
	while(1) {
		shf->rnleft = blocking_read(shf->fd, (char *) shf->buf
				, shf->rbsize);
		if (shf->rnleft < 0 && errno == EINTR
			&& !(shf->flags & SHF_INTERRUPT))
			continue;
		break;
	}
	if (shf->rnleft <= 0) {
		if (shf->rnleft < 0) {
			shf->flags |= SHF_ERROR;
			shf->errno_ = errno;
			shf->rnleft = 0;
			shf->rp = shf->buf;
			return EOF;
		}
		shf->flags |= SHF_EOF;
	}
#elif _WIN32
  	if (!ReadFile(shf->fd, shf->buf, shf->rbsize
			, &shf->rnleft, NULL)) {
		shf->flags |= SHF_ERROR;
		shf->errno_ = GetLastError();
		shf->rnleft = 0;
		shf->rp = shf->buf;
		return EOF;
	}
	if (shf->rnleft == 0)
		shf->flags |= SHF_EOF;
#endif
	return 0;
}
 

/* Flush and close file descriptor, don't free the file structure */
int shf_fdclose(struct shf *shf) {

	int ret = 0;

#if __linux__
	if (shf->fd >= 0) {
		ret = shf_flush(shf);
		if (close(shf->fd) < 0)
			ret = EOF;
#elif _WIN32
	if (shf->fd != INVALID_HANDLE_VALUE) {
		ret = shf_flush(shf);
		if (CloseHandle(shf->fd) == 0)
			ret = EOF;
#endif
		shf->rnleft = 0;
		shf->rp = shf->buf;
		shf->wnleft = 0;
		SET_INVALID_FD(shf);
	}
	return ret;
}

/* Flush and clode file, free shf structure */
int shf_close(struct shf *shf)
{
	int ret = 0;

	printf("Closing shf\n");
	
	if (!INVALID_FD(shf)) {
		ret = shf_flush(shf);
#if __linux__
		if (close(shf->fd) < 0)
			ret = EOF;
#elif _WIN32
		if (CloseHandle(shf->fd) == 0)
			ret = EOF;
#endif
	}
	if (shf->flags & SHF_ALLOCS)
		afree(shf, shf->areap);
	else if (shf->flags & SHF_ALLOCB)
		afree(shf->buf, shf->areap);

	return ret;
}

/* Close a string - if it was opened for writing, it is null terminated;
 * returns a pointer ot the string and frees shf if it was allocated
 * (does not free string if it was allocated).
 */
char *shf_sclose(struct shf *shf)
{
	unsigned char *s = shf->buf;

	/* null terminate */
	if (shf->flags & SHF_WR) {
		shf->wnleft++;
		shf_putc('\0', shf);
	}
	if (shf->flags & SHF_ALLOCS)
		afree(shf, shf->areap);

	return (char*)s;
}

static inline int trans_from(int from)
{
	if (from == SEEK_SET) return FILE_BEGIN;
	if (from == SEEK_CUR) return FILE_CURRENT;
	if (from == SEEK_END) return FILE_END;
	return FILE_CURRENT;	
}

/* Seek to a new positin in the files.  If writing, flushes the uffer
 * first. If reading, optimizes small realtive seeks tat stay inside the
 * buffer.  Returns 0 for success, EOF otherwise
 */
int shf_seek(struct shf *shf, long int where, int from)
{

	if (INVALID_FD(shf)) {
		/* Is this correct for windows ? */
		errno = EINVAL;
		return EOF;
	}

	if (shf->flags & SHF_ERROR) {
		errno = shf->errno_;
		return EOF;
	}

	if ((shf->flags & SHF_WRITING) && shf_emptybuf(shf, EB_READSW) == EOF)
		return EOF;

	if (shf->flags & SHF_READING) {
	       if (from == SEEK_CUR && (where < 0 
			       ? -where >= shf->rbsize - shf->rnleft
	 		       : where < shf->rnleft)) {

		       shf->rnleft -= where;
		       shf->rp += where;
		       return 0;
       		}		       
		shf->rnleft = 0;
		shf->rp = shf->buf;
	}

	shf->flags &= ~(SHF_EOF | SHF_READING | SHF_WRITING);

#if __linux__
		if (lseek(shf->fd, where, from) < 0) {
			shf->errno_ = errno;
			shf->flags |= SHF_ERROR;
			return EOF;
		}

#elif _WIN32
		/* FILE_CURRENT = 1, FILE_BEGIN = 0, FILE_END = 2*/
		
		if (SetFilePointer(shf->fd
				, where 
				, NULL, trans_from(from))) {
			shf->errno_ = GetLastError();
			shf->flags |= SHF_ERROR;
			return EOF;
		}
		return 0;
#endif
	return 0;
}

/* Read a buffer from shf.  Returns the number of bytes read into buf, 
 * i fno bytes were read, return 0 if end of file was seen, EOF if
 * a read error occurred.
 */
int shf_read(char *buf, size_t bsize, struct shf *shf)
{
	int orig_bsize = bsize;
	int ncopy;

	if (!(shf->flags & SHF_RD))
		internal_errorf(1, "shf_read: flags %x", shf->flags);

	if (bsize <= 0)
		internal_errorf(1, "shf_read: bsize %d", bsize);

	while (bsize > 0) {
		if (shf->rnleft == 0
		    && (shf_fillbuf(shf) == EOF || shf->rnleft == 0))
			break;
		ncopy = shf->rnleft;
		if (ncopy > bsize)
			ncopy = bsize;
		memcpy(buf, shf->rp, ncopy);
		buf += ncopy;
		bsize -= ncopy;
		shf->rp += ncopy;
		shf->rnleft -= ncopy;
	}

	return orig_bsize == bsize ? (shf_error(shf) ? EOF : 0)
			           : orig_bsize - bsize;
}

/* Read up to a newline or EOF.  The newline is put on buf; buf is always
 * null terminated.  Returns NULL on read error or if nothing was read
 * before end of filr, returns a pointer to the null byte otherwise.
 */

char *shf_getse(char *buf, size_t bsize, struct shf *shf)
{
	unsigned char *end;
	int ncopy;
	char *orig_buf = buf;

	if (!(shf->flags & SHF_RD))
		internal_errorf(1, "shf_getse: flags %x", shf->flags);

	if (bsize <= 0)
		return (char*)0;

	--bsize;
	do {
		if (shf->rnleft == 0) {
			if (shf_fillbuf(shf) == EOF)
				return NULL;
			if (shf->rnleft == 0) {
				*buf = '\0';
				return buf == orig_buf ? NULL : buf;
			}
		}
		end = (unsigned char *) memchr((char *) shf->rp, '\n'
				, shf->rnleft);
		ncopy = end ? end - shf->rp + 1 : shf->rnleft;
		if (ncopy > bsize)
			ncopy = bsize;
		memcpy(buf, (char *) shf->rp, ncopy);
		shf->rp += ncopy;
		shf->rnleft -= ncopy;
		buf += ncopy;
		bsize -= ncopy;
	} while (!end && bsize);
	*buf = '\0';
	return buf;
}

/* Returns the char read.  Returns EOF for error and end of file */
int shf_getchar(struct shf *shf)
{
	if (!(shf->flags & SHF_RD))
		internal_errorf(1, "shf_getchar: flags %x", shf->flags);

	if (shf->rnleft == 0 && (shf_fillbuf(shf) == EOF || shf->rnleft == 0))
		return EOF;
	--shf->rnleft;
	return *shf->rp++;
}

/* Put a character back in the input stream.  Returns the character if
 * successful, EOF if there is no room.
 */
int shf_ungetc(int c, struct shf *shf)
{
	if (!(shf->flags & SHF_RD))
		internal_errorf(1, "shf_ungetc: flags %x", shf->flags);

	if ((shf->flags & SHF_ERROR) || c == EOF
		|| (shf->rp == shf->buf && shf->rnleft))
		return EOF;

	if ((shf->flags & SHF_WRITING) && shf_emptybuf(shf, EB_READSW) == EOF)
		return EOF;

	if (shf->rp == shf->buf)
		shf->rp = shf->buf + shf->rbsize;

	if (shf->flags & SHF_STRING) {
		/* Can unget what was read, but no something different
		 * - we don't want to modify a string
		 */
		if (shf->rp[-1] != c)
			return EOF;
		shf->flags &= ~SHF_EOF;
		shf->rp--;
		shf->rnleft++;
		return c;
	}
	shf->flags &= ~SHF_EOF;
	*--(shf->rp) = c;
	shf->rnleft++;
	return c;
}

/* Write a character. Returns the character if successful, EOF if 
 * the char could not be written.
 */
int shf_putchar(int c, struct shf *shf)
{
		if (!(shf->flags & SHF_WR)) 
			//internal_error(1, "shf_putchar: flags %x", shf->flags);
			internal_error(1, "shf_putchar: flags");

		if (c == EOF)
			return EOF;

		if (shf->flags & SHF_UNBUF) {
			printf("Unbufferd\n");
			char cc = c;
			int n = 0;

			if (INVALID_FD(shf)) 
				internal_error(1, "shf_putchar: no fd");
			if (shf->flags & SHF_ERROR) {
				errno = shf->errno_;
				return EOF;
			}
#if __linux__
			while((n = write(shf->fd, &cc, 1)) != 1) {
				if (n < 0) {
					if (errno == EINTR
					    && !(shf->flags & SHF_INTERRUPT))
							continue;
					shf->flags |= SHF_ERROR;
	 				shf->errno_ = errno; 
					return EOF;
				}
			}
#elif _WIN32
			/* this call does not return until all data
			 * has been written */
			if (!WriteFile(shf->fd, &cc, 1, &n, NULL)) {
				shf->flags |= SHF_ERROR;
				shf->errno_ = GetLastError();
				return EOF;
			}
#endif
		} else {
			printf("Bufferd\n");
			if (shf->wnleft == 0 && shf_emptybuf(shf, EB_GROW) == EOF)
					return EOF;
			shf->wnleft--;
			*shf->wp++ = c;
			printf("wrote: %c\n", *(shf->wp - 1));
		}
		return c;
}


/* Write a string. Returnst the length of the string if successful, EOF
 * if the string could not be written
 */
int shf_puts(const char *s, struct shf *shf)
{
	if (!s)
		return EOF;

	return shf_write(s, strlen(s), shf);
}

/* Write a buffer. Returns nbytes if successful, EOF if there is an error */
int shf_write(const char *buf, size_t nbytes, struct shf *shf)
{
	int orig_nbytes = nbytes;
	int n;
	int ncopy;

	if (!(shf->flags & SHF_WR))
		internal_errorf(1, "shf_write: flags %x", shf->flags);

	if (nbytes < 0)
		internal_errorf(1, "shf_write: nbytes %d", nbytes);

	if ((ncopy = shf->wnleft)) {
		if (ncopy > nbytes)
			ncopy = nbytes;
		memcpy(shf->wp, buf, ncopy);
		nbytes -= ncopy;
		buf += ncopy;
		shf->wp += ncopy;
		shf->wnleft -= ncopy;
	}
	if (nbytes > 0) {
		/* Flush deals with strings and sticky errors */
		if (shf_emptybuf(shf, EB_GROW) == EOF)
			return EOF;
		if (nbytes > shf->wbsize) {
			ncopy = nbytes;
			if (shf->wbsize)
				ncopy -= nbytes % shf->wbsize;
			nbytes -= ncopy;
			while(ncopy > 0) {
#if __linux__
				n = write(shf->fd, buf, ncopy);
				if (n < 0) {
					if (errno == EINTR) {
						&& !(shf->flags & SHF_INTERRUPT))
						continue;
					}
					shf->flags |= SHF_ERROR;
					shf->errno_ = errno;
					shr->wnleft = 0;
					/* Note fwrite(3S) return 0 
					 * for errors - this doesn't */
					return EOF;
				}
#elif _WIN32
				if (!WriteFile(shf->fd, buf, ncopy
						, &n, NULL)) {
					shf->flags |= SHF_ERROR;
					shf->errno_ = GetLastError();
					shf->wnleft = 0;
					return EOF;
				}
#endif
				buf += n;
				ncopy -= n;
			}
		}
		if (nbytes > 0) {
			memcpy(shf->wp, buf, nbytes);
			shf->wp += nbytes;
			shf->wnleft -= nbytes;
		}
	}
	return orig_nbytes;

}

_ssize_t xshf_out(void * ofn_arg, const char * buf, size_t size)
{
	shf_write(buf, size, (struct shf*)ofn_arg);
}

int shf_vfprintf(struct sfh *shf, const char *fmt, va_list args)
{
	return (int) xprintf(xshf_out, (void*)shf, fmt, &args);
}

