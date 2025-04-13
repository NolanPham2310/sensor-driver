#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <sys/stat.h>

#define I2C_BUS "/dev/i2c-2"      // BeaglePlay I2C bus
#define DEVICE_ADDR 0x48          // VEML6030 address (ADDR high)
#define FIFO_PATH "/tmp/sensor_fifo"

int main() {
    int fd_i2c, fd_fifo;
    char buf[3];
    const float resolution = 0.0288; // lx/bit for gain ×2, 100ms integration

    // Open I2C bus
    if ((fd_i2c = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    // Set slave address
    if (ioctl(fd_i2c, I2C_SLAVE, DEVICE_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(fd_i2c);
        return 1;
    }

    // Configure sensor (Register 0x00): Gain ×2 (01), Integration Time 100ms (0000), Power On (0)
    buf[0] = 0x00;    // Command code
    buf[1] = 0x00;    // LSB
    buf[2] = 0x08;    // MSB (0x0800 = 0000 10 00 0000 0000)
    if (write(fd_i2c, buf, 3) != 3) {
        perror("Failed to configure sensor");
        close(fd_i2c);
        return 1;
    }

    // Create FIFO if it doesn’t exist
    if (access(FIFO_PATH, F_OK) != 0) {
        if (mkfifo(FIFO_PATH, 0666) < 0) {
            perror("Failed to create FIFO");
            close(fd_i2c);
            return 1;
        }
    }

    // Open FIFO for writing
    if ((fd_fifo = open(FIFO_PATH, O_WRONLY)) < 0) {
        perror("Failed to open FIFO");
        close(fd_i2c);
        return 1;
    }

    while (1) {
        // Select ALS data register (0x04)
        buf[0] = 0x04;
        if (write(fd_i2c, buf, 1) != 1) {
            perror("Failed to select ALS register");
            continue;
        }

        // Read 2 bytes of ALS data
        if (read(fd_i2c, buf, 2) != 2) {
            perror("Failed to read ALS data");
            continue;
        }

        // Combine bytes (little-endian)
        u_int16_t als_data = (buf[1] << 8) | buf[0];
        float lux = als_data * resolution;

        // Write to FIFO
        char msg[50];
        snprintf(msg, sizeof(msg), "Lux: %.2f\n", lux);
        if (write(fd_fifo, msg, strlen(msg)) < 0) {
            perror("Failed to write to FIFO");
        }

        sleep(1); // Update every second
    }

    printf("Sensor driver initialized.\n");
    return 0;
    close(fd_fifo);
    close(fd_i2c);
    return 0;
}
