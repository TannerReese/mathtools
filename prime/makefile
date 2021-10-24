CC=gcc
CFLAGS=
directories=bin
targets=primes
libs=m

bin/primes: primes_main.o primes.o
primes_main.o: primes_main.c primes.h bit_array.h
primes.o: primes.c primes.h bit_array.h



# Provide simple target names for binaries
$(targets): %: dirs bin/%

# Make sure directories exist
dirs:
	@mkdir -p $(directories)

# Compile binaries into bin folder
bin/%:
	$(CC) $(CFLAGS) -o $@ $(addprefix -l,$(libs)) $(filter %.o,$^)

%.o:
	$(CC) -c $(CFLAGS) -o $@ $(filter %.c,$^)


clean:
	@echo Removing object files
	@rm -f *.o
	@echo Removing binaries: $(target)
	@cd bin && rm -f $(targets) && cd ..

.PHONY: clean dirs $(targets)

