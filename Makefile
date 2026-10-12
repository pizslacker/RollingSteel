CC = gcc
CFLAGS = -Wall -Wextra -O2
# Remove -lraylib from LDFLAGS
LDFLAGS = -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = RollingSteel
SRC = rollingsteel.c

# Define the path to your static raylib file (update this if it's stored elsewhere)
RAYLIB_STATIC = /usr/local/lib/libraylib.a 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(RAYLIB_STATIC) -o $(TARGET) $(LDFLAGS)
	strip $(TARGET)

clean:
	rm -f $(TARGET)