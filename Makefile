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
PYOBJS = $(OBJS) $(PYSRCS:%.c=%.o)

all: $(PROJ) py$(PROJ)
	@echo "Done"

em: clean
	@make CROSS=emulate

$(PROJ): $(OBJS)
	@echo "Linking: \033[0;32m$@\033[0m"
	@$(LD) $(OBJS) $(LFLAGS) $(OFLAGS) -o $@

py$(PROJ): $(PYOBJS)
	@echo "Linking: \033[0;32m$@\033[0m"
	@$(LD) -lpython2.7 $(PYOBJS) $(LFLAGS) -shared $(OFLAGS) -o $@/$@.so

%.o: %.c
	@echo "Compiling \033[0;31m$<\033[0m"
	@$(CC) $(CFLAGS) $(DFLAGS) $(WFLAGS) $(OFLAGS) -c $< -o $@

clean: clean_objects
	@rm -f $(PROJ)
	@echo "Clean"

clean_objects:
	@rm -f $(OBJS) $(PYOBJS) py$(PROJ)/py$(PROJ).so $(PROJ) *.pyc
	@echo "Cleaning objects..."
