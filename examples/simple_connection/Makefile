##
## Author: Mickaël BLET
##

MAKEFLAG	=	--no-print-directory

BUILD_DIR	=	./

MODULE		=	\
				client \
				server \
				../../udtcp

all:		MODE=debug
all:		LIB_MODE=lib_debug
all:		$(MODULE)

help:
	@echo "Makefile options:"
	@echo
	@echo "all      #launch default (depend default mode) module rules."
	@echo "debug    #launch debug module rules."
	@echo "release  #launch release module rules."
	@echo "test     #launch test module rules."
	@echo "exe_test #launch exe_test module rules."
	@echo "clean    #launch clean module rules."
	@echo "fclean   #launch fclean module rules."
	@echo "re       #launch fclean and all module rules."
	@echo
	@$(foreach module,$(MODULE),printf "%-20s #launch default rule for $(module) module\n" "$(module)";)
	@echo

#------------------------------------------------------------------------------

../../udtcp:
	$(MAKE) $(MAKEFLAG) $(LIB_MODE) -C $(BUILD_DIR)$@
client: ../../udtcp
	$(MAKE) $(MAKEFLAG) $(MODE) -C $(BUILD_DIR)$@
server:	../../udtcp
	$(MAKE) $(MAKEFLAG) $(MODE) -C $(BUILD_DIR)$@

#------------------------------------------------------------------------------

debug:		MODE=debug
debug:		LIB_MODE=lib_debug
debug:		$(MODULE)

release:	MODE=release
release:	LIB_MODE=lib_release
release:	$(MODULE)

test:		MODE=test
test:		LIB_MODE=test
test:		$(MODULE)

exe_test:	MODE=exe_test
exe_test:	LIB_MODE=exe_test
exe_test:	$(MODULE)

clean:		MODE=clean
clean:		LIB_MODE=clean
clean:		$(MODULE)

fclean:		MODE=fclean
fclean:		LIB_MODE=fclean
fclean:		$(MODULE)

re:			fclean
	$(MAKE) $(MAKEFLAG) all

.PHONY:		all help debug release test clean fclean re $(MODULE)