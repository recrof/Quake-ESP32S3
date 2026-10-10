/**
 * @file esp_parallel.c
 * @brief Splits renderer work between the two cores (see esp_parallel.h).
 *
 * The worker runs on core 0 at a low priority: the display, audio and Bluetooth
 * tasks preempt it. Between parallelBegin() and parallelEnd() it polls for a job
 * instead of blocking, because a job lasts only tens of microseconds; the rest of
 * the time it sleeps. A job the worker has not started when the game core needs
 * it is taken back and run on the game core.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_parallel.h"

#define WORKER_TASK_CORE        0
#define WORKER_TASK_PRIORITY    1
#define WORKER_TASK_STACK_SIZE  3072

enum { JOB_IDLE, JOB_POSTED, JOB_RUNNING, JOB_DONE };

static TaskHandle_t workerTask;
static volatile int workerActive;
static volatile uint32_t jobState;          // internal RAM: atomics do not work in PSRAM
static void (*volatile jobFn)(void *arg);
static void *volatile jobArg;

static void worker(void *arg)
{
    (void) arg;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (workerActive)
        {
            uint32_t expected = JOB_POSTED;
            if (jobState == JOB_POSTED &&
                __atomic_compare_exchange_n(&jobState, &expected, JOB_RUNNING, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            {
                jobFn(jobArg);
                __atomic_store_n(&jobState, JOB_DONE, __ATOMIC_RELEASE);
            }
        }
    }
}

void parallelInit(void)
{
    if (xTaskCreatePinnedToCore(worker, "worker", WORKER_TASK_STACK_SIZE, NULL, WORKER_TASK_PRIORITY,
        &workerTask, WORKER_TASK_CORE) != pdPASS)
    {
        ESP_LOGW("parallel", "No worker task: rendering on one core");
        workerTask = NULL;
    }
}

void parallelBegin(void)
{
    if (workerTask)
    {
        workerActive = 1;
        xTaskNotifyGive(workerTask);
    }
}

void parallelEnd(void)
{
    workerActive = 0;
}

bool parallelPost(void (*fn)(void *arg), void *arg)
{
    if (!workerActive)
    {
        return false;
    }
    jobFn = fn;
    jobArg = arg;
    __atomic_store_n(&jobState, JOB_POSTED, __ATOMIC_RELEASE);
    return true;
}

void parallelFinish(void)
{
    uint32_t expected = JOB_POSTED;
    if (__atomic_compare_exchange_n(&jobState, &expected, JOB_IDLE, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    {
        // not started by the worker: run it here
        jobFn(jobArg);
        return;
    }
    while (__atomic_load_n(&jobState, __ATOMIC_ACQUIRE) != JOB_DONE)
    {
    }
    jobState = JOB_IDLE;
}
