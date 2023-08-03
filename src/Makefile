LDLIBS := -llink_core -lmiddleware_implementation -lplatform_port -lutils_modules -lblobmsg_json -lubox -lubus
INCLUDES := -I../
BIN := tuya_iot
SRCS := $(wildcard *.c)
OBJS := $(filter-out main.o, $(SRCS:.c=.o))
MAIN_OBJ := main.o

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS) $(MAIN_OBJ)
	$(CC) -o $@ $(OBJS) $(MAIN_OBJ) $(LDLIBS)

%.o: %.c
	$(CC) -c $(INCLUDES) $< -o $@

$(MAIN_OBJ): main.c
	$(CC) -c $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJS) $(MAIN_OBJ) $(BIN)
