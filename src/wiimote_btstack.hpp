#pragma once

#define inline __inline
#include <btstack.h>
#undef inline

#include <stdint.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

#define MAX_WIIMOTES 4

#define WIIMOTE_CONTROL_PSM		0x11
#define WIIMOTE_DATA_PSM		0x13
#define WIIMOTE_MTU				672

typedef std::vector<uint8_t> wiimote_report;