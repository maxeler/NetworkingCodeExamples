
NAME=signext

IP1 ?= 172.16.50.1
TAP1_IP ?= 172.16.50.10
TAP1_NETMASK ?= 255.255.255.0
INIT_TAP1 = -e QSFP_TOP_10G_PORT1:$(TAP1_IP):$(TAP1_NETMASK)
SIM_NAME ?= $(USER)Sim

MAXFILE=SignExt.max
C_OBJS := signext.o  $(MAXFILE:.max=.o)

DFE_MODEL := $(shell perl -ne 'print $$1 if /ENGINE_PARAMETERS\(DFEModel, DFEMODEL, (.*?)\)/' $(MAXFILE))
DESIGN_NAME := $(shell grep MAXFILE_BUILD_NAME $(MAXFILE) | awk -F \" '{print $$2}')


ifndef MAXCOMPILERDIR
    $(error "You must specify the path to the directory containing MaxCompiler in the variable MAXCOMPILERDIR.")
endif


USE_SLIC := 1

include $(MAXCOMPILERDIR)/lib/Makefile.include

CFLAGS += -std=gnu99 -Wall -Werror -ggdb -rdynamic -DDESIGN_NAME=$(DESIGN_NAME)

all: $(NAME) sender

%.o: %.max
	-@echo $<" -> "$@
	+$(QUIET)$(SLICCOMPILE) $< $@
	
%.o: %.c $(MAXFILE)
	-@echo "[gcc] " $*.c" -> "$@
	$(QUIET)gcc $(MAXCOMPILER_INC) $(CFLAGS) -c -o $@ $*.c
	

sender: sender.o
	gcc sender.o -o sender 


sender.o: sender.c
	gcc sender.c -O2 -std=gnu99 -c -o sender.o


$(NAME): $(C_OBJS) $(MAXFILE)
	-@echo "Linking "$@
	gcc -o $(NAME) $(MAX_OBJS) $(C_OBJS) -rdynamic $(MAXCOMPILER_LIBS)

run_sim: $(NAME) restart_sim
	./signext $(IP1) $(TAP1_IP)

start_sim:
	maxcompilersim -n $(SIM_NAME) -c $(DFE_MODEL) $(INIT_TAP1) -p QSFP_TOP_10G_PORT1:top1.pcap  restart

stop_sim:
	maxcompilersim -n $(SIM_NAME) $(INIT_TAP1) $(INIT_TAP2) stop

restart_sim:
	maxcompilersim -n $(SIM_NAME) -c $(DFE_MODEL) $(INIT_TAP1) -p QSFP_TOP_10G_PORT1:top1.pcap  restart
	
sim_debug:
	maxdebug -g graph_$(SIM_NAME) -d $(SIM_NAME)0:$(SIM_NAME) $(MAXFILE)


clean:
	rm -f *.o *.pcap *.dot *.png
