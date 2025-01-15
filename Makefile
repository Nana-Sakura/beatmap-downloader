ROOT_DIR := .
INC_DIR := $(ROOT_DIR)/inc
SRC_DIR := $(ROOT_DIR)/src

OS := $(shell uname -s)
ifeq ($(OS), Darwin)
	UV_INC_DIR := /opt/homebrew/include
	UV_LIB_DIR := /opt/homebrew/lib
endif

BIN := $(ROOT_DIR)/Downloader
CC := gcc
IFLAGS := $(addprefix -I, $(shell find $(INC_DIR) -type d))
IFLAGS += $(addprefix -I, $(INC_DIR))
IFLAGS += $(addprefix -I, $(UV_INC_DIR))
LFLAGS := $(addprefix -L, $(UV_LIB_DIR))
LDFLAGS := $(addprefix -l, curl)
LDFLAGS += $(addprefix -l, uv)
CSRCS := $(shell find $(SRC_DIR) -name "*.c")

all: $(BIN)
# Prerequisites is BIN so that BIN has to be compiled first

$(BIN): $(CSRCS)
	$(CC) $(CSRCS) $(IFLAGS) $(LFLAGS) $(LDFLAGS) -o $(BIN)  -Wno-deprecated-declarations -Wno-format
# -fsanitize=address
run: $(BIN)
	@$^

clean:
	rm -rf $(BIN)