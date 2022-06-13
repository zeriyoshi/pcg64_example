#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#include "./pcg-c/include/pcg_variants.h"

int main(int argc, char **argv) {
    if (argc < 3) {
		printf("%s\n", "requires: {seed} {iterations} {output_file}");
		return 1;
    } 

    pcg64s_random_t r;
    __uint128_t seed, advance = UINT64_MAX;
    int iterations, i;
    FILE *fp;

    seed = (__uint128_t) strtoull(argv[1], NULL, 10);
    iterations = atoi(argv[2]);
    fp = fopen(argv[3], "w");

    if (fp == NULL) {
        printf("fopen failed: %s\n", argv[3]);
        return 2;
    }

    printf("parameters:\n\tseed: %" PRIu64 "\n\titerations: %i\n\toutput_file: %s\n", (uint64_t) seed, iterations, argv[3]);

    printf("seeding...");
    pcg64s_srandom_r(&r, seed);
	printf("OK\n");

	printf("generating...");
	fprintf(fp, "seed: %" PRIu64 "\n", (uint64_t) seed);
	for (i = 0; i < iterations; i++) {
		fprintf(fp, "%i: %" PRIu64 "\n", i + 1, pcg64s_random_r(&r));
	}

    pcg64s_advance_r(&r, advance);
    fprintf(fp, "advance %llu: %" PRIu64 "\n", (uint64_t) advance, pcg64s_random_r(&r));

	printf("done\n");

	fclose(fp);

	printf("%s\n", "finished");
	return 0;
}
