.PHONY: build install test clean debug release format lint

BUILD_DIR = build

build:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc)

debug:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc)

install: build
	cmake --install $(BUILD_DIR) --prefix /usr/local

test:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --target calendly_tests -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc)
	cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

format:
	find src tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i

lint:
	cmake -B $(BUILD_DIR) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	find src -name '*.cpp' | xargs clang-tidy -p $(BUILD_DIR)
