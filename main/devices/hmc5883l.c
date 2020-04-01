#include <string.h>
#include <stdio.h>

//#define I2C_ADDR               0x0d //hmc5883 i2c address
#define I2C_ADDR               0x1e //hmc5883 i2c address
#define I2C_MASTER_FREQ_HZ   500000 // max frequency of i2c clk
#define I2C_MASTER_NUM            0 //i2c channel on ESP-WROOM-32 ESP32S
#define I2C_MASTER_SCL_IO        19 //D19 on ESP-WROOM-32 ESP32S
#define I2C_MASTER_SDA_IO        18 //D18 on ESP-WROOM-32 ESP32S

#include "boilerplate.h"

#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80
#include "wifisetup.h"

//globals
long hmc5883l_read[600];
int hmc5883l_read_ptr = 0;
char hex[10], xx[10];
static void tcp_server_task(void *pvParameters)
{
    char rx_buffer[1500];
    char addr_str[16];
    int addr_family;
    int ip_protocol;
    char *temp;

    while (1) {
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        listen(listen_sock, 1);

        struct sockaddr_in sourceAddr;
        uint addrLen = sizeof(sourceAddr);
        while (1) {
            int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
            if (sock < 0) { ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno); break; }
            ESP_LOGI(TAG, "Socket accepted");
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            // Error occured during receiving
            if (len <= 0) { ESP_LOGE(TAG, "recv failed: errno %d", errno); break; }
            else {
                //prints client source ip and http request packet to esp monitor
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, 
                              addr_str, sizeof(addr_str) - 1);
                ESP_LOGI("---  ", "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI("", "HTTP REQUEST Packet\n%s", rx_buffer);
                //parse request
                char http_type[8];
                char temp_str[16];
                sscanf(rx_buffer, "%s/", http_type);
                ESP_LOGI("", "request type %s  ", http_type);
                temp = strstr(rx_buffer, "?");    
                if(temp){
                   memcpy(temp_str, rx_buffer+strlen(http_type)+2, 
                                    strlen(rx_buffer)-strlen(temp));
                   temp_str[strlen(temp_str)] = '\0';
                } else {
                   sscanf(rx_buffer+strlen(http_type)+2, "%s", temp_str);
                }
                //return html page to socket
                if (strcmp("index.html", temp_str) ==0 ){
                    int pkt_buf_size = 1500;
                    int pkt_end = pkt_buf_size;
                    extern const char index_html_start[] asm("_binary_index_html_start");
                    extern const char index_html_end[] asm("_binary_index_html_end");
                    int html_len =  strlen(index_html_start) - strlen(index_html_end);
                    ESP_LOGI(TAG, "made it into loop HTML html_len %d", html_len );
                    for( int pkt_ptr = 0; pkt_ptr < html_len; pkt_ptr = pkt_ptr + pkt_buf_size){
                        if ((html_len - pkt_ptr) < pkt_buf_size) pkt_end = html_len - pkt_ptr;
                            ESP_LOGI(TAG, "HTML pkt_ptr %d pkt_end %d html_len %d", pkt_ptr,pkt_end,html_len );
                            send(sock, index_html_start + pkt_ptr, pkt_end, 0);
                        }
                } else 
                //parse datagram, rcv data from webpage or return collected data
                if (strcmp("GET", http_type) ==0 || strcmp("getData",temp_str) ==0 ){
                    int x;
                    char tmp[40];
                    char outstr[1500];
                    temp = strstr(rx_buffer, "hex=");  if(temp)sscanf(temp,"hex=%s", hex); 
                    temp = strstr(rx_buffer, "xx=");  if(temp)sscanf(temp,"xx=%s", xx); 
                    printf ("hex= %s xx=%s\n", hex, xx);
            
                    int hmc5883l_read_ptr_shadow = hmc5883l_read_ptr;
                    hmc5883l_read_ptr = 0;
                    
                    snprintf( outstr, sizeof outstr, "%d,",hmc5883l_read_ptr_shadow );
                    for( x = 0; x < hmc5883l_read_ptr_shadow; x=x+4){
                        snprintf(tmp, sizeof tmp, "%ld,", hmc5883l_read[x]); strcat (outstr, tmp);  
                        snprintf(tmp, sizeof tmp, "%ld,", hmc5883l_read[x+1]); strcat (outstr, tmp);  
                        snprintf(tmp, sizeof tmp, "%ld,", hmc5883l_read[x+2]); strcat (outstr, tmp);  
                        snprintf(tmp, sizeof tmp, "%ld,", hmc5883l_read[x+3]); strcat (outstr, tmp);  
                    }
                    strcat (outstr, "\0");  
                    ESP_LOGI("","sizeof=%d  %s",strlen(outstr), outstr);
                    send(sock, outstr, sizeof outstr, 0);
                    
                //resource not found 
                } else {
                    char temp404[] = "<html><h1>Not Found 404</html>";
                    send(sock, temp404, sizeof(temp404), 0);
                    //ESP_LOGI("", "404");
                }
                vTaskDelay(10/ portTICK_RATE_MS); //waits for buffer to purge
                shutdown(sock, 0);
                close(sock);
            }
        }
    }
    vTaskDelete(NULL);
}

void adc_task () {
    int cnt, samp = 1, tcnt = 0;
    uint8_t regdata[256];
    while(1){
        i2c_read(I2C_MASTER_NUM, 0x03, regdata, 6); 
    
        int x = 256 * regdata[0] + regdata[1]; if(regdata[0]>=128) x = x - (1 << 16);
        int y = 256 * regdata[2] + regdata[3]; if(regdata[2]>=128) y = y - (1 << 16);
        int z = 256 * regdata[4] + regdata[5]; if(regdata[4]>=128) z = z - (1 << 16);
        ESP_LOGI("adc_task samp","----  %d %d ",  hmc5883l_read_ptr, samp);
        printf("x=%d y=%d z=%d\n",x,y,z);
        for(cnt = 0; cnt < samp; cnt++){
            tcnt++;
            hmc5883l_read[hmc5883l_read_ptr++] = tcnt;
            hmc5883l_read[hmc5883l_read_ptr++] = x;
            hmc5883l_read[hmc5883l_read_ptr++] = y;
            hmc5883l_read[hmc5883l_read_ptr++] = z;
            if(hmc5883l_read_ptr>400)hmc5883l_read_ptr=0;
        }
        vTaskDelay(20);
    }
}

void app_main()
{

    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi(); cycles(1);
    wait_for_ip();
    i2c_master_init();

    //configure hmc5883l with i2c instructions
    uint8_t data[1];
    data[0] = 0x0c;
    i2c_write(I2C_MASTER_NUM, 0x00, data, 1); cycles(1);
    data[0] = 0x20;
    i2c_write(I2C_MASTER_NUM, 0x01, data, 1); cycles(1);
    data[0] = 0x80;
    i2c_write(I2C_MASTER_NUM, 0x02, data, 1); cycles(1);

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 5, NULL);
}

