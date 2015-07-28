NAME=cme

PORT := 5001
DFE_TOP_IP := 169.254.10.1
DFE_BOT_IP := 169.254.11.1
SIM_TOP_IP := 169.254.10.2
SIM_BOT_IP := 169.254.11.2
NETMASK    := 255.255.255.0
INIT_TOP = -e QSFP_TOP_10G_PORT1:$(SIM_TOP_IP):$(NETMASK)
INIT_BOT = -e QSFP_BOT_10G_PORT1:$(SIM_BOT_IP):$(NETMASK)

SIM_NAME ?= $(USER)Sim

MAXFILE=CmeTrading.max
C_OBJS := $(NAME).o  $(MAXFILE:.max=.o)

DFE_MODEL := $(shell perl -ne 'print $$1 if /ENGINE_PARAMETERS\(DFEModel, DFEMODEL, (.*?)\)/' $(MAXFILE))


ifndef MAXCOMPILERDIR
    $(error "You must specify the path to the directory containing MaxCompiler in the variable MAXCOMPILERDIR.")
endif

ifndef MAXMPTDIR
    $(error "You must specify the path to the directory containing MaxMPT in the variable MAXMPTDIR.")
endif


USE_SLIC := 1
include $(MAXCOMPILERDIR)/lib/Makefile.include

CFLAGS := -std=gnu99 -Wall -Werror -ggdb $(MAXCOMPILER_INC) -I$(MAXMPTDIR)/include
LDLIBS := $(MAXCOMPILER_LIBS) -L$(MAXMPTDIR)/lib -lmaxmpt_cme_globex_ilink -lmaxmpt_cme_globex_marketdata -lmaxmpt_core

all: $(NAME)

%.o: %.max
	-@echo $<" -> "$@
	+$(QUIET)$(SLICCOMPILE) $< $@
	
%.o: %.c
	-@echo "[cc] " $<" -> "$@
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@


$(NAME): $(C_OBJS)
	-@echo "Linking "$@
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

run_sim: $(NAME) restart_sim
	./$(NAME) $(DFE_TOP_IP):$(NETMASK) $(DFE_BOT_IP):$(NETMASK) $(SIM_TOP_IP):$(PORT)

start_sim:
	maxcompilersim -n $(SIM_NAME) -c $(DFE_MODEL) $(INIT_TOP) $(INIT_BOT) -p QSFP_TOP_10G_PORT1:top1.pcap restart

stop_sim:
	maxcompilersim -n $(SIM_NAME) stop

restart_sim:
	maxcompilersim -n $(SIM_NAME) -c $(DFE_MODEL) $(INIT_TOP) $(INIT_BOT) -p QSFP_TOP_10G_PORT1:top1.pcap  restart
	
sim_debug:
	maxdebug -g graph_$(SIM_NAME) -d $(SIM_NAME)0:$(SIM_NAME) $(MAXFILE)


clean:
	rm -f *.o *.pcap *.dot *.png
