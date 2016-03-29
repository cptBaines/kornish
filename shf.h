#define SHF_BSIZE	512

#if __linux__
typedef int shfd;
#elif _WIN32
#include <windows.h>
typedef HANDLE shfd;
#endif

#define shf_fileno(shf)		((shf)->fd)
#define shf_setfileno(shf,nfd)	((shf)->fd = (nfd))
#define shf_getc(shf)		((shf)->rnleft > 0 ? (shf)->rnleft--, *(shf)->rp++ \
		                : shf_getchar(shf))
#define shf_putc(c, shf)	((shf)->wnleft == 0 ? shf_putchar((c), (shf)) \
	                        : (shf)->wnleft--, *(shf)->wp++ = (c))

#define shf_eof(shf)		((shf)->flags & SHF_EOF)
#define shf_error(shf)		((shf)->flags & SHF_ERROR)
#define shf_errno(shf)		((shf)->errno_)
#define shf_clearerr(shf)	((shf)->flags &= ~(SHF_EOF | SHF_ERROR))


/* OS agnostic file creation flags */
#define GIO_RD		0x0001
#define GIO_WR		0x0002
#define GIO_RDWR	(GIO_RD | GIO_WR)
#define GIO_ACCMODE	0x0003			/* mask */
#define GIO_CREATE	0x0010			/* creat not exists */
#define GIO_TRUNC	0x0020			/* truncate file if exist */
#define GIO_APPEND	0x0030			/* Append to file */
#define GIO_EXCL	0x0040			/* Append to file */

/* Flags passed to shf_*open() */
#define SHF_RD		0x0001
#define SHF_WR		0x0002
#define SHF_RDWR 	(SHF_RD | SHF_WR)	
#define SHF_ACCMODE	0x0003			/* mask */
#define SHF_GETFL	0x0004			/* use fcntl() to figure out RD/RW flags */
#define SHF_UNBUF	0x0008			/* unbuffered I/O */
#define SHF_CLEXEC	0x0010			/* set close on exec flag */
#define SHF_MAPHI	0x0020			/* make fd > FDBASE */
#define SHF_DYNAMIC	0x0040			/* string: increas buffer as needed */
#define SHF_INTERRUPT	0x0080			/* EINTR in read/write causes error */

/* Internal flags */
#define SHF_STRING	0x0100			/* a string not a file */
#define SHF_ALLOCS	0x0200			/* shf and shf->buf where alloc()ed */
#define SHF_ALLOCB	0x0400			/* shf->buf was alloc()ed */
#define SHF_ERROR	0x0800			/* read()/write() error */
#define SHF_EOF		0x1000			/* read eof (sticky) */
#define SHF_READING	0x2000			/* currently reading: rnleft, rp valid */
#define SHF_WRITING	0x4000			/* currently writing: wnleft, wp valid */

struct shf {
	int flags;		/* see SHF_* */
	unsigned char *rp;	/* read: current position in buffer */
	int rbsize;		/* size of buffer (1 if SHF_UNBUF) */
	int rnleft;		/* read: no bytes left in buffer */
	unsigned char *wp;	/* write: current position in buffer */
	int wbsize;		/* size of buffer (0 if SHF_UNBUF) */
	int wnleft;		/* write: no bytes left in buffer */
	unsigned char *buf;	/* buffer */
	int bsize;		/* actual size of buffer */
	int errno_;		/* saved value of errno after error */
	Area *areap;		/* Area where shf/buf were allocated */
	shfd fd;
};

struct shf * shf_open(const char *name, int oflags, int mode, int sflags);
struct shf * shf_fdopen(shfd fd, int sflags, struct shf *shf);
//struct shf * shf_reopen(shfd fd, int sflags, struct shf *shf);
struct shf * shf_sopen(char *buf, size_t bsize, int sflags, struct shf *shf);
int shf_close(struct shf *shf);
int shf_fdclose(struct shf *shf);
char *shf_sclose(struct shf *shf);
int shf_finish(struct shf *shf);
int shf_flush(struct shf *shf);
int shf_seek(struct shf *shf, long where, int from);
int shf_read(char *buf, size_t bsize, struct shf *shf);
char *shf_getse(char *buf, size_t bsize, struct shf *shf);
int shf_getchar(struct shf *shf);
int shf_ungetc(int c, struct shf *shf);
int shf_putchar(int c, struct shf *shf);
int shf_puts(const char *c, struct shf *shf);
int shf_write(const char *buf, size_t bsize, struct shf *shf);
//int shf_fprintf(struct sfh *shf, const char *fmt, ...);
//int shf_fprintf(struct sfh *shf, const char *fmt, va_alist);
//int shf_snprintf(char *buf, int bsize, const char *fmt, ...);
//int shf_snprintf(char *buf, int bsize, const char *fmt, va_alist);
//int shf_smprintf(const char *fmt, ...);
//int shf_smprintf(const char *fmt, va_alist);
//int shf_vfprintf(struct sfh *shf, const char *fmt, va_list args);

