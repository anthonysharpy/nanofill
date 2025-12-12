# ==================== HOW TO USE ==================== #
#
# Release build: make pgo-gen -> make release
# Profile build: make pgo-gen -> make profile
# Normal build (not recommended): make
#
# NOTE: profile build may require some extra software.
#
# ==================================================== #


# ===== Configuration ===== #

SUPPRESSED_WARNINGS = -Wno-interference-size
COMPILER = g++-12
BINARY_NAME = trader
SRC_DIR = src
BUILD_DIR = build
BASE_COMPILE_FLAGS = -DNDEBUG -std=c++23 -march=native -flto=auto -Ofast -Wall -Wextra -Wpedantic -pipe -MMD -MP -I$(SRC_DIR) $(SUPPRESSED_WARNINGS)
BASE_LINK_FLAGS = -flto
# Use -Ofast optimisation. More risky, but should work assuming the code is correct.
RELEASE_COMPILE_FLAGS = $(BASE_COMPILE_FLAGS) -fprofile-use=pgodata -fprofile-correction
RELEASE_LINK_FLAGS = $(BASE_LINK_FLAGS) -fprofile-use=pgodata
PROFILE_COMPILE_FLAGS  = $(BASE_COMPILE_FLAGS) -g -fno-omit-frame-pointer -fprofile-use=pgodata -fprofile-correction
PROFILE_LINK_FLAGS = $(BASE_LINK_FLAGS) -fprofile-use=pgodata
PGO_COMPILE_FLAGS = $(BASE_COMPILE_FLAGS) -fprofile-generate=pgodata
PGO_LINK_FLAGS = $(BASE_LINK_FLAGS) -fprofile-generate=pgodata

# ===== Vars ===== #

CPP_FILES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
DEPENDENCY_FILES = $(OBJ_FILES:.o=.d)
COMPILE_FLAGS = $(BASE_COMPILE_FLAGS)
LINK_FLAGS = $(BASE_LINK_FLAGS)

# ===== Build ===== #

.PHONY: clean profile pgo-gen release

all: $(BINARY_NAME)

$(BINARY_NAME): $(OBJ_FILES)
	$(COMPILER) $(OBJ_FILES) -o $@ $(LINK_FLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(COMPILE_FLAGS)  -c $< -o $@

-include $(DEPENDENCY_FILES)

# ===== Build (Release) ===== #

release:
	$(MAKE) clean
	$(MAKE) all COMPILE_FLAGS="$(RELEASE_COMPILE_FLAGS)" LINK_FLAGS="$(RELEASE_LINK_FLAGS)"

# ===== Performance-Guided Optimisation ===== #

pgo-gen: clean
	rm -rf pgodata
	$(MAKE) all COMPILE_FLAGS="$(PGO_COMPILE_FLAGS)" LINK_FLAGS="$(PGO_LINK_FLAGS)"
	./trader
	$(MAKE) clean

# ===== Profile ===== #

profile: clean
	$(MAKE) all COMPILE_FLAGS="$(PROFILE_COMPILE_FLAGS)" LINK_FLAGS="$(PROFILE_LINK_FLAGS)"
	sudo perf record -F 16000 -g -- ./$(BINARY_NAME)
	sudo hotspot perf.data

# ===== Clean ===== #

clean:
	rm -rf $(BUILD_DIR) $(BINARY_NAME)

