#include <string.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "soc/gpio_struct.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/spi_common.h"
#include "driver/i2c.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <math.h>
#include <driver/adc.h>
#include "sdkconfig.h"

//wifi sockets etc requirements
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//wifi requirements
#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80
#include "esp_wifi.h"
#include "./interfaces/wifisetup.c"
char outstr[4096];
char rx_buffer[1024];
#include "./interfaces/tcp_server_task.c"

//i2c and periferal requirements
#include "driver/i2c.h"
#define i2c_frequency       500000 // max frequency of i2c clk
#define i2c_port                 0 
#define i2c_gpio_scl            18 
#define i2c_gpio_sda            19 
#include "./interfaces/i2c.c"
//magnetometer calibration values
int xmax = 7062; int xmin = -5330;
int ymax = 6460; int ymin = -5900;
int zmax = 5567; int zmin = -5817;
float caloff = -2.3;
int x, y, z, tcnt = 0;
//new values in on pcb
#include "./devices/qmc5883l.c"

void app_main()
{

    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi(); 
    wait_for_ip();

    i2c_init();
    i2cdetect();
    qmc5883_init(); //using hmc device

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);
    xTaskCreate(qmc5883_read, "qmc5883_read", 4096, NULL, 5, NULL);

    char tmp[16];
    while (1) {
        snprintf( outstr, sizeof outstr, "1,%d,", ++tcnt);
        snprintf( tmp, sizeof tmp, "%d,", x); strcat (outstr, tmp);
        snprintf( tmp, sizeof tmp, "%d,", y); strcat (outstr, tmp);
        snprintf( tmp, sizeof tmp, "%d,", z); strcat (outstr, tmp);
        strcat (outstr, "\0");
        printf("%s\n", outstr);
        vTaskDelay(50);
    }
}

