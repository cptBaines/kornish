#include <CUnit.h>
#include <Basic.h>

/* must declare reg functions from external modules here */
extern int reg_utf_suite(void);
extern int reg_alloc_suite(void);
extern int reg_winpath_suite(void);
extern int reg_printf_suite(void);

/* main entrypoint for tests */
int main(int argc, char **argv)
{
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* register suites in other files */	
	if (reg_alloc_suite() != CUE_SUCCESS) goto cleanup;
	if (reg_winpath_suite() != CUE_SUCCESS) goto cleanup;
	if (reg_utf_suite() != CUE_SUCCESS) goto cleanup;
	if (reg_printf_suite() != CUE_SUCCESS) goto cleanup;
	/* ... */

	/* run tests */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

cleanup:
	CU_cleanup_registry();
	return CU_get_error();
}
