CC ?= gcc

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
OPTIONS := -fPIC -funsigned-char -std=gnu11
DEPFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$(*F).d
CFLAGS += $(DEPFLAGS) $(WARN) $(OPTIONS) $(OPT) $(INCLUDES)
LDFLAGS += $(DEPFLAGS) -shared

ifeq ($(CC),clang)
	WARN += -Wno-gnu-zero-variadic-macro-arguments -Wno-language-extension-token
	OPTIONS += -fms-extensions
endif
ifeq ($(CC),gcc)
	OPTIONS += -fplan9-extensions
endif
ifeq ($(OPT),-O0)
	OPTIONS += -finline-functions
endif

MAIN := $(ODIR)/tests.o

NOW := $(shell date +%s.%N)  # Eager.
END  = $(shell date +%s.%N)  # Lazy.
TIME = $(shell dc -e "20 k $(END) $(NOW) - 1000 * p")

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

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

clean:
	@echo "[~] Cleaning last build."
	$(begin_command)
	rm -f ./$(TARGET) ./$(TARGET_LIB)
	$(end_command)
	$(begin_command)
	rm -fr ./$(ODIR)/ ./$(DDIR)/
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
	$(CC) $(OPT) -o $(TARGET) $(OBJS) $(MAIN) $(LINKS)
	$(end_command)

$(TARGET_LIB): $(OBJS)
	@echo "$(bold)Building shared library.$(r)"
	$(begin_command)
	$(CC) $(LDFLAGS) -o $@ $^
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
	@printf "$(bold)Building object file.$(r) %25s  ->  %13s\n" "$<" "$@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(LINKS)

$(DEPS):

include $(wildcard $(DEPS))

.PHONY: all pre-build clean install
