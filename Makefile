#
# File:
#    Makefile
#
# Description:
#    Makefile for the et_producer
#
# SVN: $Rev$
#
# Uncomment DEBUG line, to include some debugging info ( -g and -Wall)
#
#

CODA	?= /daqfs/daq_setups/coda/3.10_devel

ET_LIB	?= ${CODA}/Linux-x86_64/lib
ET_INC	?= ${CODA}/Linux-x86_64/include

EVIO_LIB ?= ${CODA}/Linux-x86_64/lib
EVIO_INC ?= ${CODA}/Linux-x86_64/include

#

# Uncomment DEBUG line, to include some debugging info ( -g and -Wall)
DEBUG	?= 1
QUIET	?= 1
#
ifeq ($(QUIET),1)
        Q = @
else
        Q =
endif

CC			= g++
AR                      = ar
RANLIB                  = ranlib
CFLAGS			= -std=c++11 -L. -L${ET_LIB} -let -lpthread -ldl -levio
INCS			= -I. -I${CODA}/common/include -I${ET_INC} -I${EVIO_INC}


ifdef DEBUG
CFLAGS			+= -Wall -Wno-unused -g
else
CFLAGS			+= -O2
endif

SRC			= et_consumer.c
DEPS			= $(SRC:.c=.d)
PROG			= $(SRC:.c=)

all: ${PROG}

%: %.c
	@echo " CC     $@"
	${Q}$(CC) $(CFLAGS) $(INCS) -o $@ $<

%.d: %.c
	@echo " DEP    $@"
	${Q}set -e; rm -f $@; \
	$(CC) -MM -shared $(INCS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS)

clean:
	@rm -vf ${OBJ} ${DEPS} ${LIBS} ${DEPS}.* *~ ${PROG}

.PHONY: clean
