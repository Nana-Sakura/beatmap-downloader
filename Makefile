SRC_DIR := $(shell pwd)
OS := $(shell uname -s)
if [[ $(OS) == "Darwin" ]]; then
UV_INC_DIR := /opt/homebrew/include
UV_LIB_DIR := /opt/homebrew/lib
fi

BIN := $(SRC_DIR)/Downloader
CC := gcc
IFLAGS := $(addprefix -I, $(SRC_DIR))
IFLAGS += $(addprefix -I, $(UV_INC_DIR))
LFLAGS := $(addprefix -L, $(UV_LIB_DIR))
LDFLAGS := $(addprefix -l, curl)
LDFLAGS += $(addprefix -l, uv)
CSRCS := $(shell find $(SRC_DIR) -name "*.c")

all: $(BIN)
# Prerequisites is BIN so that BIN has to be compiled first

$(BIN): $(CSRCS)
	$(CC) $(CSRCS) $(IFLAGS) $(LFLAGS) $(LDFLAGS) -o $(BIN) -fsanitize=address

run: $(BIN)
	@$^

clean:
	rm -rf $(BIN)