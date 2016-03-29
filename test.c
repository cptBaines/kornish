

#include "sh.h"

Area aperm;

extern int shf_putchar(int c, struct shf *shf);
extern Area *ainit(Area *ap);

int main(int argc, char **argv)
{
	struct shf *myfile;
	int c;

	ainit(&aperm);
	
	printf("main: aperm = 0x%p\n", &aperm);

	myfile = shf_open("/c/users/senbjo/documents/myfile.txt"
			, GIO_CREATE | GIO_TRUNC | GIO_WR, 0, 0);

	if (myfile == NULL) {
		printf("File creation failed");
		printf("Errorcode %d\n", GetLastError());
		return 1;
	}

	printf("wnleft = %d\n", myfile->wnleft);

	shf_putc('a', myfile);
	shf_putc('b', myfile);


	shf_close(myfile);
	
}
