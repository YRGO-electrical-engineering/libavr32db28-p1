# Cross-compile the AVR32DB28 firmware.
build:
	@bash ci/build.sh

# Build and run the host test suite against the mocked hardware platform.
test:
	@bash ci/test.sh

# Format all C/C++ files in place.
format:
	@bash ci/format.sh

# Check formatting without modifying any files; fails if something isn't formatted.
format-check:
	@bash ci/format.sh --check

# Remove all build artifacts.
clean:
	@rm -rf build
	@$(MAKE) -C test clean

.PHONY: build test format format-check clean
