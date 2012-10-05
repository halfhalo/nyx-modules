/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
*      Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>
#include "rtc.h"

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

nyx_device_t *nyxDev;

#define DISPLAY_SYSFS_PATH "/sys/class/backlight/"

NYX_DECLARE_MODULE(NYX_DEVICE_LED_CONTROLLERS, "LedControllers");

nyx_error_t nyx_module_open (nyx_instance_t i, nyx_device_t** d)
{
    if (nyxDev) {
        nyx_info("LedControllers module already open.");
        return NYX_ERROR_NONE;
    }

    nyxDev = (nyx_device_t*)calloc(sizeof(nyx_device_t), 1);
    if (NULL == nyxDev)
        return NYX_ERROR_OUT_OF_MEMORY;

    nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_LED_CONTROLLER_EXECUTE_EFFECT_MODULE_METHOD,
        "led_controller_execute_effect");

    *d = (nyx_device_t*)nyxDev;
    return NYX_ERROR_NONE;
}

nyx_error_t nyx_module_close (nyx_device_t* d)
{
    return NYX_ERROR_NONE;
}

static int FileGetInt(const char *path, int *ret_data)
{
    GError *gerror = NULL;
    char *contents = NULL;
    char *endptr;
    gsize len;
    long int val;

    if (!path || !g_file_get_contents(path, &contents, &len, &gerror)) {
        if (gerror) {
            nyx_critical( "%s: %s", __FUNCTION__, gerror->message);
            g_error_free(gerror);
        }
        return -1;
    }

    val = strtol(contents, &endptr, 10);
    if (endptr == contents) {
        nyx_critical( "%s: Invalid input in %s.",
            __FUNCTION__, path);
        goto end;
    }

    if (ret_data)
        *ret_data = val;
end:
    g_free(contents);
    return 0;
}


static nyx_error_t handle_backlight_effect(nyx_device_handle_t handle, nyx_led_controller_effect_t effect)
{
    int max_brightness;
    int value;

    switch(effect.required.effect)
    {
    case NYX_LED_CONTROLLER_EFFECT_LED_SET:
#if 0
        if (FileGetInt(DISPLAY_SYSFS_PATH "fb_power", &power) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        /* activate display when it's powered off and we want to set some brightness
         * greater than zero */
        if (power == 0 && effect.backlight.brightness_lcd > 0)
        {
            if (!g_file_set_contents(DISPLAY_SYSFS_PATH "fb_power", "1", 1, NULL))
                return NYX_ERROR_GENERIC;
        }
#endif
        if (FileGetInt(DISPLAY_SYSFS_PATH "max_brightness", &max_brightness) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        value = (int)(100.0 / max_brightness * effect.backlight.brightness_lcd);

        if (!g_file_set_contents(DISPLAY_SYSFS_PATH "brightness", "10", 2))
            return NYX_ERROR_GENERIC;

        break;
    case NYX_LED_CONTROLLER_EFFECT_LED_GET:
        return NYX_ERROR_NOT_IMPLEMENTED;
    default:
        break;
    }

    effect.backlight.callback(handle, NYX_CALLBACK_STATUS_DONE,
                                  effect.backlight.context);
}

nyx_error_t led_controller_execute_effect(nyx_device_handle_t handle, nyx_led_controller_effect_t effect)
{
    if (handle != nyxDev)
        return NYX_ERROR_INVALID_HANDLE;

    if (effect.required.led == NYX_LED_CONTROLLER_BACKLIGHT_LEDS)
    {
        return handle_backlight_effect(handle, effect);
    }

    return NYX_ERROR_NONE;
}
