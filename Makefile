SHELL = bash
CCFLAGS =
FINGERPRINT = $(shell ./shishua | ./fingerprint.sh)
# Please add this list to .gitignore when modifying this line.
PRNGS = shishua shishua-half chacha8 xoshiro256plusx8 xoshiro256plus romu wyrand lehmer128 rc4
SSH_KEY = ~/.ssh/id_ed25519

shishua: shishua.h prng.c
	cp shishua.h prng.h
	gcc -O9 -mavx2 $(CCFLAGS) -o $@ prng.c
	rm prng.h

shishua-half: shishua-half.h prng.c
	cp shishua-half.h prng.h
	gcc -O9 -mavx2 $(CCFLAGS) -o $@ prng.c
	rm prng.h

chacha8: chacha8.h prng.c
	cp chacha8.h prng.h
	gcc -O9 -mavx2 $(CCFLAGS) -o $@ prng.c
	rm prng.h

xoshiro256plusx8: xoshiro256+x8.h prng.c
	cp xoshiro256+x8.h prng.h
	gcc -O9 -fdisable-tree-cunrolli -march=native $(CCFLAGS) -o $@ prng.c
	rm prng.h

xoshiro256plus: xoshiro256+.h prng.c
	cp xoshiro256+.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

romu: romu.h prng.c
	cp romu.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

wyrand: wyrand.h prng.c
	cp wyrand.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

lehmer128: lehmer128.h prng.c
	cp lehmer128.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

rc4: rc4.h prng.c
	cp rc4.h prng.h
	gcc -O9 $(CCFLAGS) -o $@ prng.c
	rm prng.h

intertwine: intertwine.c
	gcc -o $@ $<

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

test/seed: $(PRNGS) intertwine
	@mkdir -p test
	@echo "Date $$(date)" | tee test/seed
	for prng in $(PRNGS); do \
	  echo "$$prng fingerprint: $$(./$$prng | ./fingerprint.sh)" | tee -a test/seed; \
	  ./intertwine <(./$$prng -s 0) <(./$$prng -s 1) \
	               <(./$$prng -s 2) <(./$$prng -s 4) \
	               <(./$$prng -s 8) <(./$$prng -s 10) \
	               <(./$$prng -s 20) <(./$$prng -s 40) \
	    | RNG_test stdin -tlmax 1M -tlmin 1K -te 1 -tf 2 | tee -a test/seed; \
	done

##
## Performance testing.
##

# This must be performed with no other processes running.

test/perf-$(FINGERPRINT): shishua
	@mkdir -p test
	@echo "Date $$(date)" | tee test/perf-$(FINGERPRINT)
	@echo "PRNG fingerprint: $(FINGERPRINT)" | tee -a test/perf-$(FINGERPRINT)
	./shishua --bytes 4294967296 2>&1 >/dev/null | tee -a test/perf-$(FINGERPRINT)

test/perf: $(PRNGS)
	@mkdir -p test
	@echo "Date $$(date)" | tee test/perf
	for prng in $(PRNGS); do \
	  echo "$$prng fingerprint: $$(./$$prng | ./fingerprint.sh)" | tee -a test/perf; \
	  ./fix-cpu-freq.sh ./$$prng --bytes 4294967296 2>&1 >/dev/null | tee -a test/perf; \
	done

# To reach a consistent benchmark, we need a universally-reproducible system.
# GCP will do.

# Installation instructions from https://cloud.google.com/sdk/docs/downloads-apt-get
/usr/bin/gcloud:
	echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | sudo tee -a /etc/apt/sources.list.d/google-cloud-sdk.list
	sudo apt-get install apt-transport-https ca-certificates gnupg
	curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key --keyring /usr/share/keyrings/cloud.google.gpg add -
	sudo apt-get update && sudo apt-get install google-cloud-sdk
	gcloud init

test/gcp-intel-perf: /usr/bin/gcloud
	gcloud compute instances create shishua-intel \
	  --machine-type=n2-standard-2 \
	  --image-project=ubuntu-os-cloud --image-family=ubuntu-1910 \
	  --zone=us-central1-f \
	  --maintenance-policy=TERMINATE
	tar cJf shishua.tar.xz $$(git ls-files)
	gcloud compute scp ./shishua.tar.xz shishua-intel:~ --ssh-key-file=$(SSH_KEY)
	rm shishua.tar.xz
	gcloud compute ssh shishua-intel --ssh-key-file=$(SSH_KEY) -- 'tar xJf shishua.tar.xz && ./gcp-perf.sh'
	gcloud compute instances delete shishua-intel

# We must use us-central1 to have access to N2D, with the new AMD CPUs.
test/gcp-amd-perf: /usr/bin/gcloud
	gcloud compute instances create shishua-amd \
	  --machine-type=n2d-standard-2 \
	  --image-project=ubuntu-os-cloud --image-family=ubuntu-1910 \
	  --zone=us-central1-f \
	  --maintenance-policy=TERMINATE
	tar cJf shishua.tar.xz $$(git ls-files)
	gcloud compute scp ./shishua.tar.xz shishua-amd:~ --ssh-key-file=$(SSH_KEY)
	rm shishua.tar.xz
	gcloud compute ssh shishua-amd --ssh-key-file=$(SSH_KEY) -- 'tar xJf shishua.tar.xz && ./gcp-perf.sh'
	gcloud compute instances delete shishua-amd
