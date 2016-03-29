#include <CUnit.h>
#include <stdarg.h>
#include <string.h>
#include "..\printf\xprintf.h"

#define SUITE_NAME "Printf test suite"

struct sbuf {
	char *b;
	int pos;
};


/* return 0 on succes non-zero otherwize */
static int setup(void)
{
	return 0;
}

/* return 0 on succes non-zero otherwize */
static int teardown(void)
{
	return 0;
}

_ssize_t myout(void* dst, const char *src, size_t n)
{
	struct sbuf *sb = (struct sbuf*)dst;
	memcpy(sb->b + sb->pos, src, n);
	sb->pos += n;
	sb->b[sb->pos] = 0;
       	return n;	
}

_ssize_t mysprintf(struct sbuf *dst, const char *fmt, ...)
{
	_ssize_t len;
	va_list ap;
	va_start(ap, fmt);

	len = xprintf(myout, (void*)dst, fmt, &ap);
	va_end(ap);

	return len;
}

void test_xfloat(const char * fmt, double dval)
{
	struct sbuf sb;
	char myval[512];
	char stdval[512];
	char msg[80];

	sb.b = myval;
	sb.pos = 0;

	sprintf(msg, "\"%s\", %f", fmt, dval); 

	printf("\n%s ", msg);
	mysprintf(&sb, fmt, dval);
	sprintf(stdval, fmt, dval);
	printf("... |%s| != |%s| ", myval, stdval);
	CU_ASSERT_STRING_EQUAL(myval, stdval);
}

#define RESET_BUF(x, s)	do {	\
	memset((x).b, 0, (s));	\
	(x).pos = 0;		\
} while(0)
			
void test_mytest2(void)
{
	struct sbuf sb;
	char buf[512];

	sb.b = buf;
	
	RESET_BUF(sb, 512);
	mysprintf(&sb, "%5d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "    1");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "00001");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%-5d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "1    ");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%-05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "1    ");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%+5d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "   +1");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%+05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "+0001");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%+-5d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "+1   ");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%+-05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "+1   ");

	RESET_BUF(sb, 512);
	mysprintf(&sb, "%5.2d", 1);
	printf("\n|%s|\n", buf);
	CU_ASSERT_STRING_EQUAL(buf, "   01");
}

void test_mytest(void)
{
	struct sbuf sb;
	char buf[512];

	sprintf(buf, "%05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "00001");

	memset(buf, 0, 512);
	sb.b = buf;
	sb.pos = 0;

	mysprintf(&sb, "%05d", 1);
	CU_ASSERT_STRING_EQUAL(buf, "00001");
}

void test_xint(const char * fmt, int ival)
{
	struct sbuf sb;
	char myval[512];
	char stdval[512];
	char msg[80];

	sb.b = myval;
	sb.pos = 0;

	sprintf(msg, "\"%s\", %d", fmt, ival); 

	printf("\n%s ", msg);
	mysprintf(&sb, fmt, ival);
	sprintf(stdval, fmt, ival);
	printf("... |%s| != |%s| ", myval, stdval);
	CU_ASSERT_STRING_EQUAL(myval, stdval);

/*
	if (strcmp(myval, stdval) == 0)  {
		CU_PASS(msg);
	} else  {
		CU_FAIL(msg);
		printf("%s", msg);
	}
	*/
}

void test_d(void)
{
	test_xint("%d", 123);
	test_xint("%5d", 123);
	test_xint("%-5d", 123);
	test_xint("%05d", 123);
}

void test_f(void)
{
	test_xint("%e", 1.0/3);
}

/*
void test_PASS(void)
{
	CU_PASS("Passed with flying colours");
}
*/
#define ADD_TEST(title, func) \
	if (NULL == CU_add_test(suite, (title), (func))) \
		return CU_get_error()

int reg_printf_suite(void)
{
	CU_pSuite suite = NULL;

	/* create suite struct */
	suite = CU_add_suite(SUITE_NAME, setup, teardown);
	if (NULL == suite) 
		return CU_get_error();

	/* register test funct */
	ADD_TEST("Output d flag", test_d);
	ADD_TEST("Output f flag", test_f);
	ADD_TEST("Output my test", test_mytest);
	ADD_TEST("Output my test2", test_mytest2);

	return CUE_SUCCESS;
}
