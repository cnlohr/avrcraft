include user.cfg
-include esp82xx/common.mf
-include esp82xx/main.mf

% :
	$(warning This is the empty rule. Something went wrong.)
	@true

ifndef TARGET
$(info Modules were not checked out... use git clone --recursive in the future. Pulling now.)
$(shell git submodule update --init --recursive 2>/dev/null)
endif

CFLAGS += -I../libs -I../dumbcraft -DHAS_UINT32TO10STR
SRCS += ../dumbcraft/dumbcraft.c \
	../dumbcraft/dumbutils.c \
	../libs/util10.c \
	user/dumbgame.c

# Example for a custom rule.
# Most of the build is handled in main.mf
.PHONY : showvars
showvars:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))
	true

