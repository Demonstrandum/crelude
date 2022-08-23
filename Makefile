FIND_CC ?= $(shell \
  if   cc --version | grep -qi gcc;   then echo gcc; \
  elif cc --version | grep -qi clang; then echo clang; \
  else echo cc; \
  fi \
)

ifeq ($(CC),cc)
CC := $(FIND_CC)
endif

ARCH ?= amd64

TARGET ?= tests
TARGET_LIB ?= libcrelude.so
TARGET_INCLUDE ?= crelude
CDIR ?= src/crelude
ODIR ?= o
DDIR ?= d
OBJS := $(patsubst $(CDIR)/%.c,$(ODIR)/%.o,$(wildcard $(CDIR)/*.c))
DEPS := $(patsubst $(CDIR)/%.c,$(DDIR)/%.d,$(wildcard $(CDIR)/*.c))
HEADERS := $(wildcard $(CDIR)/*.h)

OPT ?= -O3
WARN := -Wall -Wpedantic -Wextra -Wshadow
LINKS :=
INCLUDES := -Isrc
OPTIONS += -fPIC -funsigned-char -std=gnu11
DEPFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$(*F).d
CFLAGS += $(DEPFLAGS) $(WARN) $(OPTIONS) $(OPT) $(INCLUDES)
LDFLAGS += $(DEPFLAGS) -shared

ifeq ($(CC),clang)
	WARN += -Wno-gnu-zero-variadic-macro-arguments -Wno-language-extension-token -Wno-gnu-auto-type
	OPTIONS += -fms-extensions
endif
ifeq ($(CC),gcc)
	OPTIONS += -fplan9-extensions
endif
ifeq ($(OPT),-O0)
	OPTIONS += -finline-functions
endif

MAIN := $(ODIR)/tests.o

DATE := $(shell which gdate || which date)
NOW := $(shell $(DATE) +%s.%N)  # Eager.
END  = $(shell $(DATE) +%s.%N)  # Lazy.
TIME = $(shell dc -e "20 k $(END) $(NOW) - 1000 * p")

PREFIX ?= /usr

bold := $(shell tput bold)
dim := $(shell tput dim)
r := $(shell tput sgr0)

begin_command := @printf "$(dim)> "
end_command := @printf "$(r)"

all: pre-build $(TARGET) $(TARGET_LIB)
	$(eval T := $(TIME))
	@printf "$(bold)\nBuild success:$(r) \`$(TARGET)\`.\n"
	@printf "$(bold)Build success:$(r) \`$(TARGET_LIB)\`.\n"
	@printf "$(bold)Took:$(r) %g ms.\n" "$(T)"

debug-opts:
	$(eval TARGET := tests_debug)
	$(eval CFLAGS += -g$(shell which gdb && echo gdb3))
	$(eval OPTIONS += -fstack-protector -fstack-protector-all)
	$(eval OPT := -O$(shell which gdb && echo g || echo 0))

debug: debug-opts all

clean:
	@echo "[~] Cleaning last build."
	$(begin_command)
	rm -f ./$(TARGET) ./$(TARGET_LIB)
	$(end_command)
	$(begin_command)
	rm -fr ./$(ODIR)/ ./$(DDIR)/
	$(end_command)
	$(begin_command)
	rm -fr .docs/xml ./docs/doxybook2 ./docs/*.zip
	$(end_command)

fresh: clean all

DOXYBOOK_VER ?= 1.3.5
DOXYBOOK_TAR ?= linux-$(ARCH)
DOXYBOOK_ZIP ?= doxybook2-$(DOXYBOOK_TAR)-v$(DOXYBOOK_VER).zip
DOXYBOOK_BIN ?= ./docs/doxybook2/bin/doxybook2

docs/$(DOXYBOOK_ZIP):
	$(begin_command)
	curl -L https://github.com/matusnovak/doxybook2/releases/download/v$(DOXYBOOK_VER)/$(DOXYBOOK_ZIP) > ./docs/$(DOXYBOOK_ZIP)
	$(end_command)

docs/doxybook2: docs/$(DOXYBOOK_ZIP)
	$(begin_command)
	unzip -q docs/$(DOXYBOOK_ZIP) -d ./docs/doxybook2
	$(end_command)

$(DOXYBOOK_BIN): docs/doxybook2
	$(begin_command)
	strip $(DOXYBOOK_BIN)
	$(end_command)

./docs/book:
	$(begin_command)
	mkdir ./docs/book
	$(end_command)

docs: $(DOXYBOOK_BIN) ./docs/book
	@echo "[~] Building documentation."
	$(begin_command)
	git submodule update --init --recursive
	$(end_command)
	$(begin_command)
	cd ./docs && doxygen
	$(end_command)
	$(begin_command)
	$(DOXYBOOK_BIN) -q \
		--input ./docs/xml \
		--output ./docs/src \
		--config ./docs/.doxybook/config.json \
		--summary-input ./docs/src/SUMMARY.md.tmpl \
		--summary-output ./docs/src/SUMMARY.md
	$(end_command)
	$(begin_command)
	cd ./docs && mdbook build
	$(end_command)

$(ODIR):
	@mkdir $@
	@echo "[~] Making object directory: $@/"

$(DDIR):
	@mkdir $@
	@echo "[~] Making dependency directory: $@/"

pre-build: $(ODIR) $(DDIR)

$(TARGET): $(MAIN)
	@echo "$(bold)Building test target.$(r)"
	$(begin_command)
	$(CC) $(OPT) $(OPTIONS) -o $(TARGET) $(OBJS) $(MAIN) $(LINKS)
	$(end_command)

$(TARGET_LIB): $(OBJS)
	@echo "$(bold)Building shared library.$(r)"
	$(begin_command)
	$(CC) $(OPTIONS) $(LDFLAGS) -o $@ $^
	$(end_command)

install: $(TARGET) $(HEADERS)
	@echo "Installing to $(PREFIX)/lib/$(TARGET_LIB)."
	$(begin_command)
	install -d $(PREFIX)/lib
	$(end_command)
	$(begin_command)
	install -m 755 $(TARGET_LIB) $(PREFIX)/lib
	$(end_command)
	@echo "Installing to $(PREFIX)/include/$(TARGET_INCLUDE)/*.h."
	$(begin_command)
	install -d $(PREFIX)/include/$(TARGET_INCLUDE)
	$(end_command)
	$(begin_command)
	install -m 755 $(HEADERS) $(PREFIX)/include/$(TARGET_INCLUDE)
	$(end_command)

$(MAIN): src/tests.c $(OBJS)
	@echo "$(bold)Building test binary.$(r)"
	$(begin_command)
	$(CC) $(CFLAGS) -c src/tests.c -o $@ $(LINKS)
	$(end_command)

$(ODIR)/%.o: $(CDIR)/%.c $(DDIR)/%.d
	@printf "$(bold)Building object file.$(r) %25s  ->  %9s\n" "$<" "$@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(LINKS)

$(DEPS):

include $(wildcard $(DEPS))

whichcc:
	@echo "$(CC)"

.PHONY: all debug debug-opts docs pre-build clean install whichcc
