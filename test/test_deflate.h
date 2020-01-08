
UFBXT_TEST(bits_simple)
#if UFBXT_IMPL
{
	char data[3] = "\xab\xcd\xef";
	ufbxi_bit_stream s;
	ufbxi_bit_init(&s, data, sizeof(data));

	uint64_t bits = ufbxi_bit_read(&s, 0);
	ufbxt_assert(bits == UINT64_C(0x00000000EFCDAB));
	bits = ufbxi_bit_read(&s, 1);
	ufbxt_assert(bits == UINT64_C(0x00000000EFCDAB) >> 1);
}
#endif

UFBXT_TEST(bits_small)
#if UFBXT_IMPL
{
	char data[] = "Hello world!";
	ufbxi_bit_stream s;
	ufbxi_bit_init(&s, data, sizeof(data) - 1);
	uint64_t lo = UINT64_C(0x6f77206f6c6c6548);
	uint64_t hi = UINT64_C(0x0000000021646c72);

	for (uint64_t pos = 0; pos < 256; pos++) {
		uint64_t bits = ufbxi_bit_read(&s, pos);
		uint64_t ref;
		if (pos == 0) ref = lo;
		else if (pos < 64) ref = lo >> pos | hi << (64 - pos);
		else if (pos < 128) ref = hi >> (pos - 64);
		else ref = 0;
		ufbxt_assert(bits == ref);
	}
}
#endif

UFBXT_TEST(bits_long_bytes)
#if UFBXT_IMPL
{
	char data[1024];
	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (char)i;
	}

	for (size_t align = 0; align < 8; align++) {
		ufbxi_bit_stream s;
		ufbxi_bit_init(&s, data + align, sizeof(data) - 8);

		uint64_t bits = 0;
		uint64_t pos = 0;
		for (size_t i = 0; i < sizeof(data) - 8; i++) {
			bits = ufbxi_bit_read(&s, pos);
			ufbxt_assert((uint8_t)(bits & 0xff) == (uint8_t)data[i + align]);
			bits >>= 8;
			pos += 8;
		}

		for (size_t i = 0; i < 128; i++) {
			bits = ufbxi_bit_read(&s, pos);
			ufbxt_assert(bits == 0);
			bits >>= 8;
			pos += 8;
		}
	}
}
#endif

UFBXT_TEST(bits_long_bits)
#if UFBXT_IMPL
{
	char data[1024];
	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (char)i;
	}

	for (size_t align = 0; align < 8; align++) {
		ufbxi_bit_stream s;
		ufbxi_bit_init(&s, data + align, sizeof(data) - 8);

		uint64_t bits = 0;
		uint64_t pos = 0;
		for (size_t i = 0; i < sizeof(data) - 8; i++) {
			for (size_t bit = 0; bit < 8; bit++) {
				bits = ufbxi_bit_read(&s, pos);
				uint8_t byte = (uint8_t)data[i + align];
				ufbxt_assert((bits & 1) == ((byte >> bit) & 1));
				bits >>= 1;
				pos += 1;
			}
		}

		for (size_t i = 0; i < 128; i++) {
			bits = ufbxi_bit_read(&s, pos);
			ufbxt_assert(bits == 0);
			bits >>= 1;
			pos += 1;
		}
	}
}
#endif

UFBXT_TEST(bits_empty)
#if UFBXT_IMPL
{
	ufbxi_bit_stream s;
	ufbxi_bit_init(&s, NULL, 0);

	uint64_t bits = ufbxi_bit_read(&s, 0);
	ufbxt_assert(bits == 0);
}
#endif

#if UFBXT_IMPL
static void
test_huff_range(ufbxi_huff_tree *tree, uint32_t begin, uint32_t end, uint32_t num_bits, uint32_t code_begin)
{
	for (uint32_t i = 0; i <= end - begin; i++) {
		uint64_t code = code_begin + i;
		uint64_t rev_code = 0;
		for (uint32_t bit = 0; bit < num_bits; bit++) {
			if (code & (1 << bit)) rev_code |= 1 << (num_bits - bit - 1);
		}

		uint32_t hi_max = 1 << (12 - num_bits);
		for (uint32_t hi = 0; hi < hi_max; hi++) {
			uint64_t bits = rev_code | (hi << num_bits);
			uint64_t pos = 0;
			uint32_t value = ufbxi_huff_decode_bits(tree, &bits, &pos);
			ufbxt_assert(pos == num_bits);
			ufbxt_assert(bits == hi);
			ufbxt_assert(value == begin + i);
		}
	}
}
#endif

UFBXT_TEST(huff_static_lit_length)
#if UFBXT_IMPL
{
	ufbxi_deflate_context dc;
	ufbxi_init_static_huff(&dc);

	test_huff_range(&dc.huff_lit_length, 0, 143, 8, 0x30);
	test_huff_range(&dc.huff_lit_length, 144, 255, 9, 0x190);
	test_huff_range(&dc.huff_lit_length, 256, 279, 7, 0x0);
	test_huff_range(&dc.huff_lit_length, 280, 287, 8, 0xc0);
}
#endif

UFBXT_TEST(huff_static_dist)
#if UFBXT_IMPL
{
	ufbxi_deflate_context dc;
	ufbxi_init_static_huff(&dc);

	test_huff_range(&dc.huff_dist, 0, 31, 5, 0x0);
}
#endif


UFBXT_TEST(deflate_empty)
#if UFBXT_IMPL
{
	char src[1], dst[1];
	ptrdiff_t res = ufbxi_inflate(dst, 1, src, 0);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res != 0);
}
#endif

UFBXT_TEST(deflate_simple)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello!\x07\xa2\x02\x16";
	char dst[6];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 6);
	ufbxt_assert(!memcmp(dst, "Hello!", 6));
}
#endif

UFBXT_TEST(deflate_simple_chunks)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x00\x06\x00\xf9\xffHello \x01\x06\x00\xf9\xffworld!\x1d\x09\x04\x5e";
	char dst[12];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "Hello world!", 12));
}
#endif

UFBXT_TEST(deflate_static)
#if UFBXT_IMPL
{
	const char src[] = "x\xda\xf3H\xcd\xc9\xc9W(\xcf/\xcaIQ\x04\x00\x1d\t\x04^";
	char dst[12];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "Hello world!", 12));
}
#endif

UFBXT_TEST(deflate_static_match)
#if UFBXT_IMPL
{
	const char src[] = "x\xda\xf3H\xcd\xc9\xc9W\xf0\x00\x91\x8a\x00\x1b\xbb\x04*";
	char dst[12];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "Hello Hello!", 12));
}
#endif

UFBXT_TEST(deflate_static_rle)
#if UFBXT_IMPL
{
	const char src[] = "x\xdastD\x00\x00\x13\xda\x03\r";
	char dst[12];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "AAAAAAAAAAAA", 12));
}
#endif

UFBXT_TEST(deflate_dynamic)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x1d\xc4\x31\x0d\x00\x00\x0c\x02\x41\x2b\xad"
		"\x1b\x8c\xb0\x7d\x82\xff\x8d\x84\xe5\x64\xc8\xcd\x2f\x1b\xbb\x04\x2a";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "Hello Hello!", 12));
}
#endif

UFBXT_TEST(deflate_dynamic_no_match)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x05\x80\x41\x09\x00\x00\x08\x03\xab\x68\x1b\x1b\x58\x40\x7f\x07\x83\xf5"
		"\x7f\x8c\x79\x50\xad\xcc\x75\x00\x1c\x49\x04\x3e";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 12);
	ufbxt_assert(!memcmp(dst, "Hello World!", 12));
}
#endif

UFBXT_TEST(deflate_dynamic_rle)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x5d\xc0\xb1\x00\x00\x00\x00\x80\x30\xb6\xfc\xa5\xfa\xb7\x34\x26\xea\x04"
		"\x52";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 17);
	ufbxt_assert(!memcmp(dst, "AAAAAAAAAAAAAAAAA", 17));
}
#endif

UFBXT_TEST(deflate_repeat_length)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\x00\x05\x0d\x00\x20\x2c\x1b\xee\x0e\xb7"
		"\xfe\x41\x98\xd2\xc6\x3a\x1f\x62\xca\xa5\xb6\x3e\xe6\xda\xe7\x3e\x40"
		"\x62\x11\x26\x84\x77\xcf\x5e\x73\xf4\x56\x4b\x4e\x31\x78\x67\x8d\x56\x1f\xa1\x6e\x0f\xbf";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 52);
	ufbxt_assert(!memcmp(dst, "ABCDEFGHIJKLMNOPQRSTUVWXYZZYXWVUTSRQPONMLKJIHGFEDCBA", 52));
}
#endif

UFBXT_TEST(deflate_huff_lengths)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\xe0\xc1\x95\x65\x59\x96\x65\xd9\xb1\x84"
		"\xca\x70\x53\xf9\xaf\x79\xcf\x5e\x93\x7f\x96\x30\xfe\x7f\xff\xdf\xff"
		"\xfb\xbf\xff\xfd\xf7\xef\xef\xf7\xbd\x5b\xfe\xff\x19\x28\x03\x5d";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 15);
	ufbxt_assert(!memcmp(dst, "0123456789ABCDE", 15));
}
#endif

UFBXT_TEST(deflate_multi_part_matches)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x00\x04\x00\xfb\xff\x54\x65\x73\x74\x52\x08"
		"\x48\x2c\x02\x10\x00\x06\x32\x00\x00\x00\x0c\x52\x39\xcc\x45\x72\xc8"
		"\x7f\xcd\x9d\x00\x08\x00\xf7\xff\x74\x61\x20\x44\x61\x74\x61\x20\x02"
		"\x8b\x01\x38\x8c\x43\x12\x00\x00\x00\x00\x40\xff\x5f\x0b\x36\x8b\xc0"
		"\x12\x80\xf9\xa5\x96\x23\x84\x00\x8e\x36\x10\x41";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == 48);
	ufbxt_assert(!memcmp(dst, "Test Part Data Data Test Data Part New Test Data", 48));
}
#endif

UFBXT_TEST(deflate_uncompressed_bounds)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello!";

	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -8);
}
#endif

UFBXT_TEST(deflate_fail_cfm)
#if UFBXT_IMPL
{
	const char src[] = "\x79\x9c";
	char dst[4];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -1);
}
#endif

UFBXT_TEST(deflate_fail_fdict)
#if UFBXT_IMPL
{
	const char src[] = "\x78\xbc";
	char dst[4];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -2);
}
#endif

UFBXT_TEST(deflate_fail_fcheck)
#if UFBXT_IMPL
{
	const char src[] = "\x78\0x9d";
	char dst[4];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -3);
}
#endif

UFBXT_TEST(deflate_fail_nlen)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf8\xffHello!\x07\xa2\x02\x16";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -4);
}
#endif

UFBXT_TEST(deflate_fail_dst_overflow)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello!\x07\xa2\x02\x16";
	char dst[5];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -6);
}
#endif

UFBXT_TEST(deflate_fail_src_overflow)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -5);
}
#endif

UFBXT_TEST(deflate_fail_bad_block)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x07\x08\x00\xf8\xff";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -7);
}
#endif

UFBXT_TEST(deflate_fail_bad_truncated_checksum)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello!\x07\xa2\x02";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -8);
}
#endif

UFBXT_TEST(deflate_fail_bad_checksum)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x01\x06\x00\xf9\xffHello!\x07\xa2\x02\xff";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -9);
}
#endif

UFBXT_TEST(deflate_fail_codelen_16_overflow)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\x80\x85\x0c\x00\x00\x00\xc0\xfc\xa1\x5f\xc3\x06\x05\xf5\x02\xfb";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -18);
}
#endif

UFBXT_TEST(deflate_fail_codelen_17_overflow)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\xc0\xb1\x0c\x00\x00\x00\x00\x20\x7f\xe7\xae\x26\x00\xfd\x00\xfd";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -19);
}
#endif

UFBXT_TEST(deflate_fail_codelen_18_overflow)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\xc0\x81\x08\x00\x00\x00\x00\x20\x7f\xdf\x09\x4e\x00\xf5\x00\xf5";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -20);
}
#endif

UFBXT_TEST(deflate_fail_codelen_overfull)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\x80\x31\x11\x01\x00\x00\x01\xc3\xa9\xe2\x37\x47\xff\xcd\x69\x26\xf4\x0a\x7a\x02\xbb";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -14);
}
#endif

UFBXT_TEST(deflate_fail_codelen_underfull)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\x80\x31\x11\x00\x00\x00\x41\xc3\xa9\xe2\x37\x47\xff\xcd\x69\x26\xf4\x0a\x7a\x02\xbb";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -15);
}
#endif

UFBXT_TEST(deflate_fail_litlen_bad_huffman)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x05\x40\x81\x09\x00\x20\x08\x7b\xa5\x0f\x7a\xa4\x27\xa2"
		"\x46\x0a\xa2\xa0\xfb\x1f\x11\x23\xea\xf8\x16\xc4\xa7\xae\x9b\x0f\x3d\x4e\xe4\x07\x8d";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -17);
}
#endif

UFBXT_TEST(deflate_fail_distance_bad_huffman)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x1d\xc5\x31\x0d\x00\x00\x0c\x02\x41\x2b\x55\x80\x8a\x9a"
		"\x61\x06\xff\x21\xf9\xe5\xfe\x9d\x1e\x48\x3c\x31\xba\x05\x79";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -23);
}
#endif

UFBXT_TEST(deflate_fail_bad_distance)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x73\xc9\x2c\x2e\x51\x00\x3d\x00\x0f\xd7\x03\x49";
	char dst[64];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -11);
}
#endif

UFBXT_TEST(deflate_fail_literal_overflow)
#if UFBXT_IMPL
{
	const char src[] = "x\xda\xf3H\xcd\xc9\xc9W(\xcf/\xcaIQ\x04\x00\x1d\t\x04^";
	char dst[8];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -10);
}
#endif

UFBXT_TEST(deflate_fail_match_overflow)
#if UFBXT_IMPL
{
	const char src[] = "x\xda\xf3H\xcd\xc9\xc9W\xf0\x00\x91\x8a\x00\x1b\xbb\x04*";
	char dst[8];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -12);
}
#endif

UFBXT_TEST(deflate_fail_bad_distance_bit)
#if UFBXT_IMPL
{
	const char src[] = "\x78\x9c\x0d\xc3\x41\x09\x00\x00\x00\xc2\xc0\x2a\x56\x13"
		"\x6c\x60\x7f\xd8\x1e\xd7\x2f\x06\x0a\x41\x02\x91";
	char dst[8];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -11);
}
#endif

UFBXT_TEST(deflate_fail_bad_distance_empty)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x0d\xc4\x41\x09\x00\x00\x00\xc2\xc0\x2a\x56\x13\x6c\x60\x7f\xd8\x1e\xd0"
		"\x2f\x02\x0a\x41\x02\x91";
	char dst[8];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -11);
}
#endif

UFBXT_TEST(deflate_fail_bad_lit_length)
#if UFBXT_IMPL
{
	char src[] =
		"\x78\x9c\x05\xc0\x81\x08\x00\x00\x00\x00\x20\x7f\xeb\x0b\x00\x00\x00\x01";
	char dst[8];
	ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == -13);
}
#endif

#if UFBXT_IMPL
static uint32_t fnv1a(const void *data, size_t size)
{
	const char *ptr = data, *end = ptr + size;
	uint32_t h = 0x811c9dc5u;
	for (; ptr != end; ptr++) {
		h = (h ^ (uint8_t)*ptr) * 0x01000193;
	}
	return h;
}
#endif

UFBXT_TEST(deflate_bit_flip)
#if UFBXT_IMPL
{
	char src[] = "\x78\x9c\x00\x04\x00\xfb\xff\x54\x65\x73\x74\x52\x08"
		"\x48\x2c\x02\x10\x00\x06\x32\x00\x00\x00\x0c\x52\x39\xcc\x45\x72\xc8"
		"\x7f\xcd\x9d\x00\x08\x00\xf7\xff\x74\x61\x20\x44\x61\x74\x61\x20\x02"
		"\x8b\x01\x38\x8c\x43\x12\x00\x00\x00\x00\x40\xff\x5f\x0b\x36\x8b\xc0"
		"\x12\x80\xf9\xa5\x96\x23\x84\x00\x8e\x36\x10\x41";

	char dst[64];
	int num_res[64] = { 0 };

	for (size_t byte_ix = 0; byte_ix < sizeof(src) - 1; byte_ix++) {
		for (size_t bit_ix = 0; bit_ix < 8; bit_ix++) {
			size_t bit = 1 << bit_ix;

			ufbxt_hintf("byte_ix==%u && bit_ix==%u", (unsigned)byte_ix, (unsigned)bit_ix);

			src[byte_ix] ^= bit;
			ptrdiff_t res = ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
			src[byte_ix] ^= bit;

			res = -res;
			if (res < 0) res = 0;
			if (res > ufbxi_arraycount(num_res)) res = ufbxi_arraycount(num_res);
			num_res[res]++;
		}
	}

	char line[128], *ptr = line, *end = line + sizeof(line);
	for (size_t i = 0; i < ufbxi_arraycount(num_res); i++) {
		if (num_res[i] > 0) {
			ptr += snprintf(ptr, end - ptr, "%3d:%3d    ", -(int)i, num_res[i]);
			if (ptr - line > 70) {
				ufbxt_logf("%s", line);
				ptr = line;
			}
		}
	}
}
#endif

UFBXT_TEST(deflate_static_distances_and_lengths)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x63\x60\x04\x02\x26\x66\x10\x62\x61\x05\x53\x6c\xec\x10\x36\x07\x27\x54"
		"\x80\x8b\x1b\x26\xc1\xc3\x0b\x57\xc0\xc7\x8f\x50\x27\x20\x88\xa4\x43\x48\x18\x59"
		"\x87\x88\x28\x8a\x51\x62\xe2\xa8\x46\x49\x48\xa2\x59\x20\x25\x8d\x6e\x9b\x8c\x2c"
		"\x86\x03\xe4\xe4\x31\x1d\xa0\xa0\x88\xc5\x7d\x4a\xca\xd8\xdc\xa7\xa2\x8a\xd5\x03"
		"\x6a\xea\xd8\x3d\xa0\xa1\x89\xc3\x97\x5a\xda\xb8\x02\x40\x47\x17\x67\x00\xe8\xe9"
		"\xe3\x0e\x00\x03\x43\x3c\x21\x69\x64\x8c\x2f\x24\x4d\x4c\xf1\xc6\x87\x99\x39\xfe"
		"\xf8\xb0\xb0\x24\x10\x1f\x56\xd6\x84\xe2\xc3\xc6\x96\x60\x02\xb0\xb3\x27\x9c\x00"
		"\x1c\x1c\x89\x48\x00\x4e\xce\x44\xa6\x2b\x17\x57\xe2\xd3\x95\x9b\x3b\x09\xe9\xca"
		"\xc3\x93\xd4\x74\xe5\xe5\x4d\x7a\x06\xf0\xf1\x25\x23\x03\xf8\xf9\x93\x99\x07\x03"
		"\x02\xc9\xcf\x83\x41\xc1\x14\xe4\xc1\x90\x50\x0a\x0b\x80\xb0\x70\xca\x0b\x80\x88"
		"\x48\x2a\x14\x00\x51\xd1\xd4\x29\xe1\x62\x62\xa9\x5f\xc2\xc5\xc5\xd3\xa0\x84\x4b"
		"\x48\xa4\x5d\x09\x97\x94\x4c\xfb\x0a\x20\x25\x95\x0e\x15\x40\x5a\x3a\xdd\x2a\x80"
		"\x8c\x4c\xfa\xd7\x83\x59\xd9\x03\x50\x0f\xe6\xe4\x0e\x54\x03\x20\x2f\x7f\xe0\x1b"
		"\x00\x05\x85\x83\xa0\x01\x50\x54\x3c\x98\xda\x70\x25\xa5\x83\xb3\x0d\x57\x56\x3e"
		"\xf8\xdb\x70\x15\x95\x43\xa0\x0d\x57\x55\x3d\x74\x3a\x00\x35\xb5\x43\xb1\x03\x50"
		"\x57\x3f\xf4\x3b\x00\x0d\x8d\xc3\xa0\xaf\xd6\xd4\x3c\x5c\xfa\x6a\x2d\xad\xc3\xaf"
		"\xaf\xd6\xd6\x3e\xfc\x07\x00\x3a\x3a\x47\xc0\x00\x40\x57\xf7\xc8\x18\x00\xe8\xe9"
		"\x1d\x69\x03\x00\x7d\xfd\x23\x77\x1c\x6e\xc2\xc4\x11\x3f\x0e\x37\x69\xf2\xe8\x38"
		"\xdc\x94\xa9\xa3\xe3\x70\xd3\xa6\x8f\x4e\x00\xcc\x98\x39\x3a\x01\x30\x6b\xf6\xe8"
		"\x04\xc0\x9c\xb9\xa3\x13\x00\xf3\xe6\x8f\xce\xab\x2d\x58\x38\x3a\xaf\xb6\x68\xf1"
		"\xe8\xbc\xda\x92\xa5\xa3\xf3\x6a\xcb\x96\x8f\x2e\x00\x58\xb1\x72\x74\x01\xc0\xaa"
		"\xd5\xa3\x0b\x00\xd6\xac\x1d\x5d\x00\xb0\x6e\xfd\xe8\x02\x80\x0d\x1b\x47\x17\x00"
		"\x6c\xda\x3c\xba\x00\x60\xcb\xd6\xd1\xf5\x70\xdb\xb6\x8f\xae\x87\xdb\xb1\x73\x74"
		"\x3d\xdc\xae\xdd\xa3\xeb\xe1\xf6\xec\x1d\x5d\x0f\xb7\x6f\xff\xe8\x7a\xb8\x03\x07"
		"\x47\xd7\xc3\x1d\x3a\x3c\xba\x1e\xee\xc8\xd1\xd1\xf5\x70\xc7\x8e\x8f\x6e\x00\x38"
		"\x71\x72\x74\x03\xc0\xa9\xd3\xa3\x1b\x00\xce\x9c\x1d\xdd\x00\x70\xee\xfc\xe8\x06"
		"\x80\x0b\x17\x47\x37\x00\x5c\xba\x3c\xba\x01\xe0\xca\xd5\xd1\xfd\x6a\xd7\xae\x8f"
		"\xee\x57\xbb\x71\x73\x74\xbf\xda\xad\xdb\xa3\xfb\xd5\xee\xdc\x1d\xdd\xaf\x76\xef"
		"\xfe\xe8\x7e\xb5\x07\x0f\x47\xf7\xab\x3d\x7a\x3c\xba\x5f\xed\xc9\xd3\xd1\xfd\x6a"
		"\xcf\x9e\x8f\x1e\x00\xf0\xe2\xe5\xe8\x01\x00\xaf\x5e\x8f\x1e\x00\xf0\xe6\xed\xe8"
		"\x01\x00\xef\xde\x8f\x1e\x00\xf0\xe1\xe3\xe8\x01\x00\x9f\x3e\x8f\x1e\x00\xf0\xe5"
		"\xeb\xe8\x01\x00\xdf\xbe\x8f\x1e\x00\xf0\xe3\xe7\xe8\x01\x00\xbf\x7e\x8f\x1e\x00"
		"\xf0\xe7\xef\xe8\x01\x00\xff\xfe\x8f\x1e\x00\xc0\xc0\x38\x7a\x00\x00\x13\xf3\xe8"
		"\xf9\x70\x2c\xac\xa3\xe7\xc3\xb1\xb1\x8f\x9e\x0f\xc7\xc1\x39\x7a\x3e\x1c\x17\xf7"
		"\xe8\xf9\x70\x3c\xbc\xa3\xe7\xc3\xf1\xf1\x8f\x9e\x0f\x27\x20\x38\x7a\x3e\x9c\x90"
		"\xf0\xe8\xf9\x70\x22\xa2\xa3\xe7\xc3\x89\x89\x8f\x9e\x0f\x27\x21\x39\x7a\x3e\x9c"
		"\x94\xf4\xe8\xf9\x70\x32\xb2\xa3\xe7\xc3\xc9\xc9\x8f\x9e\x0f\xa7\xa0\x38\x7a\x3e"
		"\x9c\x92\xf2\xe8\xf9\x70\x2a\xaa\xa3\xe7\xc3\xa9\xa9\x8f\x5e\x00\xa0\xa1\x39\x7a"
		"\x4f\x83\x96\xf6\xe8\x3d\x0d\x3a\xba\xa3\xf7\x34\xe8\xe9\x8f\xde\xd3\x60\x60\x38"
		"\x7a\x1f\x8b\x91\xf1\xe8\x7d\x2c\x26\xa6\xa3\xf7\xb1\x98\x99\x8f\xde\xc7\x62\x61"
		"\x39\x7a\xef\x92\x95\xf5\xe8\xbd\x4b\x36\xb6\xa3\xf7\x2e\xd9\xd9\x8f\xde\xbb\xe4"
		"\xe0\x38\x7a\xbf\x9a\x93\xf3\xe8\xfd\x6a\x2e\xae\xa3\xf7\xab\xb9\xb9\x8f\xde\xaf"
		"\xe6\xe1\x39\x7a\x8f\xa2\x97\xf7\xe8\x3d\x8a\x3e\xbe\xa3\xf7\x28\xfa\xf9\x8f\xde"
		"\xa3\x18\x10\x38\x7a\x5f\x6a\x50\xf0\xe8\x7d\xa9\x21\xa1\xa3\xf7\xa5\x86\x85\x8f"
		"\xde\x97\x1a\x11\x39\x7a\x2f\x72\x54\xf4\xe8\xbd\xc8\x31\xb1\xa3\xf7\x22\xc7\xc5"
		"\x8f\xde\x8b\x9c\x90\x38\x7a\x2f\x72\x52\xf2\xe8\xbd\xc8\x29\xa9\xa3\xf7\xff\xa7"
		"\xa5\x8f\xde\xff\x0f\x00\x5e\x3b\xcf\x7c";

	size_t dst_size = 33665;
	char *dst = malloc(dst_size);
	ptrdiff_t res = ufbxi_inflate(dst, dst_size, src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == dst_size);
	ufbxt_assert(fnv1a(dst, dst_size) == 0x88398917);
	free(dst);
}
#endif

UFBXT_TEST(deflate_dynamic_distances_and_lengths)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\xed\x9d\x03\x70\x34\x41\x14\x84\x63\xdb\xb6\x6d\xdb\xb6\xed\xfc\xb6\x6d"
		"\xdb\xb6\x6d\xdb\xb6\x6d\xdb\x36\x82\xb3\xb1\x9d\x4a\x55\x52\xc9\xdd\xed\xee\x78"
		"\xe6\xbd\xee\x4f\x44\xf4\xe7\x97\x98\xf8\xaf\x6f\x09\xc9\xdf\x3f\xa4\xa4\xff\xfc"
		"\x2e\x23\xfb\xf7\x0f\x72\xf2\xff\xfe\xa1\xa0\xf8\xff\x05\x4a\xca\x35\xaf\x53\x51"
		"\xad\xf5\x0e\x35\xf5\xda\xef\xd0\xd0\xac\xf3\x51\x5a\xda\x75\x3f\x4a\x47\xb7\xde"
		"\x05\xf4\xf4\xeb\x5f\xcd\xc0\x90\xe4\x06\x8c\x8c\x49\x6f\xc0\xc4\x94\xcc\xfd\x99"
		"\x99\x93\xbb\x3f\x0b\x4b\xb2\x0f\x60\x65\x4d\xfe\x01\x6c\x6c\x29\x3c\xa5\x9d\x3d"
		"\xa5\x02\x70\x70\xa4\x58\x00\x4e\xce\x94\x0b\xc0\xc5\x95\x4a\x49\xba\xb9\x53\x2b"
		"\x49\x0f\x4f\xaa\xf5\xe1\xe5\x4d\xbd\x3e\x7c\x7c\x69\xd4\x87\x9f\x3f\xad\xfa\x08"
		"\x08\xa4\xd9\x00\x82\x82\x69\x37\x80\x90\x50\x3a\x1a\x40\x58\x38\x9d\xed\x2a\x22"
		"\x92\xfe\x76\x15\x15\xcd\x40\xbb\x8a\x89\x65\xb4\x5d\xc5\xc5\x33\xde\x01\x12\x12"
		"\x99\xe8\x00\x49\xc9\x4c\xf6\xc1\x94\x54\xe6\xfb\x60\x5a\x3a\x0b\x7d\x30\x23\x93"
		"\xc5\x01\x20\x2b\x9b\xf5\x01\x20\x27\x97\x0d\x03\x40\x5e\x3e\x7b\x46\xb8\x82\x42"
		"\xf6\x8f\x70\x45\xc5\x1c\x18\xe1\x4a\x4a\x39\x37\xc2\x95\x95\x73\x7e\x02\xa8\xa8"
		"\xe4\xc2\x04\x50\x55\xcd\xb5\x09\xa0\x41\x43\xee\xcf\x83\x8d\x1a\xf3\x60\x1e\x6c"
		"\xd2\x94\x57\x0b\x80\x66\xcd\x79\xbf\x00\x68\xd1\x92\x0f\x16\x00\xad\x5a\xf3\xd3"
		"\x1a\xae\x4d\x5b\xfe\x5c\xc3\xb5\x6b\xcf\xff\x6b\xb8\x0e\x1d\x05\x60\x0d\xd7\xa9"
		"\xb3\xe0\x6c\x00\xba\x74\x15\xc4\x0d\x40\xb7\xee\x82\xbf\x01\xe8\xd1\x53\x08\xf6"
		"\x6a\xbd\x7a\x0b\xcb\x5e\xad\x4f\x5f\xe1\xdb\xab\xf5\xeb\x2f\xfc\x07\x00\x03\x06"
		"\x12\xe0\x00\x60\xd0\x60\x62\x1c\x00\x0c\x19\x4a\xb4\x03\x80\x61\xc3\x89\x7b\x0e"
		"\x37\x62\x24\xe1\xcf\xe1\x46\x8d\xc6\x39\xdc\x98\xb1\x38\x87\x1b\x37\x1e\x01\x80"
		"\x09\x13\x11\x00\x98\x34\x19\x01\x80\x29\x53\x11\x00\x98\x36\x1d\x71\xb5\x19\x33"
		"\x11\x57\x9b\x35\x1b\x71\xb5\x39\x73\x11\x57\x9b\x37\x1f\x09\x00\x0b\x16\x22\x01"
		"\x60\xd1\x62\x24\x00\x2c\x59\x8a\x04\x80\x65\xcb\x91\x00\xb0\x62\x25\x12\x00\x56"
		"\xad\x46\x02\xc0\x9a\xb5\xc8\x87\x5b\xb7\x1e\xf9\x70\x1b\x36\x22\x1f\x6e\xd3\x66"
		"\xe4\xc3\x6d\xd9\x8a\x7c\xb8\x6d\xdb\x91\x0f\xb7\x63\x27\xf2\xe1\x76\xed\x46\x3e"
		"\xdc\x9e\xbd\xc8\x87\xdb\xb7\x1f\x02\x80\x03\x07\x21\x00\x38\x74\x18\x02\x80\x23"
		"\x47\x21\x00\x38\x76\x1c\x02\x80\x13\x27\x21\x00\x38\x75\x1a\x02\x80\x33\x67\xa1"
		"\x57\x3b\x77\x1e\x7a\xb5\x0b\x17\xa1\x57\xbb\x74\x19\x7a\xb5\x2b\x57\xa1\x57\xbb"
		"\x76\x1d\x7a\xb5\x1b\x37\xa1\x57\xbb\x75\x1b\x7a\xb5\x3b\x77\xa1\x57\xbb\x77\x1f"
		"\x06\x00\x0f\x1e\xc2\x00\xe0\xd1\x63\x18\x00\x3c\x79\x0a\x03\x80\x67\xcf\x61\x00"
		"\xf0\xe2\x25\x0c\x00\x5e\xbd\x86\x01\xc0\x9b\xb7\x30\x00\x78\xf7\x1e\x06\x00\x1f"
		"\x3e\xc2\x00\xe0\xd3\x67\x18\x00\x7c\xf9\x0a\x03\x80\x6f\xdf\x61\x00\x20\x22\x0a"
		"\x03\x00\x31\x71\xf8\xc3\x49\x48\xc2\x1f\x4e\x4a\x1a\xfe\x70\x32\xb2\xf0\x87\x93"
		"\x93\x87\x3f\x9c\x82\x22\xfc\xe1\x94\x94\xe1\x0f\xa7\xa2\x0a\x7f\x38\x35\x75\xf8"
		"\xc3\x69\x68\xc2\x1f\x4e\x4b\x1b\xfe\x70\x3a\xba\xf0\x87\xd3\xd3\x87\x3f\x9c\x81"
		"\x21\xfc\xe1\x8c\x8c\xe1\x0f\x67\x62\x0a\x7f\x38\x33\x73\xf8\xc3\x59\x58\xc2\x1f"
		"\xce\xca\x1a\x00\x00\x1b\x5b\x70\x1a\xec\xec\xc1\x69\x70\x70\x04\xa7\xc1\xc9\x19"
		"\x9c\x06\x17\x57\xf0\x58\xdc\xdc\xc1\x63\xf1\xf0\x04\x8f\xc5\xcb\x1b\x3c\x16\x1f"
		"\x5f\x70\x97\xfc\xfc\xc1\x5d\x0a\x08\x04\x77\x29\x28\x18\xdc\xa5\x90\x50\xf0\xd5"
		"\xc2\xc2\xc1\x57\x8b\x88\x04\x5f\x2d\x2a\x1a\x7c\xb5\x98\x58\x70\x14\xe3\xe2\xc1"
		"\x51\x4c\x48\x04\x47\x31\x29\x19\x1c\xc5\x94\x54\xf0\x52\xd3\xd2\xc1\x4b\xcd\xc8"
		"\x04\x2f\x35\x2b\x1b\xbc\xd4\x9c\x5c\x70\x91\xf3\xf2\xc1\x45\x2e\x28\x04\x17\xb9"
		"\xa8\x18\x5c\xe4\x92\x52\x70\x91\xcb\xca\xc1\x45\xae\xa8\x04\xff\xbf\xaa\x1a\xfc"
		"\xff\x1f\x5e\x3b\xcf\x7c";

	size_t dst_size = 33665;
	char *dst = malloc(dst_size);
	ptrdiff_t res = ufbxi_inflate(dst, dst_size, src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == dst_size);
	ufbxt_assert(fnv1a(dst, dst_size) == 0x88398917);
	free(dst);
}
#endif

UFBXT_TEST(deflate_long_codes)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\xed\xfd\xc7\xb9\x65\x5d\xb6\x65\xd9\xc9\x06\x40\x85\xae\xc2\x90\x60\x2e"
		"\xfd\x3f\x14\xf6\xb9\xe6\xfe\x32\xc1\x39\x69\x85\x56\xeb\x63\xae\x7d\xae\xfd\x1e"
		"\x01\x8e\xb7\x7b\x00\x00\x00\x00\x00\x00\x00\x00\x00\xc0\xff\x27\xfd\x7f\x1e\xee"
		"\xff\xa3\xfe\x3f\x0f\xf7\xbf\xf9\xff\xac\xff\xcf\xc3\xfd\x6f\xfe\xb7\xff\x1f\xf6"
		"\xff\x79\xb8\xff\xcd\xff\xf6\x7f\xf7\xff\x69\xff\x9f\x87\x03\x00\x00\xe0\xff\x1b"
		"\xff\xbf\xaf\xf6\xff\x95\xff\xdf\x57\xfb\xdf\xfc\x7f\xe7\xff\xf7\xd5\xfe\x37\xff"
		"\xdb\xff\x2f\xfd\xff\xbe\xda\xff\xe6\x7f\xfb\xbf\xfb\xff\xd6\xff\xef\xab\x01\x00"
		"\x00\x00\x00\xfc\xff\xde\xff\xef\xc3\xfd\xff\xe0\xff\xef\xc3\xfd\x6f\xfe\x7f\xf1"
		"\xff\xf7\xe1\xfe\x37\xff\xdb\xff\x9f\xfc\xff\x7d\xb8\xff\xcd\xff\xf6\x7f\xf7\xff"
		"\x9b\xff\xbf\x0f\x07\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xf1\xff\x7f\xa9\xff"
		"\x7f\xf2\xff\x7f\xa9\xff\x9b\xff\x7f\xf9\xff\xbf\xd4\xff\xcd\xff\xf6\xff\x6f\xfe"
		"\xff\x2f\xf5\x7f\xf3\xbf\xfd\xdf\xfd\xff\xcf\xff\xff\xa5\xfe\xef\x01\x7e\xa8\x57"
		"\xe0";

	size_t dst_size = 31216;
	char *dst = malloc(dst_size);
	ptrdiff_t res = ufbxi_inflate(dst, dst_size, src, sizeof(src) - 1);
	ufbxt_hintf("res = %d", (int)res);
	ufbxt_assert(res == dst_size);
	ufbxt_assert(fnv1a(dst, dst_size) == 0x9e9ed1e5);
	free(dst);
}
#endif

UFBXT_TEST(deflate_fuzz_1)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x30\x04\x00\xfb\xff\x30\x30\x30\x30\x52\x30\x30\x30\x02\x10\x00\x06\x32"
		"\x00\x00\x00\x0c\x52\x39\xcc\x45\x72\xc8\x7f\xcd\x9d\x30\x08\x00\xf7\xff\x30\x30"
		"\x30\x30\x30\x30\x30\x30\x02\x8b\x01\x38\x8c\x43\x12\x00\x00\x00\x00\x40\xff\x5f"
		"\x0b";

	char dst[4096];
	ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
}
#endif

UFBXT_TEST(deflate_fuzz_2)
#if UFBXT_IMPL
{
	const char src[] =
		"\x78\x9c\x00\x04\x00\xfb\xff\x54\x65\x73\x74\x52\x08\x48\x2c\x02\x10\x00\x06\x32"
		"\x00\x00\x00\x0c\x52\x39\xcc\x45\x72\xc8\x7f\xcd\x9d\x00\x08\x00\xf7\xff\x74\x61"
		"\x20\x44\x61\x74\x61\x20\x02\x8b\x01\x38\x8c\x43\x12\x00\x00\x00\x00\x40\xff\x5f"
		"\x0b\x36\x8b\xc0\x12\x80\xf9\xa5\x92\x23\x84\x00\x8e\x36\x10\x41";

	char dst[4096];
	ufbxi_inflate(dst, sizeof(dst), src, sizeof(src) - 1);
}
#endif
