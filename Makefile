LIBS = -lglfw -ldl -lvulkan
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Iinclude -Iinclude/lib/imgui -MMD -MP

SRC_DIR = src
OBJ_DIR = obj
BUILD_DIR = build

SRCS = $(shell find $(SRC_DIR) -name "*.cpp")
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

VERT_SHADERS = $(wildcard shaders/*.vert)
FRAG_SHADERS = $(wildcard shaders/*.frag)
ALL_SHADERS = $(VERT_SHADERS) $(FRAG_SHADERS)

TARGET = VoxelCraft

all: debug

debug: CXXFLAGS += -g
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

shader:
	@for file in $(ALL_SHADERS); do \
		filename=$$(basename "$$file"); \
		name="$${filename%.*}"; \
		echo "Compiling $$file -> shaders/$$name.spv"; \
		glslangValidator -V "$$file" -o "shaders/$$name.spv"; \
	done

run: all
	./$(TARGET)

.PHONY: all debug release clean run shader

-include $(OBJS:.o=.d)