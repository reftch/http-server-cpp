.SILENT:

# ==================================================
# --- Configuration ---
# ==================================================
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3  # Compiler flags: C++17 standard, warnings, optimization
TARGET_DIR = build
TARGET = $(TARGET_DIR)/server
SRC_DIR = src

# --- Source Files Declaration ---
# Use wildcard to automatically find all .cpp files in the src directory
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# --- Targets ---

# Default target: Build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): build $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Target to clean up the build directory and object files
clean:
	@echo "Cleaning build files..."
	rm -rf $(TARGET_DIR)

# Optional: A target to get the build directory structure
build:
	@mkdir -p $(TARGET_DIR)

# --- Optional: Make it easier to run ---
run: $(TARGET)
	./$(TARGET)