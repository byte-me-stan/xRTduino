/*
 * Copyright (c) 2021-2022, RTduino Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * https://github.com/RTduino/RTduino
 * https://gitee.com/rtduino/RTduino
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-10     Meco Man     first version
 * 2022-05-01     Meco Man     support hardware timer
 * 2022-05-20     Meco Man     support no setup-loop structure
 */

#include <RTduino.h>
#include "wiring_private.h"

#define DBG_TAG    "RTduino.thread"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifndef RTDUINO_NO_SETUP_LOOP
#ifndef RTDUINO_THREAD_SIZE
#define RTDUINO_THREAD_SIZE     1024
#endif /* RTDUINO_THREAD_SIZE */

#ifndef RTDUINO_THREAD_PRIO
#define RTDUINO_THREAD_PRIO     30
#endif /* RTDUINO_THREAD_PRIO */

rt_align(RT_ALIGN_SIZE)
static rt_uint8_t rtduino_thread_stack[RTDUINO_THREAD_SIZE];
static struct rt_thread rtduino_thread;

static void rtduino_entry(void *parameter)
{
    setup();
    while(1)
    {
        loop();
        rt_schedule();
    }
}

#ifdef RTDUINO_USING_SKETCH
rt_align(RT_ALIGN_SIZE)
static rt_uint8_t rtduino_sketch_thread_stack[RTDUINO_THREAD_SIZE];
static struct rt_thread rtduino_sketch_thread;

static void rtduino_sketch_entry(void *parameter)
{
    sketch_setup();
    while(1)
    {
        sketch_loop();
        rt_schedule();
    }
}
#endif /* RTDUINO_USING_SKETCH */
#endif /* RTDUINO_NO_SETUP_LOOP */

#ifdef RTDUINO_DEFAULT_HWTIMER_DEVICE_NAME
static void hwtimer_init(void)
{
    rt_device_t hwtimer_device;

    hwtimer_device = rt_device_find(RTDUINO_DEFAULT_HWTIMER_DEVICE_NAME);
    if(hwtimer_device != RT_NULL)
    {
        rt_hwtimer_mode_t mode = HWTIMER_MODE_PERIOD; /* periodic */
        rt_uint32_t freq = 1000000; /* 1Mhz, 1us */
        rt_hwtimerval_t val = {.sec=1, .usec=0}; /* callback interval */

        if(rt_device_open(hwtimer_device, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        {
            LOG_E("Failed to open hardware timer!");
            return;
        }
        rt_device_control(hwtimer_device, HWTIMER_CTRL_FREQ_SET, &freq); /* set hwtimer prescaler frequency */
        rt_device_control(hwtimer_device, HWTIMER_CTRL_MODE_SET, &mode); /* set hwtimer mode */
        if(rt_device_write(hwtimer_device, 0, &val, sizeof(val)) == 0)
        {
            LOG_E("Failed to start (write) hardware timer!");
        }
    }
    else
    {
        LOG_W("Cannot find a hardware timer. Some functions cannot be used.");
    }
}
#endif /* RTDUINO_DEFAULT_HWTIMER_DEVICE_NAME */

/* initialization for BSP; maybe a blank function  */
rt_weak void initVariant(void) {}

static int rtduino_init(void)
{
#ifdef RTDUINO_DEFAULT_HWTIMER_DEVICE_NAME
    hwtimer_init();
#endif /* RTDUINO_DEFAULT_HWTIMER_DEVICE_NAME */

    initVariant();

#ifndef RTDUINO_NO_SETUP_LOOP
    rt_err_t rt_err = rt_thread_init(&rtduino_thread, "RTduino",
                            rtduino_entry, RT_NULL,
                            rtduino_thread_stack,
                            RTDUINO_THREAD_SIZE,
                            RTDUINO_THREAD_PRIO, 20);

    if (rt_err == RT_EOK)
    {
        rt_thread_startup(&rtduino_thread);
    }
    else
    {
        LOG_E("RTduino thread initialization failed!");
    }
#ifdef RTDUINO_USING_SKETCH
    rt_err = rt_thread_init(&rtduino_sketch_thread, "sketch",
                            rtduino_sketch_entry, RT_NULL,
                            rtduino_sketch_thread_stack,
                            RTDUINO_THREAD_SIZE,
                            RTDUINO_THREAD_PRIO, 20);

    if (rt_err == RT_EOK)
    {
        rt_thread_startup(&rtduino_sketch_thread);
    }
    else
    {
        LOG_E("RTduino sketch thread initialization failed!");
    }
#endif /* RTDUINO_USING_SKETCH */
#endif /* RTDUINO_NO_SETUP_LOOP */
    return 0;
}
INIT_COMPONENT_EXPORT(rtduino_init);
