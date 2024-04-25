include prelude.mk

COMPILER      ?= clang
CONFIGURATION ?= debug
DEFAULT_HOST  ?= 127.0.0.1
DEFAULT_PORT  ?= 5556
COVERAGE      ?=

$(eval $(call noexpand,COMPILER))
$(eval $(call noexpand,CONFIGURATION))
$(eval $(call noexpand,DEFAULT_HOST))
$(eval $(call noexpand,DEFAULT_PORT))
$(eval $(call noexpand,COVERAGE))

src_dir   := $(abspath .)
build_dir := $(src_dir)/build

.PHONY: all
all: debug

.PHONY: DO
DO:

.PHONY: clean
clean:
	rm -rf -- '$(call escape,$(build_dir))'

.PHONY: build
build:
	@echo -----------------------------------------------------------------
	@echo 'Building: $(call escape,$(CONFIGURATION))'
	@echo -----------------------------------------------------------------
	@mkdir -p -- '$(call escape,$(build_dir)/cmake)'
	cmake \
		-G 'Unix Makefiles' \
		-D 'CMAKE_C_COMPILER=$(call escape,$(COMPILER))' \
		-D 'CMAKE_BUILD_TYPE=$(call escape,$(CONFIGURATION))' \
		-D 'CMAKE_INSTALL_PREFIX=$(call escape,$(build_dir)/install)' \
		-D 'DEFAULT_HOST=$(call escape,$(DEFAULT_HOST))' \
		-D 'DEFAULT_PORT=$(call escape,$(DEFAULT_PORT))' \
		-D 'TEST_REPORT_DIR=$(call escape,$(build_dir)/test_report)' \
		-D 'COVERAGE=$(call escape,$(COVERAGE))' \
		-D 'FLAME_GRAPHS_DIR=$(call escape,$(build_dir)/flame_graphs)' \
		-S '$(call escape,$(src_dir))' \
		-B '$(call escape,$(build_dir)/cmake)'
	cmake --build '$(call escape,$(build_dir)/cmake)' -- -j

.PHONY: debug
debug debug/%: CONFIGURATION := debug
debug debug/%: build_dir     := $(build_dir)/debug
debug: build

.PHONY: release
release release/%: CONFIGURATION := release
release release/%: build_dir     := $(build_dir)/release
release: build

# Coverage report depends on GCC debug data.
.PHONY: coverage
coverage coverage/%: CONFIGURATION := debug
coverage coverage/%: COMPILER      := gcc
coverage coverage/%: COVERAGE      := 1
coverage coverage/%: build_dir     := $(build_dir)/coverage
coverage: build coverage/test
	@echo -----------------------------------------------------------------
	@echo Generating code coverage report
	@echo -----------------------------------------------------------------
	@mkdir -p -- '$(call escape,$(build_dir))/html'
	gcovr --html-details '$(call escape,$(build_dir))/html/index.html' --print-summary
ifndef CI
	xdg-open '$(call escape,$(build_dir))/html/index.html' &> /dev/null
endif

%/install: % DO
	cmake --install '$(call escape,$(build_dir))/cmake'

%/test: DO
	@echo -----------------------------------------------------------------
	@echo Running tests
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(build_dir))/cmake' \
		--verbose --tests-regex python_tests_default

%/report: DO
	@echo -----------------------------------------------------------------
	@echo Generating test report
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(build_dir))/cmake' \
		--verbose --tests-regex python_tests_report
ifndef CI
	xdg-open '$(call escape,$(build_dir))/test_report/index.html' &> /dev/null
endif

# A subset of tests, excluding long-running stress tests.
%/sanity: DO
	@echo -----------------------------------------------------------------
	@echo Running sanity tests
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(build_dir))/cmake' \
		--verbose --tests-regex python_tests_sanity

# Same, but run under Valgrind.
%/valgrind: DO
	@echo -----------------------------------------------------------------
	@echo Running sanity tests w/ Valgrind
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(build_dir))/cmake' \
		--verbose --tests-regex python_tests_valgrind

# When building a Docker image for a different platform, exclude stress tests:
# they simply take way too long.

%/flame_graphs: DO
	@echo -----------------------------------------------------------------
	@echo Generating flame graphs
	@echo -----------------------------------------------------------------
	ctest --test-dir '$(call escape,$(build_dir))/cmake' \
		--verbose --tests-regex python_tests_flame_graphs
