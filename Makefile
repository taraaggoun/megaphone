CC            := gcc
LDLIBS		  := -pthread -lreadline
CFLAGS        := -Iinclude -Wall -Wextra -pedantic          \
		 		 -Wshadow -Wpointer-arith -Wcast-align                \
	  	 		 -Wwrite-strings -Winline -Wno-long-long -Wconversion \
	     	 	 -Wmissing-declarations -Wredundant-decls

# -Werror

BIN           := bin
OBJ           := obj
SRC           := src

CLIENT        := client
SERVER        := server

TARGET_CLIENT := $(BIN)/$(CLIENT)
TARGET_SERVER := $(BIN)/$(SERVER)


SRCS_CLIENT   := $(shell find $(SRC) -type d -name $(SERVER) -prune -o -name '*.c' -print)
SRCS_SERVER   := $(shell find $(SRC) -type d -name $(CLIENT) -prune -o -name '*.c' -print)

OBJS_CLIENT   := $(patsubst $(SRC)/%, $(OBJ)/%, $(SRCS_CLIENT:.c=.o))
OBJS_SERVER   := $(patsubst $(SRC)/%, $(OBJ)/%, $(SRCS_SERVER:.c=.o))

RESSOURCES    := res

#--------------------------------------#

.PHONY: all clean

all: $(TARGET_SERVER) $(TARGET_CLIENT)

client: $(TARGET_CLIENT)

server: $(TARGET_SERVER)

debug: CFLAGS := -DDEBUG $(CFLAGS)
debug: $(TARGET_SERVER) $(TARGET_CLIENT)

client_memory: $(TARGET_CLIENT) $(OBJS_CLIENT)
	valgrind ./$(TARGET_CLIENT)

server_memory: $(TARGET_SERVER) $(OBJS_SERVER)
	valgrind ./$(TARGET_SERVER)


$(TARGET_CLIENT): $(OBJS_CLIENT)
	@mkdir -p $(RESSOURCES)/$(CLIENT)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)

$(TARGET_SERVER): $(OBJS_SERVER)
	@mkdir -p $(RESSOURCES)/$(SERVER)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDLIBS)


$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	$(RM) -r $(BIN) $(OBJ) $(RESSOURCES)

help :
	@echo "Usage:"
	@echo "    make [all]\t\t\tCompilation du client et du serveur"
	@echo "    make client\t\t\tCompilation du client"
	@echo "    make server\t\t\tCompilation du serveur"
	@echo "    make clean\t\t\tSupprime les .o et les executable"
	@echo "    make client_memory\t\tLance valgrind sur le client"
	@echo "    make server_memory\t\tLance valgrind sur le seveur"
	@echo "    make help\t\t\tAffichage d'aide"
