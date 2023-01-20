include config.mk

# Sub systems
MODULES = main emul socktest avr

.PHONY: all clean upload $(MODULES) $(TESTS)
all : $(MODULES)

# Make submodule $1 with rule $2
define make_module
	$(MAKE) -C $1 $2 \
		CURMODULE=$1 \
		--no-print-directory \
		-E 'include $(PWD)/config.mk'
endef

# Sub sustems deps
main: emul

# Commands / shortcuts
run: main
	rlwrap bin/main/main

upload:
	@$(call make_module,avr,upload)

test: emul
	bin/emul/test

# Gen modules
$(MODULES) :
	@echo
	@echo ==== MAKING $@ ====
	@$(call make_module,$@,)

# Cleaning
clean :
	rm -rf $(BIN)
