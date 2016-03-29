#include <CUnit.h>
#include "..\utf8.h"
#include <stdio.h>

#define SUITE_NAME "utf test suite"

uint32_t cp8[4] = { 0x24, 0x00A2, 0x20AC, 0x10348 };
char *expect8[4] = { 
	  "\x24\x00" 
	, "\xC2\xA2\x00"
	, "\xE2\x82\xac\x00"
	, "\xf0\x90\x8D\x88\x00"
};

uint32_t cp16[4] = { 0x24, 0x20AC, 0x10437, 0x24b62 };
char *expect16[4] = { 
	  "\x24\x00\x00\x00" 
	, "\xAC\x20\x00\x00"
	, "\x01\xD8\x37\xDC\x00\x00"
	, "\x52\xD8\x62\xDF\x00\x00"
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

#define NCP 4
static void test_utf8_2_utf16le(void)
{
	uint32_t lcp[NCP] = { 0x24, 0x20AC, 0x10437, 0x24b62 };
	unsigned char b8[64];
	unsigned char b16[128];
	unsigned char buf[128];
	int n = 0;

	memset(b8, 0, 64);	
	memset(b16, 0, 128);	
	memset(buf, 0, 128);	

	unsigned char *p = b8;
	size_t sb = 64;
	/* create utf8 str */
	for (int i = 0; i < NCP; i++) {
		n = cp_2_utf8(p, sb, lcp[i]);
		if (n == 0) {
			CU_FAIL("code point translation returned zero");
		} else {
			p += n;
			sb -= n;	
		}
	}
	
	/* create utf16le string */
	p = b16;
	sb = 128;
	for (int i = 0; i < NCP; i++) {
		n = cp_2_utf16le(p, sb, lcp[i]);
		if (n == 0) {
			CU_FAIL("code point translation returned zero");
		} else {
			p += n;
			sb -= n;	
		}
	}

	/* convert utf8 str to utf16le */
	n = utf8_to_utf16le(buf, 128, b8);
	CU_ASSERT_NOT_EQUAL(n, -1);
	CU_ASSERT_STRING_EQUAL(buf, b16); 
}

static void test_nullpointer(void)
{
	uint32_t cp = 0;
	unsigned char *b = NULL;
	CU_ASSERT_FALSE(cp_2_utf8(NULL, 8, 0xD800));
	CU_ASSERT_FALSE(cp_2_utf16le(NULL, 8, 0xD800));
	CU_ASSERT_FALSE(utf8_2_cp(NULL, b, 8));
	CU_ASSERT_FALSE(utf16le_2_cp(NULL, b, 8));
	CU_ASSERT_FALSE(utf8_2_cp(&cp, NULL, 8));
	CU_ASSERT_FALSE(utf16le_2_cp(&cp, NULL, 8));
}

static void test_zero_length(void)
{
	uint32_t cp = 0;
	unsigned char b[8];
	CU_ASSERT_FALSE(cp_2_utf8(b, 0, 0xE000));
	CU_ASSERT_FALSE(cp_2_utf16le(b, 0, 0xE000));
	CU_ASSERT_FALSE(utf8_2_cp(&cp, b, 0));
	CU_ASSERT_FALSE(utf16le_2_cp(&cp, b, 0));
}	

static void test_utf16_illega_cp(void)
{
	unsigned char b[8];
	CU_ASSERT_FALSE(cp_2_utf16le(b, 8, 0xD800));
	CU_ASSERT_FALSE(cp_2_utf16le(b, 8, 0xDF00));
	CU_ASSERT_FALSE(cp_2_utf16le(b, 8, 0xDFFF));
}

static void test_utf16_edge_cases(void)
{
	unsigned char b[8];
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0x00), 2);
	CU_ASSERT_STRING_EQUAL(b, "\x00\x00"); 
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0xD7FF), 2);
	CU_ASSERT_STRING_EQUAL(b, "\xFF\xD7\x00\x00"); 
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0xE000), 2);
	CU_ASSERT_STRING_EQUAL(b, "\x00\xE0\x00\x00"); 
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0xFFFF), 2);
	CU_ASSERT_STRING_EQUAL(b, "\xFF\xFF\x00\x00"); 
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0x10000), 4);
	CU_ASSERT_STRING_EQUAL(b, "\x00\xD8\x00\xDC\x00\x00"); 
	memset(b, 0, 8);
	CU_ASSERT_EQUAL(cp_2_utf16le(b, 8, 0x10FFFF), 4);
	CU_ASSERT_STRING_EQUAL(b, "\xFF\xDB\xFF\xDF\x00\x00"); 
}

static void utf16_encode_decode(int i) 
{
	unsigned char b[8]; 
	uint32_t cp = 0;

	/* ecnode */
	int n = cp_2_utf16le(b, 8, cp16[i]);	
	if (n == 0) {
		CU_FAIL("encoding returned zero"); 
	} else {
		b[n] = 0;
		CU_ASSERT_STRING_EQUAL(b, expect16[i]); 
	} 
	FILE *f;
	f = fopen("utf16_test.log", "a");
	n = utf16le_2_cp(&cp, b, 4);
	if (n == 0) {
		CU_FAIL("decoding returned zero"); 
	} else {
		fprintf(f, "test [%d] (0x%x):\n\t", i, cp16[i]);
		for(int j = 0; j < 8; j++) {
			fprintf(f, "0x%x ", b[j]);
		}
		fprintf(f, "\n\tcp = 0x%x\n", cp);
		//CU_ASSERT_EQUAL(n, strlen(b));
		CU_ASSERT_EQUAL(cp, cp16[i]);
	}
	fclose(f);
}

static void utf8_encode_decode(int i)
{
	unsigned char b[8]; 
	uint32_t cp = 0;

	/* ecnode */
	int n = cp_2_utf8(b, 8, cp8[i]);	
	if (n == 0) {
		CU_FAIL("encoding returned zero"); 
	} else {
		b[n] = 0;
		CU_ASSERT_STRING_EQUAL(b, expect8[i]); 
	} 

	n = utf8_2_cp(&cp, b, 8);
	if (n == 0) {
		CU_FAIL("decoding returned zero"); 
	} else {
		CU_ASSERT_EQUAL(n, strlen(b));
		CU_ASSERT_EQUAL(cp, cp8[i]);
	}
}

static void test_utf8_ed0(void) { utf8_encode_decode(0); }
static void test_utf8_ed1(void) { utf8_encode_decode(1); }
static void test_utf8_ed2(void) { utf8_encode_decode(2); }
static void test_utf8_ed3(void) { utf8_encode_decode(3); }
static void test_utf16_ed0(void) { utf16_encode_decode(0); }
static void test_utf16_ed1(void) { utf16_encode_decode(1); }
static void test_utf16_ed2(void) { utf16_encode_decode(2); }
static void test_utf16_ed3(void) { utf16_encode_decode(3); }

int reg_utf_suite(void) {
	CU_pSuite suite = NULL;

	/* create suite struct */
	suite = CU_add_suite(SUITE_NAME, setup, teardown);
	if (NULL == suite) 
		return CU_get_error();

	/* register test funct */
	if (NULL == CU_add_test(suite, "null pointers"
				, test_nullpointer))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "zero length buffers"
				, test_zero_length))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf8 encode/decode sample 0"
				, test_utf8_ed0))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf8 encode/decode sample 1"
				, test_utf8_ed1))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf8 encode/decode sample 2"
				, test_utf8_ed2))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf8 encode/decode sample 3"
				, test_utf8_ed3))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 encode/decode sample 0"
				, test_utf16_ed0))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 encode/decode sample 1"
				, test_utf16_ed1))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 encode/decode sample 2"
				, test_utf16_ed2))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 encode/decode sample 3"
				, test_utf16_ed3))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 illegal cp"
				, test_utf16_illega_cp))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf16 edge cases"
				, test_utf16_edge_cases))
		return CU_get_error();
	if (NULL == CU_add_test(suite, "utf8 2 utf16le conversion"
				, test_utf8_2_utf16le))
		return CU_get_error();

	return CUE_SUCCESS;
}
