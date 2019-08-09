BINARY = ifstat

SOURCES = \
	ifstat.c \

CFLAGS += \
	-Wall \
	-Wextra \
	-Werror \
	-O2 \
	-g \

all: ${BINARY}
	./$^

${BINARY}: ${SOURCES:.c=.o}
	${LINK.c} -o $@ ${LDFLAGS} $<

clean:
	@-rm ${BINARY} ${SOURCES:.c=.o}
