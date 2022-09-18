#include "cute_aseprite.h"
#define CUTE_ASEPRITE_IMPLEMENTATION

#ifdef CUTE_ASEPRITE_IMPLEMENTATION
#ifndef CUTE_ASEPRITE_IMPLEMENTATION_ONCE
#define CUTE_ASEPRITE_IMPLEMENTATION_ONCE

#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
	#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if !defined(CUTE_ASEPRITE_ALLOC)
	#include <stdlib.h>
	#define CUTE_ASEPRITE_ALLOC(size, ctx) malloc(size)
	#define CUTE_ASEPRITE_FREE(mem, ctx) free(mem)
#endif

#if !defined(CUTE_ASEPRITE_UNUSED)
	#if defined(_MSC_VER)
		#define CUTE_ASEPRITE_UNUSED(x) (void)x
	#else
		#define CUTE_ASEPRITE_UNUSED(x) (void)(sizeof(x))
	#endif
#endif

#if !defined(CUTE_ASEPRITE_MEMCPY)
	#include <string.h> // memcpy
	#define CUTE_ASEPRITE_MEMCPY memcpy
#endif

#if !defined(CUTE_ASEPRITE_MEMSET)
	#include <string.h> // memset
	#define CUTE_ASEPRITE_MEMSET memset
#endif

#if !defined(CUTE_ASEPRITE_ASSERT)
	#include <assert.h>
	#define CUTE_ASEPRITE_ASSERT assert
#endif

#if !defined(CUTE_ASEPRITE_STDIO)
	#include <stdio.h> // fopen
	#define CUTE_ASEPRITE_STDIO
	#define CUTE_ASEPRITE_SEEK_SET SEEK_SET
	#define CUTE_ASEPRITE_SEEK_END SEEK_END
	#define CUTE_ASEPRITE_FILE FILE
	#define CUTE_ASEPRITE_FOPEN fopen
	#define CUTE_ASEPRITE_FSEEK fseek
	#define CUTE_ASEPRITE_FREAD fread
	#define CUTE_ASEPRITE_FTELL ftell
	#define CUTE_ASEPRITE_FCLOSE fclose
#endif

static const char* s_error_file = NULL; // The filepath of the file being parsed. NULL if from memory.
static const char* s_error_reason;      // Used to capture errors during DEFLATE parsing.

#if !defined(CUTE_ASEPRITE_WARNING)
	#define CUTE_ASEPRITE_WARNING(msg) cute_aseprite_warning(msg, __LINE__)

    static int s_error_cline;               // The line in cute_aseprite.h where the error was triggered.
	void cute_aseprite_warning(const char* warning, int line)
	{
		s_error_cline = line;
		const char *error_file = s_error_file ? s_error_file : "MEMORY";
		printf("WARNING (cute_aseprite.h:%i): %s (%s)\n", s_error_cline, warning, error_file);
	}
#endif

#define CUTE_ASEPRITE_FAIL() do { goto ase_err; } while (0)
#define CUTE_ASEPRITE_CHECK(X, Y) do { if (!(X)) { s_error_reason = Y; CUTE_ASEPRITE_FAIL(); } } while (0)
#define CUTE_ASEPRITE_CALL(X) do { if (!(X)) goto ase_err; } while (0)
#define CUTE_ASEPRITE_DEFLATE_MAX_BITLEN 15

// DEFLATE tables from RFC 1951
static uint8_t s_fixed_table[288 + 32] = {
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
}; // 3.2.6
static uint8_t s_permutation_order[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 }; // 3.2.7
static uint8_t s_len_extra_bits[29 + 2] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,  0,0 }; // 3.2.5
static uint32_t s_len_base[29 + 2] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,  0,0 }; // 3.2.5
static uint8_t s_dist_extra_bits[30 + 2] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,  0,0 }; // 3.2.5
static uint32_t s_dist_base[30 + 2] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577, 0,0 }; // 3.2.5

typedef struct deflate_t
{
	uint64_t bits;
	int count;
	uint32_t* words;
	int word_count;
	int word_index;
	int bits_left;

	int final_word_available;
	uint32_t final_word;

	char* out;
	char* out_end;
	char* begin;

	uint32_t lit[288];
	uint32_t dst[32];
	uint32_t len[19];
	uint32_t nlit;
	uint32_t ndst;
	uint32_t nlen;
} deflate_t;

static int s_would_overflow(deflate_t* s, int num_bits)
{
	return (s->bits_left + s->count) - num_bits < 0;
}

static char* s_ptr(deflate_t* s)
{
	CUTE_ASEPRITE_ASSERT(!(s->bits_left & 7));
	return (char*)(s->words + s->word_index) - (s->count / 8);
}

static uint64_t s_peak_bits(deflate_t* s, int num_bits_to_read)
{
	if (s->count < num_bits_to_read)
	{
		if (s->word_index < s->word_count)
		{
			uint32_t word = s->words[s->word_index++];
			s->bits |= (uint64_t)word << s->count;
			s->count += 32;
			CUTE_ASEPRITE_ASSERT(s->word_index <= s->word_count);
		}

		else if (s->final_word_available)
		{
			uint32_t word = s->final_word;
			s->bits |= (uint64_t)word << s->count;
			s->count += s->bits_left;
			s->final_word_available = 0;
		}
	}

	return s->bits;
}

static uint32_t s_consume_bits(deflate_t* s, int num_bits_to_read)
{
	CUTE_ASEPRITE_ASSERT(s->count >= num_bits_to_read);
	uint32_t bits = (uint32_t)(s->bits & (((uint64_t)1 << num_bits_to_read) - 1));
	s->bits >>= num_bits_to_read;
	s->count -= num_bits_to_read;
	s->bits_left -= num_bits_to_read;
	return bits;
}

static uint32_t s_read_bits(deflate_t* s, int num_bits_to_read)
{
	CUTE_ASEPRITE_ASSERT(num_bits_to_read <= 32);
	CUTE_ASEPRITE_ASSERT(num_bits_to_read >= 0);
	CUTE_ASEPRITE_ASSERT(s->bits_left > 0);
	CUTE_ASEPRITE_ASSERT(s->count <= 64);
	CUTE_ASEPRITE_ASSERT(!s_would_overflow(s, num_bits_to_read));
	s_peak_bits(s, num_bits_to_read);
	uint32_t bits = s_consume_bits(s, num_bits_to_read);
	return bits;
}

static uint32_t s_rev16(uint32_t a)
{
	a = ((a & 0xAAAA) >>  1) | ((a & 0x5555) << 1);
	a = ((a & 0xCCCC) >>  2) | ((a & 0x3333) << 2);
	a = ((a & 0xF0F0) >>  4) | ((a & 0x0F0F) << 4);
	a = ((a & 0xFF00) >>  8) | ((a & 0x00FF) << 8);
	return a;
}

// RFC 1951 section 3.2.2
static uint32_t s_build(deflate_t* s, uint32_t* tree, uint8_t* lens, int sym_count)
{
	int n, codes[16], first[16], counts[16] = { 0 };
    CUTE_ASEPRITE_UNUSED(s);

	// Frequency count
	for (n = 0; n < sym_count; n++) counts[lens[n]]++;

	// Distribute codes
	counts[0] = codes[0] = first[0] = 0;
	for (n = 1; n <= 15; ++n)
	{
		codes[n] = (codes[n - 1] + counts[n - 1]) << 1;
		first[n] = first[n - 1] + counts[n - 1];
	}

	for (int i = 0; i < sym_count; ++i)
	{
		uint8_t len = lens[i];

		if (len != 0)
		{
			CUTE_ASEPRITE_ASSERT(len < 16);
			uint32_t code = (uint32_t)codes[len]++;
			uint32_t slot = (uint32_t)first[len]++;
			tree[slot] = (code << (32 - (uint32_t)len)) | (i << 4) | len;
		}
	}

	return (uint32_t)first[15];
}

static int s_stored(deflate_t* s)
{
	char* p;

	// 3.2.3
	// skip any remaining bits in current partially processed byte
	s_read_bits(s, s->count & 7);

	// 3.2.4
	// read LEN and NLEN, should complement each other
	uint16_t LEN = (uint16_t)s_read_bits(s, 16);
	uint16_t NLEN = (uint16_t)s_read_bits(s, 16);
	CUTE_ASEPRITE_CHECK(LEN == (uint16_t)(~NLEN), "Failed to find LEN and NLEN as complements within stored (uncompressed) stream.");
	CUTE_ASEPRITE_CHECK(s->bits_left / 8 <= (int)LEN, "Stored block extends beyond end of input stream.");
	p = s_ptr(s);
	CUTE_ASEPRITE_MEMCPY(s->out, p, LEN);
	s->out += LEN;
	return 1;

ase_err:
	return 0;
}

// 3.2.6
static int s_fixed(deflate_t* s)
{
	s->nlit = s_build(s, s->lit, s_fixed_table, 288);
	s->ndst = s_build(0, s->dst, s_fixed_table + 288, 32);
	return 1;
}

static int s_decode(deflate_t* s, uint32_t* tree, int hi)
{
	uint64_t bits = s_peak_bits(s, 16);
	uint32_t search = (s_rev16((uint32_t)bits) << 16) | 0xFFFF;
	int lo = 0;
	while (lo < hi)
	{
		int guess = (lo + hi) >> 1;
		if (search < tree[guess]) hi = guess;
		else lo = guess + 1;
	}

	uint32_t key = tree[lo - 1];
	uint32_t len = (32 - (key & 0xF));
	CUTE_ASEPRITE_ASSERT((search >> len) == (key >> len));

	s_consume_bits(s, key & 0xF);
	return (key >> 4) & 0xFFF;
}

// 3.2.7
static int s_dynamic(deflate_t* s)
{
	uint8_t lenlens[19] = { 0 };

	uint32_t nlit = 257 + s_read_bits(s, 5);
	uint32_t ndst = 1 + s_read_bits(s, 5);
	uint32_t nlen = 4 + s_read_bits(s, 4);

	for (uint32_t i = 0 ; i < nlen; ++i)
		lenlens[s_permutation_order[i]] = (uint8_t)s_read_bits(s, 3);

	// Build the tree for decoding code lengths
	s->nlen = s_build(0, s->len, lenlens, 19);
	uint8_t lens[288 + 32];

	for (uint32_t n = 0; n < nlit + ndst;)
	{
		int sym = s_decode(s, s->len, (int)s->nlen);
		switch (sym)
		{
		case 16: for (uint32_t i =  3 + s_read_bits(s, 2); i; --i, ++n) lens[n] = lens[n - 1]; break;
		case 17: for (uint32_t i =  3 + s_read_bits(s, 3); i; --i, ++n) lens[n] = 0; break;
		case 18: for (uint32_t i = 11 + s_read_bits(s, 7); i; --i, ++n) lens[n] = 0; break;
		default: lens[n++] = (uint8_t)sym; break;
		}
	}

	s->nlit = s_build(s, s->lit, lens, (int)nlit);
	s->ndst = s_build(0, s->dst, lens + nlit, (int)ndst);
	return 1;
}

// 3.2.3
static int s_block(deflate_t* s)
{
	while (1)
	{
		int symbol = s_decode(s, s->lit, (int)s->nlit);

		if (symbol < 256)
		{
			CUTE_ASEPRITE_CHECK(s->out + 1 <= s->out_end, "Attempted to overwrite out buffer while outputting a symbol.");
			*s->out = (char)symbol;
			s->out += 1;
		}

		else if (symbol > 256)
		{
			symbol -= 257;
			uint32_t length = s_read_bits(s, (int)(s_len_extra_bits[symbol])) + s_len_base[symbol];
			int distance_symbol = s_decode(s, s->dst, (int)s->ndst);
			uint32_t backwards_distance = s_read_bits(s, s_dist_extra_bits[distance_symbol]) + s_dist_base[distance_symbol];
			CUTE_ASEPRITE_CHECK(s->out - backwards_distance >= s->begin, "Attempted to write before out buffer (invalid backwards distance).");
			CUTE_ASEPRITE_CHECK(s->out + length <= s->out_end, "Attempted to overwrite out buffer while outputting a string.");
			char* src = s->out - backwards_distance;
			char* dst = s->out;
			s->out += length;

			switch (backwards_distance)
			{
			case 1: // very common in images
				CUTE_ASEPRITE_MEMSET(dst, *src, (size_t)length);
				break;
			default: while (length--) *dst++ = *src++;
			}
		}

		else break;
	}

	return 1;

ase_err:
	return 0;
}

// 3.2.3
static int s_inflate(const void* in, int in_bytes, void* out, int out_bytes, void* mem_ctx)
{
	CUTE_ASEPRITE_UNUSED(mem_ctx);
	deflate_t* s = (deflate_t*)CUTE_ASEPRITE_ALLOC(sizeof(deflate_t), mem_ctx);
	s->bits = 0;
	s->count = 0;
	s->word_index = 0;
	s->bits_left = in_bytes * 8;

	// s->words is the in-pointer rounded up to a multiple of 4
	int first_bytes = (int)((((size_t)in + 3) & ~3) - (size_t)in);
	s->words = (uint32_t*)((char*)in + first_bytes);
	s->word_count = (in_bytes - first_bytes) / 4;
	int last_bytes = ((in_bytes - first_bytes) & 3);

	for (int i = 0; i < first_bytes; ++i)
		s->bits |= (uint64_t)(((uint8_t*)in)[i]) << (i * 8);

	s->final_word_available = last_bytes ? 1 : 0;
	s->final_word = 0;
	for(int i = 0; i < last_bytes; i++)
		s->final_word |= ((uint8_t*)in)[in_bytes - last_bytes + i] << (i * 8);

	s->count = first_bytes * 8;

	s->out = (char*)out;
	s->out_end = s->out + out_bytes;
	s->begin = (char*)out;

	int count = 0;
	uint32_t bfinal;
	do
	{
		bfinal = s_read_bits(s, 1);
		uint32_t btype = s_read_bits(s, 2);

		switch (btype)
		{
		case 0: CUTE_ASEPRITE_CALL(s_stored(s)); break;
		case 1: s_fixed(s); CUTE_ASEPRITE_CALL(s_block(s)); break;
		case 2: s_dynamic(s); CUTE_ASEPRITE_CALL(s_block(s)); break;
		case 3: CUTE_ASEPRITE_CHECK(0, "Detected unknown block type within input stream.");
		}

		++count;
	}
	while (!bfinal);

	CUTE_ASEPRITE_FREE(s, mem_ctx);
	return 1;

ase_err:
	CUTE_ASEPRITE_FREE(s, mem_ctx);
	return 0;
}

typedef struct ase_state_t
{
	uint8_t* in;
	uint8_t* end;
	void* mem_ctx;
} ase_state_t;

static uint8_t s_read_uint8(ase_state_t* s)
{
	CUTE_ASEPRITE_ASSERT(s->in <= s->end + sizeof(uint8_t));
	uint8_t** p = &s->in;
	uint8_t value = **p;
	++(*p);
	return value;
}

static uint16_t s_read_uint16(ase_state_t* s)
{
	CUTE_ASEPRITE_ASSERT(s->in <= s->end + sizeof(uint16_t));
	uint8_t** p = &s->in;
	uint16_t value;
	value = (*p)[0];
	value |= (((uint16_t)((*p)[1])) << 8);
	*p += 2;
	return value;
}

static ase_fixed_t s_read_fixed(ase_state_t* s)
{
	ase_fixed_t value;
	value.a = s_read_uint16(s);
	value.b = s_read_uint16(s);
	return value;
}

static uint32_t s_read_uint32(ase_state_t* s)
{
	CUTE_ASEPRITE_ASSERT(s->in <= s->end + sizeof(uint32_t));
	uint8_t** p = &s->in;
	uint32_t value;
	value  = (*p)[0];
	value |= (((uint32_t)((*p)[1])) << 8);
	value |= (((uint32_t)((*p)[2])) << 16);
	value |= (((uint32_t)((*p)[3])) << 24);
	*p += 4;
	return value;
}

#ifdef CUTE_ASPRITE_S_READ_UINT64
// s_read_uint64() is not currently used.
static uint64_t s_read_uint64(ase_state_t* s)
{
	CUTE_ASEPRITE_ASSERT(s->in <= s->end + sizeof(uint64_t));
	uint8_t** p = &s->in;
	uint64_t value;
	value  = (*p)[0];
	value |= (((uint64_t)((*p)[1])) << 8 );
	value |= (((uint64_t)((*p)[2])) << 16);
	value |= (((uint64_t)((*p)[3])) << 24);
	value |= (((uint64_t)((*p)[4])) << 32);
	value |= (((uint64_t)((*p)[5])) << 40);
	value |= (((uint64_t)((*p)[6])) << 48);
	value |= (((uint64_t)((*p)[7])) << 56);
	*p += 8;
	return value;
}
#endif

static int16_t s_read_int16(ase_state_t* s) { return (int16_t)s_read_uint16(s); }
static int16_t s_read_int32(ase_state_t* s) { return (int32_t)s_read_uint32(s); }

#ifdef CUTE_ASPRITE_S_READ_BYTES
// s_read_bytes() is not currently used.
static void s_read_bytes(ase_state_t* s, uint8_t* bytes, int num_bytes)
{
	for (int i = 0; i < num_bytes; ++i) {
		bytes[i] = s_read_uint8(s);
	}
}
#endif

static const char* s_read_string(ase_state_t* s)
{
	int len = (int)s_read_uint16(s);
	char* bytes = (char*)CUTE_ASEPRITE_ALLOC(len + 1, s->mem_ctx);
	for (int i = 0; i < len; ++i) {
		bytes[i] = (char)s_read_uint8(s);
	}
	bytes[len] = 0;
	return bytes;
}

static void s_skip(ase_state_t* ase, int num_bytes)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + num_bytes);
	ase->in += num_bytes;
}

static char* s_fopen(const char* path, int* size, void* mem_ctx)
{
	CUTE_ASEPRITE_UNUSED(mem_ctx);
	char* data = 0;
	CUTE_ASEPRITE_FILE* fp = CUTE_ASEPRITE_FOPEN(path, "rb");
	int sz = 0;

	if (fp)
	{
		CUTE_ASEPRITE_FSEEK(fp, 0, CUTE_ASEPRITE_SEEK_END);
		sz = CUTE_ASEPRITE_FTELL(fp);
		CUTE_ASEPRITE_FSEEK(fp, 0, CUTE_ASEPRITE_SEEK_SET);
		data = (char*)CUTE_ASEPRITE_ALLOC(sz + 1, mem_ctx);
		CUTE_ASEPRITE_FREAD(data, sz, 1, fp);
		data[sz] = 0;
		CUTE_ASEPRITE_FCLOSE(fp);
	}

	if (size) *size = sz;
	return data;
}

ase_t* cute_aseprite_load_from_file(const char* path, void* mem_ctx)
{
	s_error_file = path;
	int sz;
	void* file = s_fopen(path, &sz, mem_ctx);
	if (!file) {
		CUTE_ASEPRITE_WARNING("Unable to find map file.");
		return NULL;
	}
	ase_t* aseprite = cute_aseprite_load_from_memory(file, sz, mem_ctx);
	CUTE_ASEPRITE_FREE(file, mem_ctx);
	s_error_file = NULL;
	return aseprite;
}

static int s_mul_un8(int a, int b)
{
	int t = (a * b) + 0x80;
	return (((t >> 8) + t) >> 8);
}

static ase_color_t s_blend(ase_color_t src, ase_color_t dst, uint8_t opacity)
{
	src.a = (uint8_t)s_mul_un8(src.a, opacity);
	int a = src.a + dst.a - s_mul_un8(src.a, dst.a);
	int r, g, b;
	if (a == 0) {
		r = g = b = 0;
	} else {
		r = dst.r + (src.r - dst.r) * src.a / a;
		g = dst.g + (src.g - dst.g) * src.a / a;
		b = dst.b + (src.b - dst.b) * src.a / a;
	}
	ase_color_t ret = { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a };
	return ret;
}

static int s_min(int a, int b)
{
	return a < b ? a : b;
}

static int s_max(int a, int b)
{
	return a < b ? b : a;
}

static ase_color_t s_color(ase_t* ase, void* src, int index)
{
	ase_color_t result;
	if (ase->mode == ASE_MODE_RGBA) {
		result = ((ase_color_t*)src)[index];
	} else if (ase->mode == ASE_MODE_GRAYSCALE) {
		uint8_t saturation = ((uint8_t*)src)[index * 2];
		uint8_t a = ((uint8_t*)src)[index * 2 + 1];
		result.r = result.g = result.b = saturation;
		result.a = a;
	} else {
		CUTE_ASEPRITE_ASSERT(ase->mode == ASE_MODE_INDEXED);
		uint8_t palette_index = ((uint8_t*)src)[index];
		if (palette_index == ase->transparent_palette_entry_index) {
			result.r = 0;
			result.g = 0;
			result.b = 0;
			result.a = 0;
		} else {
			result = ase->palette.entries[palette_index].color;
		}
	}
	return result;
}

ase_t* cute_aseprite_load_from_memory(const void* memory, int size, void* mem_ctx)
{
	ase_t* ase = (ase_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_t), mem_ctx);
	CUTE_ASEPRITE_MEMSET(ase, 0, sizeof(*ase));

	ase_state_t state = { 0 };
	ase_state_t* s = &state;
	s->in = (uint8_t*)memory;
	s->end = s->in + size;
	s->mem_ctx = mem_ctx;

	s_skip(s, sizeof(uint32_t)); // File size.
	int magic = (int)s_read_uint16(s);
	CUTE_ASEPRITE_ASSERT(magic == 0xA5E0);

	ase->frame_count = (int)s_read_uint16(s);
	ase->w = s_read_uint16(s);
	ase->h = s_read_uint16(s);
	uint16_t bpp = s_read_uint16(s) / 8;
	if (bpp == 4) ase->mode = ASE_MODE_RGBA;
	else if (bpp == 2) ase->mode = ASE_MODE_GRAYSCALE;
	else {
		CUTE_ASEPRITE_ASSERT(bpp == 1);
		ase->mode = ASE_MODE_INDEXED;
	}
	uint32_t valid_layer_opacity = s_read_uint32(s) & 1;
	int speed = s_read_uint16(s);
	s_skip(s, sizeof(uint32_t) * 2); // Spec says skip these bytes, as they're zero'd.
	ase->transparent_palette_entry_index = s_read_uint8(s);
	s_skip(s, 3); // Spec says skip these bytes.
	ase->number_of_colors = (int)s_read_uint16(s);
	ase->pixel_w = (int)s_read_uint8(s);
	ase->pixel_h = (int)s_read_uint8(s);
	ase->grid_x = (int)s_read_int16(s);
	ase->grid_y = (int)s_read_int16(s);
	ase->grid_w = (int)s_read_uint16(s);
	ase->grid_h = (int)s_read_uint16(s);
	s_skip(s, 84); // For future use (set to zero).

	ase->frames = (ase_frame_t*)CUTE_ASEPRITE_ALLOC((int)(sizeof(ase_frame_t)) * ase->frame_count, mem_ctx);
	CUTE_ASEPRITE_MEMSET(ase->frames, 0, sizeof(ase_frame_t) * (size_t)ase->frame_count);

	ase_udata_t* last_udata = NULL;
	int was_on_tags = 0;
	int tag_index = 0;

	ase_layer_t* layer_stack[CUTE_ASEPRITE_MAX_LAYERS];

	// Parse all chunks in the .aseprite file.
	for (int i = 0; i < ase->frame_count; ++i) {
		ase_frame_t* frame = ase->frames + i;
		frame->ase = ase;
		s_skip(s, sizeof(uint32_t)); // Frame size.
		magic = (int)s_read_uint16(s);
		CUTE_ASEPRITE_ASSERT(magic == 0xF1FA);
		int chunk_count = (int)s_read_uint16(s);
		frame->duration_milliseconds = s_read_uint16(s);
		if (frame->duration_milliseconds == 0) frame->duration_milliseconds = speed;
		s_skip(s, 2); // For future use (set to zero).
		uint32_t new_chunk_count = s_read_uint32(s);
		if (new_chunk_count) chunk_count = (int)new_chunk_count;

		for (int j = 0; j < chunk_count; ++j) {
			uint32_t chunk_size = s_read_uint32(s);
			uint16_t chunk_type = s_read_uint16(s);
			chunk_size -= (uint32_t)(sizeof(uint32_t) + sizeof(uint16_t));
			uint8_t* chunk_start = s->in;

			switch (chunk_type) {
			case 0x2004: // Layer chunk.
			{
				CUTE_ASEPRITE_ASSERT(ase->layer_count < CUTE_ASEPRITE_MAX_LAYERS);
				ase_layer_t* layer = ase->layers + ase->layer_count++;
				layer->flags = (ase_layer_flags_t)s_read_uint16(s);
				layer->type = (ase_layer_type_t)s_read_uint16(s);
				layer->parent = NULL;
				int child_level = (int)s_read_uint16(s);
				layer_stack[child_level] = layer;
				if (child_level) {
					layer->parent = layer_stack[child_level - 1];
				}
				s_skip(s, sizeof(uint16_t)); // Default layer width in pixels (ignored).
				s_skip(s, sizeof(uint16_t)); // Default layer height in pixels (ignored).
				int blend_mode = (int)s_read_uint16(s);
				if (blend_mode) CUTE_ASEPRITE_WARNING("Unknown blend mode encountered.");
				layer->opacity = s_read_uint8(s) / 255.0f;
				if (!valid_layer_opacity) layer->opacity = 1.0f;
				s_skip(s, 3); // For future use (set to zero).
				layer->name = s_read_string(s);
				last_udata = &layer->udata;
			}	break;

			case 0x2005: // Cel chunk.
			{
				CUTE_ASEPRITE_ASSERT(frame->cel_count < CUTE_ASEPRITE_MAX_LAYERS);
				ase_cel_t* cel = frame->cels + frame->cel_count++;
				int layer_index = (int)s_read_uint16(s);
				cel->layer = ase->layers + layer_index;
				cel->x = s_read_int16(s);
				cel->y = s_read_int16(s);
				cel->opacity = s_read_uint8(s) / 255.0f;
				int cel_type = (int)s_read_uint16(s);
				s_skip(s, 7); // For future (set to zero).
				switch (cel_type) {
				case 0: // Raw cel.
					cel->w = s_read_uint16(s);
					cel->h = s_read_uint16(s);
					cel->pixels = CUTE_ASEPRITE_ALLOC(cel->w * cel->h * bpp, mem_ctx);
					CUTE_ASEPRITE_MEMCPY(cel->pixels, s->in, (size_t)(cel->w * cel->h * bpp));
					s_skip(s, cel->w * cel->h * bpp);
					break;

				case 1: // Linked cel.
					cel->is_linked = 1;
					cel->linked_frame_index = s_read_uint16(s);
					break;

				case 2: // Compressed image cel.
				{
					cel->w = s_read_uint16(s);
					cel->h = s_read_uint16(s);
					int zlib_byte0 = s_read_uint8(s);
					int zlib_byte1 = s_read_uint8(s);
					int deflate_bytes = (int)chunk_size - (int)(s->in - chunk_start);
					void* pixels = s->in;
					CUTE_ASEPRITE_ASSERT((zlib_byte0 & 0x0F) == 0x08); // Only zlib compression method (RFC 1950) is supported.
					CUTE_ASEPRITE_ASSERT((zlib_byte0 & 0xF0) <= 0x70); // Innapropriate window size detected.
					CUTE_ASEPRITE_ASSERT(!(zlib_byte1 & 0x20)); // Preset dictionary is present and not supported.
					int pixels_sz = cel->w * cel->h * bpp;
					void* pixels_decompressed = CUTE_ASEPRITE_ALLOC(pixels_sz, mem_ctx);
					int ret = s_inflate(pixels, deflate_bytes, pixels_decompressed, pixels_sz, mem_ctx);
					if (!ret) CUTE_ASEPRITE_WARNING(s_error_reason);
					cel->pixels = pixels_decompressed;
					s_skip(s, deflate_bytes);
				}	break;
				}
				last_udata = &cel->udata;
			}	break;

			case 0x2006: // Cel extra chunk.
			{
				ase_cel_t* cel = frame->cels + frame->cel_count;
				cel->has_extra = 1;
				cel->extra.precise_bounds_are_set = (int)s_read_uint32(s);
				cel->extra.precise_x = s_read_fixed(s);
				cel->extra.precise_y = s_read_fixed(s);
				cel->extra.w = s_read_fixed(s);
				cel->extra.h = s_read_fixed(s);
				s_skip(s, 16); // For future use (set to zero).
			}	break;

			case 0x2007: // Color profile chunk.
			{
				ase->has_color_profile = 1;
				ase->color_profile.type = (ase_color_profile_type_t)s_read_uint16(s);
				ase->color_profile.use_fixed_gamma = (int)s_read_uint16(s) & 1;
				ase->color_profile.gamma = s_read_fixed(s);
				s_skip(s, 8); // For future use (set to zero).
				if (ase->color_profile.type == ASE_COLOR_PROFILE_TYPE_EMBEDDED_ICC) {
					// Use the embedded ICC profile.
					ase->color_profile.icc_profile_data_length = s_read_uint32(s);
					ase->color_profile.icc_profile_data = CUTE_ASEPRITE_ALLOC(ase->color_profile.icc_profile_data_length, mem_ctx);
					CUTE_ASEPRITE_MEMCPY(ase->color_profile.icc_profile_data, s->in, ase->color_profile.icc_profile_data_length);
					s->in += ase->color_profile.icc_profile_data_length;
				}
			}	break;

			case 0x2018: // Tags chunk.
			{
				ase->tag_count = (int)s_read_uint16(s);
				s_skip(s, 8); // For future (set to zero).
				CUTE_ASEPRITE_ASSERT(ase->tag_count < CUTE_ASEPRITE_MAX_TAGS);
				for (int k = 0; k < ase->tag_count; ++k) {
					ase_tag_t tag;
					tag.from_frame = (int)s_read_uint16(s);
					tag.to_frame = (int)s_read_uint16(s);
					tag.loop_animation_direction = (ase_animation_direction_t)s_read_uint8(s);
					s_skip(s, 8); // For future (set to zero).
					tag.r = s_read_uint8(s);
					tag.g = s_read_uint8(s);
					tag.b = s_read_uint8(s);
					s_skip(s, 1); // Extra byte (zero).
					tag.name = s_read_string(s);
					ase->tags[k] = tag;
				}
				was_on_tags = 1;
			}	break;

			case 0x2019: // Palette chunk.
			{
				ase->palette.entry_count = (int)s_read_uint32(s);
				CUTE_ASEPRITE_ASSERT(ase->palette.entry_count <= CUTE_ASEPRITE_MAX_PALETTE_ENTRIES);
				int first_index = (int)s_read_uint32(s);
				int last_index = (int)s_read_uint32(s);
				s_skip(s, 8); // For future (set to zero).
				for (int k = first_index; k <= last_index; ++k) {
					int has_name = s_read_uint16(s);
					ase_palette_entry_t entry;
					entry.color.r = s_read_uint8(s);
					entry.color.g = s_read_uint8(s);
					entry.color.b = s_read_uint8(s);
					entry.color.a = s_read_uint8(s);
					if (has_name) {
						entry.color_name = s_read_string(s);
					} else {
						entry.color_name = NULL;
					}
					CUTE_ASEPRITE_ASSERT(k < CUTE_ASEPRITE_MAX_PALETTE_ENTRIES);
					ase->palette.entries[k] = entry;
				}
			}	break;

			case 0x2020: // Udata chunk.
			{
				CUTE_ASEPRITE_ASSERT(last_udata || was_on_tags);
				if (was_on_tags && !last_udata) {
					CUTE_ASEPRITE_ASSERT(tag_index < ase->tag_count);
					last_udata = &ase->tags[tag_index++].udata;
				}
				int flags = (int)s_read_uint32(s);
				if (flags & 1) {
					last_udata->has_text = 1;
					last_udata->text = s_read_string(s);
				}
				if (flags & 2) {
					last_udata->color.r = s_read_uint8(s);
					last_udata->color.g = s_read_uint8(s);
					last_udata->color.b = s_read_uint8(s);
					last_udata->color.a = s_read_uint8(s);
				}
				last_udata = NULL;
			}	break;

			case 0x2022: // Slice chunk.
			{
				int slice_count = (int)s_read_uint32(s);
				int flags = (int)s_read_uint32(s);
				s_skip(s, sizeof(uint32_t)); // Reserved.
				const char* name = s_read_string(s);
				for (int k = 0; k < (int)slice_count; ++k) {
					ase_slice_t slice = { 0 };
					slice.name = name;
					slice.frame_number = (int)s_read_uint32(s);
					slice.origin_x = (int)s_read_int32(s);
					slice.origin_y = (int)s_read_int32(s);
					slice.w = (int)s_read_uint32(s);
					slice.h = (int)s_read_uint32(s);
					if (flags & 1) {
						// It's a 9-patches slice.
						slice.has_center_as_9_slice = 1;
						slice.center_x = (int)s_read_int32(s);
						slice.center_y = (int)s_read_int32(s);
						slice.center_w = (int)s_read_uint32(s);
						slice.center_h = (int)s_read_uint32(s);
					} else if (flags & 2) {
						// Has pivot information.
						slice.has_pivot = 1;
						slice.pivot_x = (int)s_read_int32(s);
						slice.pivot_y = (int)s_read_int32(s);
					}
					CUTE_ASEPRITE_ASSERT(ase->slice_count < CUTE_ASEPRITE_MAX_SLICES);
					ase->slices[ase->slice_count++] = slice;
					last_udata = &ase->slices[ase->slice_count - 1].udata;
				}
			}	break;

			default:
				s_skip(s, (int)chunk_size);
				break;
			}

			uint32_t size_read = (uint32_t)(s->in - chunk_start);
			CUTE_ASEPRITE_ASSERT(size_read == chunk_size);
		}
	}

	// Blend all cel pixels into each of their respective frames, for convenience.
	for (int i = 0; i < ase->frame_count; ++i) {
		ase_frame_t* frame = ase->frames + i;
		frame->pixels = (ase_color_t*)CUTE_ASEPRITE_ALLOC((int)(sizeof(ase_color_t)) * ase->w * ase->h, mem_ctx);
		CUTE_ASEPRITE_MEMSET(frame->pixels, 0, sizeof(ase_color_t) * (size_t)ase->w * (size_t)ase->h);
		ase_color_t* dst = frame->pixels;
		for (int j = 0; j < frame->cel_count; ++j) {
			ase_cel_t* cel = frame->cels + j;
			if (!(cel->layer->flags & ASE_LAYER_FLAGS_VISIBLE)) {
				continue;
			}
			if (cel->layer->parent && !(cel->layer->parent->flags & ASE_LAYER_FLAGS_VISIBLE)) {
				continue;
			}
			while (cel->is_linked) {
				ase_frame_t* frame = ase->frames + cel->linked_frame_index;
				int found = 0;
				for (int k = 0; k < frame->cel_count; ++k) {
					if (frame->cels[k].layer == cel->layer) {
						cel = frame->cels + k;
						found = 1;
						break;
					}
				}
				CUTE_ASEPRITE_ASSERT(found);
			}
			void* src = cel->pixels;
			uint8_t opacity = (uint8_t)(cel->opacity * cel->layer->opacity * 255.0f);
			int cx = cel->x;
			int cy = cel->y;
			int cw = cel->w;
			int ch = cel->h;
			int cl = -s_min(cx, 0);
			int ct = -s_min(cy, 0);
			int dl = s_max(cx, 0);
			int dt = s_max(cy, 0);
			int dr = s_min(ase->w, cw + cx);
			int db = s_min(ase->h, ch + cy);
			int aw = ase->w;
			for (int dx = dl, sx = cl; dx < dr; dx++, sx++) {
				for (int dy = dt, sy = ct; dy < db; dy++, sy++) {
					int dst_index = aw * dy + dx;
					ase_color_t src_color = s_color(ase, src, cw * sy + sx);
					ase_color_t dst_color = dst[dst_index];
					ase_color_t result = s_blend(src_color, dst_color, opacity);
					dst[dst_index] = result;
				}
			}
		}
	}

	ase->mem_ctx = mem_ctx;
	return ase;
}

void cute_aseprite_free(ase_t* ase)
{
	for (int i = 0; i < ase->frame_count; ++i) {
		ase_frame_t* frame = ase->frames + i;
		CUTE_ASEPRITE_FREE(frame->pixels, ase->mem_ctx);
		for (int j = 0; j < frame->cel_count; ++j) {
			ase_cel_t* cel = frame->cels + j;
			CUTE_ASEPRITE_FREE(cel->pixels, ase->mem_ctx);
			CUTE_ASEPRITE_FREE((void*)cel->udata.text, ase->mem_ctx);
		}
	}
	for (int i = 0; i < ase->layer_count; ++i) {
		ase_layer_t* layer = ase->layers + i;
		CUTE_ASEPRITE_FREE((void*)layer->name, ase->mem_ctx);
		CUTE_ASEPRITE_FREE((void*)layer->udata.text, ase->mem_ctx);
	}
	for (int i = 0; i < ase->tag_count; ++i) {
		ase_tag_t* tag = ase->tags + i;
		CUTE_ASEPRITE_FREE((void*)tag->name, ase->mem_ctx);
	}
	for (int i = 0; i < ase->slice_count; ++i) {
		ase_slice_t* slice = ase->slices + i;
		CUTE_ASEPRITE_FREE((void*)slice->udata.text, ase->mem_ctx);
	}
	if (ase->slice_count) {
		CUTE_ASEPRITE_FREE((void*)ase->slices[0].name, ase->mem_ctx);
	}
	for (int i = 0; i < ase->palette.entry_count; ++i) {
		CUTE_ASEPRITE_FREE((void*)ase->palette.entries[i].color_name, ase->mem_ctx);
	}
	CUTE_ASEPRITE_FREE(ase->color_profile.icc_profile_data, ase->mem_ctx);
	CUTE_ASEPRITE_FREE(ase->frames, ase->mem_ctx);
	CUTE_ASEPRITE_FREE(ase, ase->mem_ctx);
}

#endif // CUTE_ASEPRITE_IMPLEMENTATION_ONCE
#endif // CUTE_ASEPRITE_IMPLEMENTATION
