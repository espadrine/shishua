SHELL := bash
CFLAGS := -O3 -g
FINGERPRINT := $(shell ./shishua -b 256 2>/dev/null | ./bin/fingerprint.sh)
TARGETS := scalar sse2 ssse3 avx2 neon
SHISHUAS :=  shishua shishua-half \
             $(addprefix shishua-,$(TARGETS)) \
             $(addprefix shishua-half-,$(TARGETS))
PRNGS := shishua shishua-half chacha8 xoshiro256plusx8 xoshiro256plus romu wyrand lehmer128 rc4
# Should match header names (aside from -scalar and -ssse3)
IMPLS := $(SHISHUAS) chacha8 chacha8-avx2 chacha8-neon xoshiro256plusx8 xoshiro256plus romu wyrand lehmer128 rc4
TESTS := $(addprefix test-,$(TARGETS))

# We need second expansions.
.SECONDEXPANSION:

##
## Target rules.
##

# Replace pseudo target names with the real names.
# The HEADER preproc variable is used in prng.c.
fix_target = $(subst -scalar,,$(subst -ssse3,-sse2,$(1)))
$(IMPLS): HEADER = $(call fix_target,$@).h
# HEADERS ensures modifications on -neon.h etc. cause recompiles.
$(PRNGS): HEADERS = $(call fix_target,$@)*.h
$(TESTS): SUFFIX = $(patsubst test%,%.h,$(call fix_target,$@))

# Force SSE2, disable SSE3
%-sse2: CFLAGS += -msse2 -mno-sse3 -mno-ssse3
%-ssse3: CFLAGS += -mssse3
# -mtune=haswell disables GCC load/store splitting
%-avx2: CFLAGS += -mavx2 -mtune=haswell
# force scalar target
%-scalar: CFLAGS += -DSHISHUA_TARGET=SHISHUA_TARGET_SCALAR

ifeq ($(shell $(CC) -v 2>&1 | tail -1 | grep -o gcc),gcc)
  xoshiro256plusx8: CFLAGS += -fdisable-tree-cunrolli
  CFLAGS += -march=native
else
  CFLAGS += -mcpu=native
endif

##
## Recipes.
##
default: shishua shishua-half

# e.g. make neon -> make shishua-neon shishua-half-neon
$(TARGETS): %: shishua-% shishua-half-%

$(IMPLS): $$(HEADER) $$(HEADERS) prng.c
	$(CC) $(CFLAGS) -DHEADER='"$(HEADER)"' prng.c -o $@

$(TESTS): test-vectors.c test-vectors.h shishua$$(SUFFIX) shishua-half$$(SUFFIX)
	$(CC) $(CFLAGS) -DHEADER='"shishua$(SUFFIX)"' -DHEADER_HALF='"shishua-half$(SUFFIX)"' $< -o $@
	./$@

intertwine: intertwine.c
	$(CC) $(CFLAGS) -o $@ $<

##
## Quality testing.
##

/usr/local/bin/RNG_test:
	mkdir PractRand
	curl -Ls 'https://downloads.sourceforge.net/project/pracrand/PractRand-pre0.95.zip' >PractRand/PractRand.zip
	cd PractRand; \
	  unzip PractRand.zip; \
	  g++ -c src/*.cpp src/RNGs/*.cpp src/RNGs/other/*.cpp -O3 -Iinclude -pthread; \
	  ar rcs libPractRand.a *.o; \
	  g++ -o RNG_test tools/RNG_test.cpp libPractRand.a -O3 -Iinclude -pthread; \
	  g++ -o RNG_benchmark tools/RNG_benchmark.cpp libPractRand.a -O3 -Iinclude -pthread; \
	  g++ -o RNG_output tools/RNG_output.cpp libPractRand.a -O3 -Iinclude -pthread
	sudo mv PractRand/RNG_{test,benchmark,output} /usr/local/bin
	rm -rf PractRand

/usr/local/bin/testu01: testu01.c
	curl -sO 'http://simul.iro.umontreal.ca/testu01/TestU01.zip'
	unzip TestU01.zip
	mv TestU01-*/ TestU01
	cd TestU01; \
	  ./configure --prefix="$$(dirname $$(pwd))"; \
	  make; make install
	gcc -std=c99 -Wall -O3 -o testu01 testu01.c -Iinclude -Llib -ltestu01 -lprobdist -lmylib -lm
	sudo mv testu01 /usr/local/bin
	rm -rf TestU01*

test: test/perf-$(FINGERPRINT) test/PractRand-$(FINGERPRINT) test/BigCrush-$(FINGERPRINT)

test/PractRand-$(FINGERPRINT): /usr/local/bin/RNG_test shishua
	@mkdir -p test
	@echo "Date $$(date)" | tee test/PractRand-$(FINGERPRINT)
	@echo "PRNG fingerprint: $(FINGERPRINT)" | tee -a test/PractRand-$(FINGERPRINT)
	./shishua | RNG_test stdin64 | tee -a test/PractRand-$(FINGERPRINT)

test/BigCrush-$(FINGERPRINT): /usr/local/bin/testu01 shishua
	@mkdir -p test
	@echo "Date $$(date)" | tee test/BigCrush-$(FINGERPRINT)
	@echo "PRNG fingerprint: $(FINGERPRINT)" | tee -a test/BigCrush-$(FINGERPRINT)
	./shishua | testu01 --big | tee -a test/BigCrush-$(FINGERPRINT)

test/benchmark-seed: $(PRNGS) intertwine
	@mkdir -p test
	@echo "Date $$(date)" | tee $@
	for prng in $(PRNGS); do \
	  echo "$$prng fingerprint: $$(./$$prng | ./bin/fingerprint.sh)" | tee -a $@; \
	  ./intertwine <(./$$prng -s 1) <(./$$prng -s 2) \
	               <(./$$prng -s 4) <(./$$prng -s 8) \
	               <(./$$prng -s 10) <(./$$prng -s 20) \
	               <(./$$prng -s 40) <(./$$prng -s 80) \
	    | RNG_test stdin -tlmax 1M -tlmin 1K -te 1 -tf 2 | tee -a $@; \
	done

##
## Performance testing.
##

# This must be performed with no other processes running.

test/perf-$(FINGERPRINT): shishua
	@mkdir -p test
	@echo "Date $$(date)" | tee $@
	@echo "PRNG fingerprint: $(FINGERPRINT)" | tee -a $@
	./shishua --bytes 4294967296 -q 2>&1 | tee -a $@

test/benchmark-perf: $(PRNGS)
	@mkdir -p test
	@echo "Date $$(date)" | tee $@
	for prng in $(PRNGS); do \
	  ./bin/fix-cpu-freq.sh ./$$prng --bytes 10000000000 -q 2>&1 | tee -a $@; \
	done
	@echo | tee -a $@
	@lscpu | tee -a $@

# To reach a consistent benchmark, we need a universally-reproducible system.
# GCP will do.

# Installation instructions from https://cloud.google.com/sdk/docs/downloads-apt-get
/usr/bin/gcloud:
	echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | sudo tee -a /etc/apt/sources.list.d/google-cloud-sdk.list
	sudo apt-get install apt-transport-https ca-certificates gnupg
	curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key --keyring /usr/share/keyrings/cloud.google.gpg add -
	sudo apt-get update && sudo apt-get install google-cloud-sdk
	gcloud init

# Installation instructions for Amazon Web Services CLI (for ARM servers with NEON)
# available here: https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2-linux.html
/usr/local/bin/aws:
	curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
	unzip awscliv2.zip
	sudo ./aws/install
	rm -r awscliv2.zip aws
	aws configure

# Installation instructions for Scaleway CLI (for ARM servers without NEON)
# available here: https://github.com/scaleway/scaleway-cli#linux
/usr/local/bin/scw:
	sudo curl -o /usr/local/bin/scw -L "https://github.com/scaleway/scaleway-cli/releases/download/v2.2.3/scw-2.2.3-linux-x86_64"
	sudo chmod +x /usr/local/bin/scw
	scw init

test/benchmark-perf-intel: /usr/bin/gcloud
	./bin/benchmark-intel

test/benchmark-perf-amd: /usr/bin/gcloud
	./bin/benchmark-amd

test/benchmark-perf-arm: /usr/local/bin/aws
	./bin/benchmark-arm

test/benchmark-perf-arm-without-neon: /usr/local/bin/scw
	./bin/benchmark-arm-without-neon

clean:
	$(RM) -rf $(TESTS) $(IMPLS) intertwine

.PHONY: test clean
