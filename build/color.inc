#color define

NC		= "\033[0m"
BRED		= "\033[1;31m"
BGREEN		= "\033[1;32m"
BYELLOW		= "\033[1;33m"
BBLUE		= "\033[1;34m"
BMAGENTA	= "\033[1;35m"
BCYAN		= "\033[1;36m"
BWHITE		= "\033[1;37m"

RED		= "\033[31m"
GREEN		= "\033[32m"
YELLOW		= "\033[33m"
BLUE		= "\033[34m"
MAGENTA		= "\033[35m"
CYAN		= "\033[36m"
WHITE		= "\033[37m"


#COLOR	0: disable color; 1: enable color
COLOR	= 1

CC_V	= $(CC_$(COLOR))
CXX_V	= $(CXX_$(COLOR))
LD_V	= $(LD_$(COLOR))
AR_V	= $(AR_$(COLOR))
CP_V	= $(CP_$(COLOR))
RM_V	= $(RM_$(COLOR))
CC_0	= $(CC)
CC_1	= @printf '%b\t%b\t%b\n' $(CYAN)$(ARCH) CC $@$(NC); $(CC)
CXX_0	= $(CXX)
CXX_1	= @printf '%b\t%b\t%b\n' $(CYAN)$(ARCH) CXX $@$(NC); $(CXX)
LD_0	= $(LD)
LD_1	= @printf '%b\t%b\t%b\n' $(BGREEN)$(ARCH) LD $@$(NC); $(LD)
AR_0	= $(AR)
AR_1	= @printf '%b\t%b\t%b\n' $(YELLOW)$(ARCH) AR $@$(NC); $(AR)
CP_0	= cp
CP_1	= @printf '%b\t%b\n' $(BGREEN)$(ARCH) install$(NC); cp
RM_0	= rm
RM_1	= @printf '%b\t%b\n' $(BGREEN)$(ARCH) clean$(NC); rm


