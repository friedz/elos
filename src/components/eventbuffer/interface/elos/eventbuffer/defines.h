// SPDX-License-Identifier: MIT
#pragma once

#define ELOS_EVENTBUFFER_PRIORITY_NORMAL 0
#define ELOS_EVENTBUFFER_PRIORITY_HIGH   1
#define ELOS_EVENTBUFFER_PRIORITY_COUNT  (ELOS_EVENTBUFFER_PRIORITY_HIGH + 1)

#ifndef ELOS_EVENTBUFFER_DEFAULT_LIMIT
#define ELOS_EVENTBUFFER_DEFAULT_LIMIT 128
#endif

#define ELOS_EVENTBUFFER_NO_TRIGGER (-1)