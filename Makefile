CC?= cc
NAME = emilia
INSTALL_DIR = /usr/local/bin

ARGS =

SRC_DIR = src
BUILD_DIR = build
TARGET_DIR = .

LDLIBS = -lGL -lfreetype -lfontconfig -lutil -L/usr/lib -lm

ifeq ($(shell uname -s),FreeBSD)
	INCLUDES = -I/usr/local/include/freetype2/
else
	INCLUDES = -I/usr/include/freetype2/
endif

ifeq ($(mode),sanitized)
	CFLAGS = -std=c18 -MD -O0 -g3 -ffinite-math-only -fno-rounding-math -fshort-enums -fsanitize=address -fsanitize=undefined -DDEBUG
	LDFLAGS =  -fsanitize=address -fsanitize=undefined -fsanitize=unreachable
	LDLIBS += -lGLU
else ifeq ($(mode),debug)
	CFLAGS = -std=c18 -MD -g -O0 -fno-omit-frame-pointer -fshort-enums -DDEBUG
	LDFLAGS = -O0 -g
	LDLIBS += -lGLU
else ifeq ($(mode),debugoptimized)
	CFLAGS = -std=c18 -MD -g -O2 -fno-omit-frame-pointer -mtune=generic -ffast-math -fshort-enums -DDEBUG
	LDFLAGS = -O2 -g
	LDLIBS += -lGLU
else
	CFLAGS = -std=c18 -MD -O2 -flto -mtune=generic -ffast-math -fshort-enums
	LDFLAGS = -O2 -flto
endif

ifeq ($(shell ldconfig -p | grep libutf8proc.so > /dev/null || echo fail),fail)
$(info libutf8proc not found. Support for language-specific combining characters and unicode normalization will be disabled.)
	CFLAGS += -DNOUTF8PROC
else
	LDLIBS += -lutf8proc
endif

CCWNO = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Werror=implicit-function-declaration

SRCS = $(wildcard $(SRC_DIR)/*.c)

XLDLIBS = -lX11 -lXrender
WLLDLIBS = -lwayland-client -lwayland-egl -lwayland-cursor -lxkbcommon -lEGL

ifeq ($(window_protocol), x11)
	CFLAGS += -DNOWL
	OBJ = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
	LDLIBS += $(XLDLIBS)
else ifeq ($(window_protocol), wayland)
	CFLAGS += -DNOX
	OBJ = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
	LDLIBS += $(WLLDLIBS)
else
	OBJ = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
	LDLIBS += $(XLDLIBS) $(WLLDLIBS)
endif


$(NAME): $(OBJ)
	$(CC) $(OBJ) $(LDLIBS) -o $(TARGET_DIR)/$(NAME) $(LDFLAGS)

all: $(OBJ)
	$(CC) $(OBJ) $(LDLIBS) -o $(TARGET_DIR)/$(NAME) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $< $(CFLAGS) $(CCWNO) $(INCLUDES) -o $@

run:
	./$(TARGET_DIR)/$(NAME) $(ARGS)

debug:
	gdb --args ./$(TARGET_DIR)/$(NAME) $(ARGS)

clean:
	-@rm -rf $(NAME) $(BUILD_DIR)

install:
	@cp $(NAME) $(INSTALL_DIR)/

uninstall:
	$(RM) $(INSTALL_DIR)/$(NAME)

-include $(OBJ:.o=.d)
