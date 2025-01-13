SRC_DIR := $(shell pwd)
BIN := $(SRC_DIR)/Downloader
CC := gcc
IFLAGS := $(addprefix -I, $(SRC_DIR))
LDFLAGS := $(addprefix -l, curl)
CSRCS := $(shell find $(SRC_DIR) -name "*.c")

all: $(BIN)
# Prerequisites is BIN so that BIN has to be compiled first

$(BIN): $(CSRCS)
	$(CC) $(CSRCS) $(IFLAGS) $(LDFLAGS) -o $(BIN)

run: $(BIN)
	@$^

clean:
	rm -rf $(BIN)