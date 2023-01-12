include config.mk

# Sub systems
MODULES = source

.PHONY: all clean $(MODULES) runtest $(TESTS)
all : $(MODULES)

# Sub sustems deps
main : machine

run : source
	rlwrap bin/source/main

test: source
	bin/source/test

# Gen modules
$(MODULES) :
	@echo
	@echo ==== MAKING $@ ====
	@$(MAKE) -C $@ \
		CURMODULE=$@ \
		--no-print-directory \
		-E 'include $(PWD)/config.mk'

# Cleaning
clean :
	rm -rf $(BIN)
