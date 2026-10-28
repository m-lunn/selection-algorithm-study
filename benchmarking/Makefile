# Keep `make -C benchmarking` working for users following benchmarking/README.md.
ROOT := $(abspath ..)

.PHONY: all test clean

all test clean:
	$(MAKE) -C $(ROOT) $@
