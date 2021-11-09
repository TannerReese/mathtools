CC=gcc
CFLAGS=
libs=m
# Test binaries
test_bins=bn_test
# Perform test by calling run_<test_bin>
tests=$(addprefix run_,$(test_bins))


# Recipe for tester
bn_test: bn_test.o bn.o
bn_test.o: bn_test.c bn.h

# Recipes for main library files
bn.o: bn.c bn.h


# Run all tests
test: $(tests)
# Run single test
$(tests): run_%: %
	./$<

# Recipe to link test binaries
$(test_bins): %:
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(addprefix -l,$(libs))

# Recipe for object files
%.o:
	$(CC) -c $(CFLAGS) -o $@ $(filter %.c,$^)

# Remove binary and object files
clean:
	@echo Removing object files
	rm -f *.o
	@echo Remove test binary
	rm -f $(bins)

.PHONY: clean test $(tests)

