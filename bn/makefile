CC=gcc
CFLAGS=

# Recipes for main library files
bn.o: bn.c bn.h


# Recipe for object files
%.o:
	$(CC) -c $(CFLAGS) -o $@ $(filter %.c,$^)

# Remove binary and object files
clean:
	@echo Removing object files
	@rm -f *.o test/*.o

.PHONY: clean

