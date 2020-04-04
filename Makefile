SHELL = bash
CCFLAGS =
FINGERPRINT = $(shell ./shishua | ./fingerprint.sh)
# We support multiple flags, such as changing the gear size:
#CCFLAGS = -DCOMBIT_REGSIZE=8
# …or enabling debugging mode:
#CCFLAGS = -DCOMBIT_DEBUG=2

shishua: shishua.h prng.c
	cp shishua.h prng.h
	gcc -O9 -mavx2 $(CCFLAGS) -o $@ prng.c
	rm prng.h

romu: romu.h prng.c
	cp romu.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

xoshiro256plusx8: xoshiro256+x8.h prng.c
	cp xoshiro256+x8.h prng.h
	gcc -O9 -fdisable-tree-cunrolli -march=native $(CCFLAGS) -o $@ prng.c
	rm prng.h

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

# This must be performed with no other processes running.
test/perf-$(FINGERPRINT): shishua
	@mkdir -p test
	@echo "Date $$(date)" | tee test/perf-$(FINGERPRINT)
	@echo "PRNG fingerprint: $(FINGERPRINT)" | tee -a test/perf-$(FINGERPRINT)
	./shishua --bytes 4294967296 2>&1 >/dev/null | tee -a test/perf-$(FINGERPRINT)

test/perf: shishua xoshiro256plusx8 romu
	@mkdir -p test
	@echo "Date $$(date)" | tee test/perf
	for prng in $^; do \
	  echo "$$prng fingerprint: $$(./$$prng | ./fingerprint.sh)" | tee -a test/perf; \
	  ./$$prng --bytes 4294967296 2>&1 >/dev/null | tee -a test/perf; \
	done
