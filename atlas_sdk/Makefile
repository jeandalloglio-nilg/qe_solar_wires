ATLAS_SDK_HOME ?= /home/ubuntu/flir_atlas_c # Raiz do SDK Atlas C

CC := gcc
CFLAGS := -O2 -Wall -I$(ATLAS_SDK_HOME)/include
LDFLAGS := -L$(ATLAS_SDK_HOME)/lib -latlas_c_sdk -lpthread -ldl -lm -Wl,-rpath,'$(ATLAS_SDK_HOME)/lib'

SRC_DIR := src
BUILD_DIR := build

BIN1 := $(BUILD_DIR)/atlas_scale_set
BIN2 := $(BUILD_DIR)/check_settings

all: $(BIN1) $(BIN2)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN1): $(SRC_DIR)/atlas_scale_set.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN2): $(SRC_DIR)/check_settings.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
