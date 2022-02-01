// program frequency synthesizer (PCI Express and USB only)
//
// SyncLink devices have a fixed 14.7456MHz base clock.
// The serial controller generates clocks by dividing the base clock
// by a 16-bit integer. Generating a clock that is not a divisor of
// the fixed base clock requires the frequency synthesizer.
//
// The frequency synthesizer is an IDT ICS307-3. Serial controller
// GPIO signals drive the synthesizer SPI interface for programming
// a 132 bit word calculated by the IDT Versaclock 2 software.
//


#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <asm/ioctl.h>
#include <linux/types.h>
#include <pthread.h>
#include <termios.h>
#include <memory.h>
#include <signal.h>


#include "../include/synclink.h"
// size of data sent in this sample
#define DATA_SIZE 100

// 1 = continuous send data (no idle between writes)
// 0 = bursts of data (zeros) separated by idle (ones)
#define CONTINUOUS_SEND 1

int run = 1;

struct table_entry {
	unsigned int freq;    /* frequency */
	unsigned int data[5]; /* programming data */
};

// PCI
// ICS307-3:
// - reference clock = 14.7456MHz oscillator
// - VDD = 3.3V
// - CLK1 (pin 8) outputs clock
struct table_entry pci_table[] =
{
	{ 1228800, {0x1800155E, 0x29A00000, 0x00000000, 0x0000DFFF, 0x60000000}},
	{12288000, {0x29BFDC00, 0x61200000, 0x00000000, 0x0000A5FF, 0xA0000000}},
	{14745600, {0x38003C05, 0x24200000, 0x00000000, 0x000057FF, 0xA0000000}},
	{16000000, {0x280CFC02, 0x64A00000, 0x00000000, 0x000307FD, 0x20000000}},
	{16384000, {0x08001402, 0xA1200000, 0x00000000, 0x0000A5FF, 0xA0000000}},
	{20000000, {0x00001403, 0xE0C00000, 0x00000000, 0x00045E02, 0xF0000000}},
	{24000000, {0x00001405, 0x61400000, 0x00000000, 0x0004D204, 0x30000000}},
	{28219200, {0x00001405, 0xA1400000, 0x00000000, 0x0003C1FC, 0x20000000}},
	{30000000, {0x20267C05, 0x64C00000, 0x00000000, 0x00050603, 0x30000000}},
	{32000000, {0x21BFDC00, 0x5A400000, 0x00000000, 0x0004D206, 0x30000000}},
	{45056000, {0x08001406, 0xE0200000, 0x00000000, 0x000217FE, 0x20000000}},
	{64000000, {0x21BFDC00, 0x12000000, 0x00000000, 0x000F5E14, 0xF0000000}},
	{0, {0, 0, 0, 0, 0}} /* final entry must have zero freq */
};

// USB
// ICS307-3:
// - reference clock = 14.7456MHz xtal
// - VDD = 3.3V
// - CLK3 (pin 14) fsynth
// - CLK1 (pin 8)  base clock
//
// Note: CLK1 and CLK3 outputs must always be driven to prevent floating
// clock inputs to the FPGA. When calculating programming word with Versaclock,
// select same output on CLK1 and CLK3 or select CLK1 as multiple of CLK3.
struct table_entry usb_table[] =
{
	{ 1228800, {0x296C1402, 0x25200000, 0x00000000, 0x00009FFF, 0xA0000000}},
	{12288000, {0x28401400, 0xE5200000, 0x00000000, 0x00009BFF, 0xA0000000}},
	{14745600, {0x28481401, 0xE5200000, 0x00000000, 0x0000A5FF, 0xA0000000}},
	{16000000, {0x284C1402, 0x64A00000, 0x00000000, 0x000307FD, 0x20000000}},
	{16384000, {0x28501402, 0xE4A00000, 0x00000000, 0x0001F9FE, 0x20000000}},
	{20000000, {0x205C1404, 0x65400000, 0x00000000, 0x00068205, 0xF0000000}},
	{24000000, {0x20641405, 0x65400000, 0x00000000, 0x0004D204, 0x30000000}},
	{28219200, {0x20641405, 0x64C00000, 0x00000000, 0x0004F603, 0x70000000}},
	{30000000, {0x20641405, 0x64C00000, 0x00000000, 0x00050603, 0x30000000}},
	{32000000, {0x206C1406, 0x65400000, 0x00000000, 0x00049E03, 0xF0000000}},
	{45056000, {0x28701406, 0xE4200000, 0x00000000, 0x000217FE, 0x20000000}},
	{64000000, {0x20781400, 0x4D400000, 0x00000000, 0x00049E03, 0xF0000000}},
	{0, {0, 0, 0, 0, 0}} /* final entry must have zero freq */
};

void sigint_handler(int sigid)
{
	printf("Ctrl-C pressed\n");
	run = 0;
}

void configure_port(int fd) {
	int ldisc;
	struct termios termios;
	MGSL_PARAMS params;

	// Set line discipline, a software layer between tty devices
	// and user applications that performs intermediate processing.
	// N_TTY = byte oriented line discipline
	ldisc = N_TTY;
	ioctl(fd, TIOCSETD, &ldisc);

	// set N_TTY details with termios functions
	tcgetattr(fd, &termios);
	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_cflag = CREAD | CS8 | HUPCL | CLOCAL;
	termios.c_lflag = 0;
	termios.c_cc[VTIME] = 0;
	termios.c_cc[VMIN] = DATA_SIZE;
	tcsetattr(fd, TCSANOW, &termios);

	// get, modify and set device parameters
	ioctl(fd, MGSL_IOCGPARAMS, &params);
	params.mode = MGSL_MODE_RAW;
	params.encoding = HDLC_ENCODING_NRZ;
	params.crc_type = HDLC_CRC_NONE;
	params.loopback = 0;
	params.flags = HDLC_FLAG_RXC_RXCPIN + HDLC_FLAG_TXC_TXCPIN;
	params.clock_speed = 2400;
	ioctl(fd, MGSL_IOCSPARAMS, &params);

	// set transmit idle pattern (sent between frames)
	ioctl(fd, MGSL_IOCSTXIDLE, HDLC_TXIDLE_ONES);

	// set receive data transfer size: range=1-256, default=256
	// < 128  : programmed I/O (PIO), low data rate
	// >= 128 : direct memory access (DMA), MUST be multiple of 4
	// Lower values reduce receive latency (time from receiving data
	// until it becomes available to system) but increase overhead.
	ioctl(fd, MGSL_IOCRXENABLE, (8 << 16));

	// set transmit data transfer mode
	if (CONTINUOUS_SEND) {
		ioctl(fd, MGSL_IOCTXENABLE, MGSL_ENABLE_PIO);
	} else {
		ioctl(fd, MGSL_IOCTXENABLE, MGSL_ENABLE_DMA);
	}

	// use blocking reads and writes
	fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) & ~O_NONBLOCK);
}

// display buffer in hex format, 16 bytes per line
void display_buf(unsigned char *buf, int size)
{
	int i;
	for (i = 0 ; i < size ; i++) {
		if (!(i % 16))
			printf("%04X: ", i);
		if (i % 16 == 15)
			printf("%02X\n", *buf++);
		else
			printf("%02X ", *buf++);
	}
	if (i % 16)
			printf("\n");
}

// Raw mode saves a bit every clock cycle without distinguishing
// between data/idle/noise. There is no framing or byte alignment.
// Sample data = all 0. Idle pattern = all 1.
// Data is shifted 0-7 bits with bytes possibly spanning 2
// read buffer bytes. Serial bit order is LSB first.
void *receive_func(void *ptr)
{
	int fd = *((int *)ptr);
	int rc;
	unsigned char buf[DATA_SIZE];
	int i = 1;
	while (run) {
		// block until DATA_SIZE bytes received (termios VMIN)
		rc = read(fd, buf, sizeof(buf));
		if (rc < 1) {
			break;
		}
		printf("<<< %09d received %d bytes\n", i, rc);
		display_buf(buf, rc);
		i++;
	}
	return NULL;
}

void set_gpio(int fd, int bit, int val)
{
	struct gpio_desc gpio = {val << bit, 1 << bit, 0, 0};
	ioctl(fd, MGSL_IOCSGPIO, &gpio);
}

// return 0 on success or error code
int set_fsynth_rate(int fd, unsigned int freq)
{
	int i;
	unsigned int dword_val = 0;
	MGSL_PARAMS params;
	struct gpio_desc gpio = {0, 0, 0, 0xffffffff};
	unsigned int usb_mask = (0xf << 20); // expected USB outputs
	int mux = 15; // PCI mux bit
	int clk = 14; // PCI clk bit
	int sel = 13; // PCI sel bit
	int dat = 12; // PCI dat bit
	struct table_entry *table = pci_table;
	struct table_entry *entry;
	unsigned int *data = NULL;

	// check GPIO directions against expected USB outputs
	if (!ioctl(fd, MGSL_IOCGGPIO, &gpio) &&
		((gpio.dir & usb_mask) == usb_mask)) {
		mux = 23;
		clk = 22;
		sel = 21;
		dat = 20;
		table = usb_table;
	}

	// search for frequency in table
	for (entry = table ; entry->freq ; entry++) {
		if (entry->freq == freq) {
			data = entry->data;
			break;
		}
	}
	if (!data) {
		return -1;
	}

	set_gpio(fd, clk, 0);
	for (i = 0 ; i < 132 ; i++) {
		if (!(i % 32))
			dword_val = data[i/32];
		set_gpio(fd, dat, (dword_val & (1 << 31)) ? 1 : 0);
		set_gpio(fd, clk, 1); // pulse clk to load bit
		set_gpio(fd, clk, 0);
		dword_val <<= 1;
	}
	set_gpio(fd, sel, 1); // pulse sel to accept data
	set_gpio(fd, sel, 0);
	set_gpio(fd, mux, 1); // switch base clock to fsynth

	// Tell serial device the new base clock frequency.
	// Repeat for all ports on same card (common base clock).
	params.mode = MGSL_MODE_BASE_CLOCK;
	params.clock_speed = freq;
	ioctl(fd, MGSL_IOCSPARAMS, &params);

	return 0;
}

int main(int argc, char* argv[])
{
	int fd;
	unsigned int freq = 20000000;
        int rc;
	int i;
	unsigned char buf[DATA_SIZE*2];
	char *devname;
	pthread_t receive_thread;
	if (argc > 1)
		devname = argv[1];
	else
		devname = "/dev/ttySLG0";

	//if (argc < 2) {
	//	printf("No device name specified.\n");
	//	return -1;
	//}
	printf("set fsynth rate for %s to %d\n", devname, freq);
	// open with O_NONBLOCK to ignore DCD
	if ((fd = open(devname, O_RDWR | O_NONBLOCK, 0)) < 0) {
		printf("open error=%d %s\n", errno, strerror(errno));
		return fd;
	}

	if (set_fsynth_rate(fd, freq)) {
		printf("%d not found in table.\n", freq);
		return -1;
	}
        // Here begins the main from raw.c, implement from here


	printf("raw sample running on %s\n", devname);

	// open with O_NONBLOCK to ignore DCD
	fd = open(devname, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) {
		printf("open error=%d %s\n", errno, strerror(errno));
		return errno;
	}

	if (strstr(devname, "USB")) {
		int arg;
		ioctl(fd, MGSL_IOCGIF, &arg);
		// uncomment to select interface (RS232,V35,RS422)
		arg = (arg & ~MGSL_INTERFACE_MASK) | MGSL_INTERFACE_RS422;
		ioctl(fd, MGSL_IOCSIF, arg);
		if (!(arg & MGSL_INTERFACE_MASK)) {
			printf("USB serial interface must be selected.\n");
			return -1;
		}
	}

	configure_port(fd);

	printf("Press Ctrl-C to stop program.\n");
	signal(SIGINT, sigint_handler);
	siginterrupt(SIGINT, 1);

	ioctl(fd, MGSL_IOCRXENABLE, 1);
	pthread_create(&receive_thread, NULL, receive_func, &fd);

	// Prepare all zeros data to contrast with all ones idle pattern.
	// Raw mode=no byte alignment, receive data may be bit shifted.
	memset(buf, 0, DATA_SIZE);

	i = 1;
	while (run) {
		printf(">>> %09d send %d bytes\n", i, DATA_SIZE);
		rc = write(fd, buf, DATA_SIZE);
		if (rc < 0) {
			break;
		}
		if (CONTINUOUS_SEND) {
			// prevent idle by keeping send count > 0
			// limit latency by keeping send count < 2*DATA_SIZE
			// latency = time from write to serial data output
			printf(">>> wait for send count <= %d\n", DATA_SIZE);
			for (;;) {
				int count;
				rc = ioctl(fd, TIOCOUTQ, &count);
				if (rc < 0 || count <= DATA_SIZE) {
					break;
				}
				usleep(5000);
			}
		} else {
			// block until all sent to insert idle between data
			tcdrain(fd);
			usleep(25000);
		}
		i++;
	}

	close(fd);
	return 0;
}
