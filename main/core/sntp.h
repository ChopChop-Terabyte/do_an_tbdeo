#pragma once

#include <stdlib.h>
#include "esp_sntp.h"
#include "esp_timer.h"

#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct tm timeinfo;

extern int wday;
extern int mday;
extern int mon;
extern int year;
extern int sec;
extern int min;
extern int hour;

void get_time(void);
void sntp_config(void);

#ifdef __cplusplus
}
#endif
