#pragma once

#include <STMSensorsList.h>
#include "sensors.h"

int st_hal_open_sensors(void **data);

STMSensorsList& st_hal_get_sensors_list(void *data);

int st_hal_dev_activate(void *data, int handle, int enabled);

int st_hal_dev_batch(void *data, int handle,
                     int flags, int64_t period_ns, int64_t timeout);

int st_hal_dev_flush(void *data, int handle);

int st_hal_dev_poll(void *data, sensors_event_t *sdata, int count);
