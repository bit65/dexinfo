PROJ = dexinfo
SRCS = dexinfo.c
PYSRCS = pydexinfo.c

CFLAGS=-fstack-protector-all -fPIC -fno-exceptions -s # -O3
WFLAGS=-Wall
LFLAGS=-Wl,-z,relro,-z,now
OFLAGS=-pipe

CC=${CROSS_COMPILE}gcc
LD=${CROSS_COMPILE}gcc
OBJS = $(SRCS:%.c=%.o)

ifeq ($(PYDEXINFO),true)

DEFINES=-DPYDEXINFO
LFLAGS+=-lpython2.7 -shared
OUTPUT=py$(PROJ)/py
PYOBJS = $(PYSRCS:%.c=%.o)
EXT=.so

else

DEFINES=
OUTPUT=

endif

.PHONY: py$(PROJ) clean


all: clean $(PROJ) py$(PROJ)
	@echo "Done"

$(PROJ): $(OBJS) $(PYOBJS)
	@echo "Linking: \033[0;32m$(OUTPUT)$@\033[0m"
	@$(LD) $(OBJS) $(PYOBJS) $(LFLAGS) $(OFLAGS) -o $(OUTPUT)$@$(EXT)

py$(PROJ):
	@# Force rebuilding of non-python files
	@touch $(SRCS)
	@make PYDEXINFO=true $(PROJ)

%.o: %.c
	@echo "Compiling \033[0;31m$<\033[0m"
	@$(CC) $(CFLAGS) $(DEFINES) $(DFLAGS) $(WFLAGS) $(OFLAGS) -c $< -o $@

clean: clean_objects
	@rm -f $(PROJ)
	@echo "Clean"

clean_objects:
	@rm -f *.o py$(PROJ)/py$(PROJ).so $(PROJ) *.pyc
	@echo "Cleaning objects..."
