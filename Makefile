# Program
CC		= cc
CFLAGS	= -std=c99 -Wall -Wextra -I/usr/local/include -I./pcg-c/include
LDFLAGS	= -L/usr/local/lib -L./pcg-c/src
LDLIBS  = -L./pcg-c/src -lpcg_random

# Test
SEED	= 4294967296
ITER	= 114514

# Docker
ARCH	= arm64v8
COMPOSE	= docker-compose -p $(shell basename $(CURDIR))

# Program
clean:;
	make -C "pcg-c" clean
	make -C "pcg-c/src" all
	rm -f *.dat *.o *~ pcg64_example pcg64_example_emulate reference_implementation

all32: clean
	$(CC) $(CFLAGS) $(LDFLAGS) pcg64_example.c $(LDLIBS) -o pcg64_example

all: all32
	$(CC) $(CFLAGS) -DFORCE_EMULATE_128 $(LDFLAGS) pcg64_example.c $(LDLIBS) -o pcg64_example_emulate
	$(CC) $(CFLAGS) $(LDFLAGS) reference_implementation.c $(LDLIBS) -o reference_implementation

test32: clean all32
	./pcg64_example 4294967296 114514 pcg64_example_32.dat
	if ! cmp "static_reference_implementation_seed_4294967296_iter_114514.refdat" "pcg64_example_32.dat"; then echo "FAILED (pcg64emu <-> static reference)"; exit 1; fi
	echo "SUCCESS"

test: clean all
	./pcg64_example ${SEED} ${ITER} pcg64_example.dat
	./pcg64_example_emulate ${SEED} ${ITER} pcg64_example_emulate.dat
	./reference_implementation ${SEED} ${ITER} reference_implementation.dat
	if ! cmp "pcg64_example.dat" "pcg64_example_emulate.dat"; then echo "FAILED (pcg64 <-> pcg64emu)"; exit 1; fi
	if ! cmp "pcg64_example.dat" "reference_implementation.dat"; then echo "FAILED (pcg64 <-> reference)"; exit 1; fi
	echo "SUCCESS"

# Docker
.PHONY: build
build:
	$(COMPOSE) build ${ARCH}

.PHONY: build-all
build-all:
	$(COMPOSE) build

.PHONY: docker-test
docker-test: build
	$(COMPOSE) run --rm ${ARCH} /bin/bash -c "make test"

.PHONY: docker-test32
docker-test32: build
	$(COMPOSE) run --rm ${ARCH} /bin/bash -c "make test32"

.PHONY: docker-test32-all
docker-test32-all: build-all
	make docker-test32 ARCH=arm32v7
	make docker-test32 ARCH=i386

.PHONY: docker-test-all
docker-test-all: build-all docker-test32-all
	make docker-test ARCH=amd64
	make docker-test ARCH=arm64v8
	make docker-test ARCH=s390x

.PHONY: up
up:
	$(COMPOSE) up -d --build ${ARCH}

.PHONY: shell
shell:
	$(COMPOSE) exec ${ARCH} /bin/bash -l

.PHONY: down
down:
	$(COMPOSE) down --remove-orphans
