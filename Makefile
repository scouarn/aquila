include config.mk

# Sub systems
MODULES = main emul socktest

.PHONY: all clean $(MODULES) runtest $(TESTS)
all : $(MODULES)

# Sub sustems deps
main : emul

# Commands / shortcuts
run : main
	rlwrap bin/main/main

test: emul
	bin/emul/test

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
