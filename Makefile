include prelude.mk

src_dir     := $(abspath .)
build_dir   := $(src_dir)/build
cmake_dir   := $(build_dir)/cmake
install_dir := $(build_dir)/install

test_report_dir  := $(build_dir)/test_report
coverage_dir     := $(build_dir)/coverage
flame_graphs_dir := $(build_dir)/flame_graphs

COMPILER       ?= clang
CONFIGURATION  ?= Debug
DEFAULT_HOST   ?= 127.0.0.1
DEFAULT_PORT   ?= 5556
INSTALL_PREFIX ?= $(install_dir)
COVERAGE       ?=

$(eval $(call noexpand,COMPILER))
$(eval $(call noexpand,CONFIGURATION))
$(eval $(call noexpand,DEFAULT_HOST))
$(eval $(call noexpand,DEFAULT_PORT))
$(eval $(call noexpand,INSTALL_PREFIX))
$(eval $(call noexpand,COVERAGE))

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
		-D 'TEST_REPORT_DIR=$(call escape,$(test_report_dir))' \
		-D 'COVERAGE=$(call escape,$(COVERAGE))' \
		-D 'FLAME_GRAPHS_DIR=$(call escape,$(flame_graphs_dir))' \
		-S '$(call escape,$(src_dir))' \
		-B '$(call escape,$(cmake_dir))'
	cmake --build '$(call escape,$(cmake_dir))' -- -j

.PHONY: release
release: CONFIGURATION := Release
release: build

.PHONY: install
install: build
	cmake --install '$(call escape,$(cmake_dir))'

.PHONY: test
test:
	@echo -----------------------------------------------------------------
	@echo Running tests
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_default

.PHONY: test/report
test/report:
	@echo -----------------------------------------------------------------
	@echo Generating test report
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_report

# A subset of tests, excluding long-running stress tests.
.PHONY: test/sanity
test/sanity:
	@echo -----------------------------------------------------------------
	@echo Running sanity tests
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_sanity

# Same, but run under Valgrind.
.PHONY: test/valgrind
test/valgrind:
	@echo -----------------------------------------------------------------
	@echo Running sanity tests w/ Valgrind
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_valgrind

# When building a Docker image for a different platform, exclude stress tests:
# they simply take way too long.
.PHONY: test/docker
test/docker: test/sanity

# Force a rebuild for a coverage report, since it depends on GCC debug data.
.PHONY: coverage
coverage: COMPILER      := gcc
coverage: CONFIGURATION := Debug
coverage: COVERAGE      := 1
coverage: clean build test
	@echo -----------------------------------------------------------------
	@echo Generating code coverage report
	@echo -----------------------------------------------------------------
	@mkdir -p -- '$(call escape,$(coverage_dir))'
	gcovr --html-details '$(call escape,$(coverage_dir))/index.html' --print-summary
ifndef CI
	xdg-open '$(call escape,$(coverage_dir))/index.html' &> /dev/null
endif

.PHONY: flame_graphs
flame_graphs:
	@echo -----------------------------------------------------------------
	@echo Generating flame graphs
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(cmake_dir))' \
		--verbose --tests-regex python_tests_flame_graphs
