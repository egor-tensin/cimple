MAKEFLAGS += --no-builtin-rules --no-builtin-variables --warn-undefined-variables
unexport MAKEFLAGS
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.SUFFIXES:
SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c

escape = $(subst ','\'',$(1))

define noexpand
ifeq ($$(origin $(1)),environment)
    $(1) := $$(value $(1))
endif
ifeq ($$(origin $(1)),environment override)
    $(1) := $$(value $(1))
endif
ifeq ($$(origin $(1)),command line)
    override $(1) := $$(value $(1))
endif
endef

src_dir := $(abspath .)
build_dir := $(src_dir)/build
cmake_dir := $(build_dir)/cmake
install_dir := $(build_dir)/install

C_COMPILER ?= clang
BUILD_TYPE ?= Debug
DEFAULT_HOST ?= 127.0.0.1
INSTALL_PREFIX ?= $(install_dir)

$(eval $(call noexpand,C_COMPILER))
$(eval $(call noexpand,BUILD_TYPE))
$(eval $(call noexpand,DEFAULT_HOST))
$(eval $(call noexpand,INSTALL_PREFIX))

.PHONY: all
all: build

.PHONY: clean
clean:
	rm -rf -- '$(call escape,$(build_dir))'

.PHONY: build
build:
	mkdir -p -- '$(call escape,$(cmake_dir))'
	cmake \
		-G 'Unix Makefiles' \
		-D 'CMAKE_C_COMPILER=$(call escape,$(C_COMPILER))' \
		-D 'CMAKE_BUILD_TYPE=$(call escape,$(BUILD_TYPE))' \
		-D 'CMAKE_INSTALL_PREFIX=$(call escape,$(INSTALL_PREFIX))' \
		-D 'DEFAULT_HOST=$(call escape,$(DEFAULT_HOST))' \
		-S '$(call escape,$(src_dir))' \
		-B '$(call escape,$(cmake_dir))'
	cmake --build '$(call escape,$(cmake_dir))' -- -j

.PHONY: install
install: build
	cmake --install '$(call escape,$(cmake_dir))'

.PHONY: test
test:
	cd -- '$(call escape,$(cmake_dir))' && ctest --verbose
