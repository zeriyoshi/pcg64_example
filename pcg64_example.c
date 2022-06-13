#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#if !defined(__SIZEOF_INT128__) || defined(PHP_RANDOM_FORCE_EMULATE_128)
typedef struct _random_uint128_t {
	uint64_t hi;
	uint64_t lo;
} random_uint128_t;

static inline random_uint128_t php_random_uint128_constant(uint64_t hi, uint64_t lo)
{
	random_uint128_t r;
	
	r.hi = hi;
	r.lo = lo;

	return r;
}

static inline random_uint128_t php_random_uint128_add(random_uint128_t num1, random_uint128_t num2)
{
	random_uint128_t r;
	
	r.lo = (num1.lo + num2.lo);
	r.hi = (num1.hi + num2.hi + (r.lo < num1.lo));

	return r;
}

static inline random_uint128_t php_random_uint128_multiply(random_uint128_t num1, random_uint128_t num2)
{
	random_uint128_t r;
	const uint64_t
		x0 = num1.lo & 0xffffffffULL,
		x1 = num1.lo >> 32,
		y0 = num2.lo & 0xffffffffULL,
		y1 = num2.lo >> 32,
		z0 = (((x1 * y0) + (x0 * y0 >> 32)) & 0xffffffffULL) + x0 * y1;
	
	r.hi = num1.hi * num2.lo + num1.lo * num2.hi;
	r.lo = num1.lo * num2.lo;
	r.hi += x1 * y1 + ((x1 * y0 + (x0 * y0 >> 32)) >> 32) + (z0 >> 32);

	return r;
}

static inline uint64_t php_random_pcg64s_rotr64(random_uint128_t num)
{
	const uint64_t
		v = (num.hi ^ num.lo),
		s = num.hi >> 58U;

	return (v >> s) | (v << ((-s) & 63));
}
#else
typedef __uint128_t random_uint128_t;

static inline random_uint128_t php_random_uint128_constant(uint64_t hi, uint64_t lo)
{
	random_uint128_t r;

	r = ((random_uint128_t) hi << 64) + lo;

	return r;
}

static inline random_uint128_t php_random_uint128_add(random_uint128_t num1, random_uint128_t num2)
{
	return num1 + num2;
}

static inline random_uint128_t php_random_uint128_multiply(random_uint128_t num1, random_uint128_t num2)
{
	return num1 * num2;
}

static inline uint64_t php_random_pcg64s_rotr64(random_uint128_t num)
{
	const uint64_t 
		v = ((uint64_t) (num >> 64U)) ^ (uint64_t) num,
		s = num >> 122U;
	
	return (v >> s) | (v << ((-s) & 63));
}
#endif


typedef struct _php_random_engine_state_pcg64 {
	random_uint128_t s;
	random_uint128_t inc;
} php_random_engine_state_pcg64;

static inline void pcg64s_step(php_random_engine_state_pcg64 *s) {
	s->s = php_random_uint128_add(
		php_random_uint128_multiply(s->s, php_random_uint128_constant(2549297995355413924ULL,4865540595714422341ULL)),
		php_random_uint128_constant(6364136223846793005ULL,1442695040888963407ULL)
	);
}

static inline void pcg64s_advance(php_random_engine_state_pcg64 *s, uint64_t advance) {	
	random_uint128_t
		cur_mult = php_random_uint128_constant(2549297995355413924ULL,4865540595714422341ULL),
		cur_plus = php_random_uint128_constant(6364136223846793005ULL,1442695040888963407ULL),
		acc_mult = php_random_uint128_constant(0ULL, 1ULL),
		acc_plus = php_random_uint128_constant(0ULL, 0ULL);

	while (advance > 0) {
		if (advance & 1) {
			acc_mult = php_random_uint128_multiply(acc_mult, cur_mult);
			acc_plus = php_random_uint128_add(php_random_uint128_multiply(acc_plus, cur_mult), cur_plus);
		}
		cur_plus = php_random_uint128_multiply(php_random_uint128_add(cur_mult, php_random_uint128_constant(0ULL, 1ULL)), cur_plus);
		cur_mult = php_random_uint128_multiply(cur_mult, cur_mult);
		advance /= 2;
	}

	s->s = php_random_uint128_add(php_random_uint128_multiply(acc_mult, s->s), acc_plus);
}

static uint64_t pcg64s_generate(void *state) {
	php_random_engine_state_pcg64 *s = (php_random_engine_state_pcg64 *) state;
	uint64_t result;

	pcg64s_step(s);
	result = php_random_pcg64s_rotr64(s->s);
	
	return result;
}

static void pcg64s_seed(void *state, const random_uint128_t seed) {
	php_random_engine_state_pcg64 *s = (php_random_engine_state_pcg64 *) state;
	
	s->s = php_random_uint128_constant(0ULL, 0ULL);
	pcg64s_step(s);
	s->s = php_random_uint128_add(s->s, seed);
	pcg64s_step(s);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("%s\n", "requires: {seed} {iterations} {output_file}");
		return 1;
	}

	php_random_engine_state_pcg64 *s = calloc(1, sizeof(php_random_engine_state_pcg64));
	uint64_t seed, advance = UINT64_MAX;
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

#ifdef PHP_RANDOM_FORCE_EMULATE_128
	printf("%s\n", "NOTICE: Emulated __uint128_t");
#endif

	printf("parameters:\n\tseed: %" PRIu64 "\n\titerations: %i\n\toutput_file: %s\n", seed, iterations, argv[3]);

	printf("seeding...");
	pcg64s_seed(s, php_random_uint128_constant(0ULL, seed));
	printf("OK\n");

	printf("generating...");
	fprintf(fp, "seed: %" PRIu64 "\n", seed);
	for (i = 0; i < iterations; i++) {
		fprintf(fp, "%i: %" PRIu64 "\n", i + 1, pcg64s_generate(s));
	}

	pcg64s_advance(s, advance);
	fprintf(fp, "advance %llu: %" PRIu64 "\n", advance, pcg64s_generate(s));

	printf("done\n");

	fclose(fp);

	free(s);

	printf("%s\n", "finished");
	return 0;
}
