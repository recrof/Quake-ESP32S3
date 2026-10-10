/**
 * @file esp_parallel.h
 * @brief Splits renderer work between the two cores: the game core (1) posts a job,
 * a worker task on core 0 runs it while the game core does its own part.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef ESP_PARALLEL_H
#define ESP_PARALLEL_H
#include <stdbool.h>
void parallelInit(void);
// The worker waits for jobs (busy) between these two calls: around the span drawing.
void parallelBegin(void);
void parallelEnd(void);
// Posts fn(arg) to the worker. Returns false if it cannot take jobs now (run fn yourself).
bool parallelPost(void (*fn)(void *arg), void *arg);
// Waits for the posted job. If the worker has not started it yet, it is run here.
void parallelFinish(void);
#endif
