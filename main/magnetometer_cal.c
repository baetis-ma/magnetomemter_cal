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
#define QMC5883L_I2C_ADDR     0x0d //hmc5883 i2c address
#define i2c_frequency       500000 // max frequency of i2c clk
#define i2c_port                 0 //i2c channel on ESP-WROOM-32 ESP32S
#define i2c_gpio_scl            18 //D19 on ESP-WROOM-32 ESP32S
#define i2c_gpio_sda            19 //D18 on ESP-WROOM-32 ESP32S
#include "./interfaces/i2c.c"
#include "./devices/qmc5883l.c"

//globals
long hmc5883l_read[600];
int hmc5883l_read_ptr = 0;
char hex[10], xx[10];

void adc_task () {
    int cnt, samp = 1, tcnt = 0;
    uint8_t regdata[256];
    while(1){
        i2c_read(QMC5883L_I2C_ADDR, 0x00, regdata, 6); 
    
        int x = 256 * regdata[1] + regdata[0]; if(regdata[1]>=128) x = x - (1 << 16);
        int y = 256 * regdata[5] + regdata[4]; if(regdata[5]>=128) y = y - (1 << 16);
        int z = 256 * regdata[3] + regdata[2]; if(regdata[3]>=128) z = z - (1 << 16);

	printf ("%x %x %x %x %x %x\n",
           regdata[0],regdata[1],regdata[2],regdata[3],regdata[4],regdata[5]);
        ESP_LOGI("adc_task samp","----  %d %d ",  hmc5883l_read_ptr, samp);
        printf("x=%d y=%d z=%d\n",x,y,z);
        char tmp[16];
        snprintf( outstr, sizeof outstr, "1,%d,", ++tcnt);
        snprintf( tmp, sizeof tmp, "%d,", x); strcat (outstr, tmp);
        snprintf( tmp, sizeof tmp, "%d,", y); strcat (outstr, tmp);
        snprintf( tmp, sizeof tmp, "%d,", z); strcat (outstr, tmp);
        strcat (outstr, "\0");
        printf("%s\n", outstr);
        for(cnt = 0; cnt < samp; cnt++){
            hmc5883l_read[hmc5883l_read_ptr++] = tcnt;
            hmc5883l_read[hmc5883l_read_ptr++] = x;
            hmc5883l_read[hmc5883l_read_ptr++] = y;
            hmc5883l_read[hmc5883l_read_ptr++] = z;
            if(hmc5883l_read_ptr>400)hmc5883l_read_ptr=0;
        }
        vTaskDelay(50);
    }
}

void app_main()
{

    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi(); 
    wait_for_ip();

    i2c_init();
    i2cdetect();
    qmc5883_init(); //using qmc device

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 5, NULL);
}

