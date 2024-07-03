#ifndef __HW_DEVICE_H__
#define __HW_DEVICE_H__

#include "../includes/lib.h"

typedef struct Device Device;

struct Device {
	void (*install)(Device *device);
	void (*uninstall)(Device *device);

	void (*enable)(Device *device);
	void (*disable)(Device *device);

	void (*free)(Device *device);
};

#endif