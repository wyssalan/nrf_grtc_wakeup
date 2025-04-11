#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <inttypes.h>
#include <stdio.h>
#include "sensor.h"
#include "coap_client_utils.h"
// Include NVS Storage
#include "storage_nvs.h"

#define DEEP_SLEEP_TIME_S 5

LOG_MODULE_REGISTER(MAIN);

const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void enter_sleep(void)
{
    int rc;
    rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printf("Could not suspend console (%d)\n", rc);
		return;
	}
    int err = z_nrf_grtc_wakeup_prepare(DEEP_SLEEP_TIME_S * USEC_PER_SEC);
    if (err < 0)
    {
        LOG_ERR("Unable to prepare GRTC as a wake up source (err = %d).\n", err);
    }
    else
    {
        LOG_INF("Entering system off; wait %u seconds to restart\n", DEEP_SLEEP_TIME_S);
        sys_poweroff();
    }
    // LOG_INF("Prepared");
    // k_msleep(500);
    
}

int main(void)
{
    // while(1){
    // LOOP ACTIVE BECAUSE OF SLEEP MODE
    float co2, temper, hum;
    

    // LOG_INF("Initializing NVS");
    // if (IS_ENABLED(CONFIG_NVS))
    // {
    //     ret = storage_nvs_init();
    //     if (ret)
    //     {
    //         LOG_ERR("NVS Init failed (error: %d)", ret);
    //         return 0;
    //     }
    // }

    coap_client_utils_init();
    sensor_measure(&co2, &temper, &hum);
    LOG_INF("CO2: %f ppm", (double)co2);
    LOG_INF("Temperature: %f °C", (double)temper);
    LOG_INF("Humidity: %f RH", (double)hum);

    k_msleep(5000);
    coap_client_send_measured_data(&co2, &temper, &hum);

    k_msleep(5000);
    enter_sleep();
    
    // }

    return 0;
}