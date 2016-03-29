#include <CUnit.h>
#include <stdlib.h>
#include "../winpath.h"

#define SUITE_NAME "Winpath test suite"
#define NPATTERNS 14

/* test pattern of single char, double and tripple char */
unsigned char u8[7] = { 0x24, 0xc2, 0xa2, 0xe2, 0x82, 0xac, 0x00 };

//unsigned char u16expected[7] = { 0x24, 0x00, 0xc2, 0xa2, 0xe2, 0x82, 0xac, 0x00 };

char *shpath[NPATTERNS] = {"c:\\mypath\\myfile.txt" 
		, "\\c\\mypath\\myfile.txt" 
		, "\\c:\\mypath\\myfile.txt" 
		, "c:/mypath/myfile.txt" 
		, "/c/mypath/myfile.txt" 
		, "/c:/mypath/myfile.txt" 
		, "\\\\myserver\\mypath\\myfile.txt" 
		, "//myserver/mypath/myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\.\\Physicaldevice0\\mypath\\myfile.txt" 
		, "//UNC/myserver/mypath/myfile.txt"
		, "//?/c:/mypath/myfile.txt" 
		, "//./Physicaldevice0/mypath/myfile.txt" 
		, "//UNC/myserver/mypath/myfile.txt" };

char *expected[NPATTERNS] = {"\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\UNC\\myserver\\mypath\\myfile.txt" 
		, "\\\\UNC\\myserver\\mypath\\myfile.txt" 
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\.\\Physicaldevice0\\mypath\\myfile.txt" 
		, "\\\\UNC\\myserver\\mypath\\myfile.txt"
		, "\\\\?\\c:\\mypath\\myfile.txt" 
		, "\\\\.\\Physicaldevice0\\mypath\\myfile.txt" 
		, "\\\\UNC\\myserver\\mypath\\myfile.txt" };

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

static void verify_pattern(int idx)
{
	char *p = shell_2_win_path(shpath[idx]);
	CU_ASSERT_PTR_NOT_NULL(p);
	if (p != NULL) {
		CU_ASSERT_STRING_EQUAL(p, expected[idx]);
		free(p);
	}
}

static void test_convert0(void) { verify_pattern(0); }
static void test_convert1(void) { verify_pattern(1); }
static void test_convert2(void) { verify_pattern(2); }
static void test_convert3(void) { verify_pattern(3); }
static void test_convert4(void) { verify_pattern(4); }
static void test_convert5(void) { verify_pattern(5); }
static void test_convert6(void) { verify_pattern(6); }
static void test_convert7(void) { verify_pattern(7); }
static void test_convert8(void) { verify_pattern(8); }
static void test_convert9(void) { verify_pattern(9); }
static void test_convert10(void) { verify_pattern(10); }
static void test_convert11(void) { verify_pattern(11); }
static void test_convert12(void) { verify_pattern(12); }
static void test_convert13(void) { verify_pattern(13); }

int reg_winpath_suite(void)
{
	CU_pSuite suite = NULL;

	/* create suite struct */
	suite = CU_add_suite(SUITE_NAME, setup, teardown);
	if (NULL == suite) 
		return CU_get_error();

	/* register test funct */
	if (NULL == CU_add_test(suite, shpath[0], test_convert0))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[1], test_convert1))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[2], test_convert2))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[3], test_convert3))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[4], test_convert4))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[5], test_convert5))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[6], test_convert6))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[7], test_convert7))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[8], test_convert8))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[9], test_convert9))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[10], test_convert10))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[11], test_convert11))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[12], test_convert12))
		return CU_get_error();
	if (NULL == CU_add_test(suite, shpath[13], test_convert13))
		return CU_get_error();

	return CUE_SUCCESS;
}
