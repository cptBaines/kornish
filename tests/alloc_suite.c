#include <CUnit.h>
#include "..\sh.h"

#define SUITE_NAME "Area alloc test suite"

#define NPTR 9
#define NCELLS 124 

Area aperm;
Area a;
char *p[NPTR];

/* return 0 on succes non-zero otherwize */
static int setup(void)
{
	ainit(&a);
	for (int i = 0; i < NPTR; i++) {
		p[i] = alloc(NCELLS, &a);
		//CU_ASSERT_PTR_NOT_NULL(p[i]);
	}
	printf("\n");
	
	return 0;
}

/* return 0 on succes non-zero otherwize */
static int teardown(void)
{
	afreeall(&a);
	return 0;
}

void test_free_odd(void) 
{
	afree(p[1], &a);
	afree(p[3], &a);
	afree(p[5], &a);
	afree(p[7], &a);
}

void test_free_even(void) 
{
	afree(p[0], &a);
	afree(p[2], &a);
	afree(p[4], &a);
	afree(p[6], &a);
	afree(p[8], &a);
}

void test_free_all(void) 
{
	afree(p[0], &a);
	afree(p[1], &a);
	afree(p[2], &a);
	afree(p[3], &a);
	afree(p[4], &a);
	afree(p[5], &a);
	afree(p[6], &a);
	afree(p[7], &a);
	afree(p[8], &a);
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

int reg_alloc_suite(void)
{
	CU_pSuite suite = NULL;

	/* create suite struct */
	suite = CU_add_suite(SUITE_NAME, setup, teardown);
	if (NULL == suite) 
		return CU_get_error();

	/* register test funct */
	ADD_TEST("Free odd posts", test_free_odd);
	ADD_TEST("Free even posts", test_free_even);

	return CUE_SUCCESS;
}
