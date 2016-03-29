#include <CUnit.h>
#include "..\sh.h"
//#include "..\shf.h"

#define SUITE_NAME "File I/O test suite"

#define FNAME "c:\\Users\\senbjo\\documents\\c_code\\shell\\output\\tf.txt"


Area a;

/* return 0 on succes non-zero otherwize */
static int setup(void)
{
	ainit(&a);
	
	return 0;
}

/* return 0 on succes non-zero otherwize */
static int teardown(void)
{
	afreeall(&a);
	return 0;
}

void test_write_file(void)
{
	char *msg = "Hello world";
	struct shf *f = shf_open(FNAME, GIO_WR | GIO_CREATE, 0, 0);
	CU_ASSERT_PTR_NOT_NULL(f);
       	CU_ASSERT_EQUAL(shf_write(msg, strlen(msg), f), 10);
	shf_close(f);
}

void test_PASS(void)
{
	CU_PASS("Passed with flying colours");
}

#define ADD_TEST(title, func) \
	if (NULL == CU_add_test(suite, (title), (func))) \
		return CU_get_error()

int reg_file_write_suite(void)
{
	CU_pSuite suite = NULL;

	/* create suite struct */
	suite = CU_add_suite(SUITE_NAME, setup, teardown);
	if (NULL == suite) 
		return CU_get_error();

	/* register test funct */
	ADD_TEST("Hello world", test_write_file);

	return CUE_SUCCESS;
}
