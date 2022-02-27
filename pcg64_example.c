#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#if !defined(__SIZEOF_INT128__) || defined(FORCE_EMULATE_128)
# define RANDOM_PCG64_EMULATED
typedef struct _random_uint128_t {
	uint64_t hi;
	uint64_t lo;
} random_uint128_t;
# define UINT128_CON(x, y, result)	\
	do { \
		result.hi = x; \
		result.lo = y; \
	} while (0);
# define UINT128_ADD(x, y, result)	\
	do { \
		const uint64_t _lo = (x.lo + y.lo), _hi = (x.hi + y.hi + (_lo < x.lo)); \
		result.hi = _hi; \
		result.lo = _lo; \
	} while (0);
# define UINT128_MUL(x, y, result)	\
	do { \
		const uint64_t \
			_x0 = x.lo & 0xffffffffULL, \
			_x1 = x.lo >> 32, \
			_y0 = y.lo & 0xffffffffULL, \
			_y1 = y.lo >> 32, \
			_z0 = ((_x1 * _y0 + (_x0 * _y0 >> 32)) & 0xffffffffULL) + _x0 * _y1; \
		result.hi = x.hi * y.lo + x.lo * y.hi; \
		result.lo = x.lo * y.lo; \
		result.hi += _x1 * _y1 + ((_x1 * _y0 + (_x0 * _y0 >> 32)) >> 32) + (_z0 >> 32); \
	} while (0);
# define PCG64_ROTL1OR1(x, result)	\
	do { \
		result.hi = x.hi << 1U | x.lo >> 63U; \
		result.lo = x.lo << 1U | 1U; \
	} while (0);
# define PCG64_ROTR64(x, result)	\
	do { \
		const uint64_t _v = (x.hi ^ x.lo), _s = x.hi >> 58U; \
		result = (_v >> _s) | (_v << ((-_s) & 63)); \
	} while (0);

#else
typedef __uint128_t random_uint128_t;
# define UINT128_CON(x, y, result)	result = (((random_uint128_t) x << 64) + y);
# define UINT128_ADD(x, y, result)	result = x + y;
# define UINT128_MUL(x, y, result)	result = x * y;
# define PCG64_ROTL1OR1(x, result)	result = (x << 1U) | 1U;
# define PCG64_ROTR64(x, result)	\
	do { \
		uint64_t _v = ((uint64_t) (x >> 64U)) ^ (uint64_t) x, _s = x >> 122U; \
		result = (_v >> _s) | (_v << ((-_s) & 63)); \
	} while (0);
#endif

typedef struct _php_random_engine_state_pcg64 {
	random_uint128_t s;
	random_uint128_t inc;
} php_random_engine_state_pcg64;

static inline void pcg64_step(php_random_engine_state_pcg64 *s) {
	random_uint128_t c;
	UINT128_CON(2549297995355413924ULL, 4865540595714422341ULL, c);

	UINT128_MUL(s->s, c, s->s);
	UINT128_ADD(s->s, s->inc, s->s);
}

static uint64_t pcg64_generate(void *state) {
	php_random_engine_state_pcg64 *s = (php_random_engine_state_pcg64 *) state;
	uint64_t result;

	pcg64_step(s);
	PCG64_ROTR64(s->s, result);
	
	return result;
}

static void pcg64_seed(void *state, const uint64_t seed) {
	php_random_engine_state_pcg64 *s = (php_random_engine_state_pcg64 *) state;
	random_uint128_t c;
	
	UINT128_CON(0ULL, seed, c);
	PCG64_ROTL1OR1(s->s, s->inc);
	pcg64_step(s);
	UINT128_ADD(c, s->s, s->s);
	pcg64_step(s);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("%s\n", "requires: {seed} {iterations} {output_file}");
		return 1;
	}

	php_random_engine_state_pcg64 *s = calloc(1, sizeof(php_random_engine_state_pcg64));
	uint64_t seed;
	int iterations, i;
	FILE *fp;

	seed = strtoull(argv[1], NULL, 10);
	iterations = atoi(argv[2]);
	fp = fopen(argv[3], "w");
	
	if (fp == NULL) {
		printf("fopen failed: %s\n", argv[3]);
		free(s);
		return 2;
	}

#ifdef FORCE_EMULATE_128
	printf("%s\n", "NOTICE: Emulated __uint128_t");
#endif

	printf("parameters:\n\tseed: %" PRIu64 "\n\titerations: %i\n\toutput_file: %s\n", seed, iterations, argv[3]);

	printf("seeding...");
	pcg64_seed(s, seed);
	printf("OK\n");

	printf("generating...");
	fprintf(fp, "seed: %" PRIu64 "\n", seed);
	for (i = 0; i < iterations; i++) {
		fprintf(fp, "%i: %" PRIu64 "\n", i + 1, pcg64_generate(s));
	}
	printf("done\n");

	fclose(fp);

	free(s);

	printf("%s\n", "finished");
	return 0;
}
