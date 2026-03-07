
#include <stdio.h>
#include "glue.h"

static FILE *file[4];

void
printer_open(uint8_t dev) {
	char filename[16];
	snprintf(filename, sizeof(filename), "printer%d.txt", dev);
	file[dev - KERN_DEVICE_PRINTERU4] = fopen(filename, "wb");
}

void
printer_bsout(uint8_t dev) {
	if (!file[dev - KERN_DEVICE_PRINTERU4]) return;
	char c = a == '\r' ? '\n' : a;
	fputc(c, file[dev - KERN_DEVICE_PRINTERU4]);
}

void
printer_close(uint8_t dev) {
	if (!file[dev - KERN_DEVICE_PRINTERU4]) return;
	fclose(file[dev - KERN_DEVICE_PRINTERU4]);
	file[dev - KERN_DEVICE_PRINTERU4] = NULL;
}
