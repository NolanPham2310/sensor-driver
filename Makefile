EXEC = sensor_driver
SRC = sensor_driver.c
CFLAGS = -Wall -Wextra -O2

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

clean:
	rm -rf $(EXEC) *.o
