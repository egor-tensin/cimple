include prelude.mk

src_dir := $(abspath .)
build_dir := $(src_dir)/build
cmake_dir := $(build_dir)/cmake
install_dir := $(build_dir)/install

COMPILER ?= clang
CONFIGURATION ?= Debug
DEFAULT_HOST ?= 127.0.0.1
DEFAULT_PORT ?= 5556
INSTALL_PREFIX ?= $(install_dir)

$(eval $(call noexpand,COMPILER))
$(eval $(call noexpand,CONFIGURATION))
$(eval $(call noexpand,DEFAULT_HOST))
$(eval $(call noexpand,DEFAULT_PORT))
$(eval $(call noexpand,INSTALL_PREFIX))

.PHONY: all
all: build

.PHONY: clean
clean:
	rm -rf -- '$(call escape,$(build_dir))'

.PHONY: build
build:
	@echo -----------------------------------------------------------------
	@echo Building
	@echo -----------------------------------------------------------------
	@mkdir -p -- '$(call escape,$(cmake_dir))'
	cmake \
		-G 'Unix Makefiles' \
		-D 'CMAKE_C_COMPILER=$(call escape,$(COMPILER))' \
		-D 'CMAKE_BUILD_TYPE=$(call escape,$(CONFIGURATION))' \
		-D 'CMAKE_INSTALL_PREFIX=$(call escape,$(INSTALL_PREFIX))' \
		-D 'DEFAULT_HOST=$(call escape,$(DEFAULT_HOST))' \
		-D 'DEFAULT_PORT=$(call escape,$(DEFAULT_PORT))' \
		-S '$(call escape,$(src_dir))' \
		-B '$(call escape,$(cmake_dir))'
	cmake --build '$(call escape,$(cmake_dir))' -- -j

.PHONY: install
install: build
	cmake --install '$(call escape,$(cmake_dir))'

.PHONY: test
test:
	@echo -----------------------------------------------------------------
	@echo Running tests
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --exclude-regex python_tests_valgrind

.PHONY: test/valgrind
test/valgrind:
	@echo -----------------------------------------------------------------
	@echo Running tests w/ Valgrind
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_valgrind

.PHONY: test/all
test/all: test test/valgrind
