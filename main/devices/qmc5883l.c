#define QMC5883L_I2C_ADDR     0x0d //qmc5883 i2c address

void qmc5883_init() {
    i2c_write(QMC5883L_I2C_ADDR, 0x09, 0x01); 
    i2c_write(QMC5883L_I2C_ADDR, 0x0a, 0x00); 
    i2c_write(QMC5883L_I2C_ADDR, 0x0b, 0x01); 
}
    
uint8_t regdata[8];
float xmag, ymag, zmag, phi;
int phii;
void qmc5883_read () {
    while (1) {
       i2c_read(QMC5883L_I2C_ADDR, 0x00, regdata, 6); 

       x = 256 * regdata[1] + regdata[0]; if(regdata[1]>=128) x = x - (1 << 16);
       y = 256 * regdata[5] + regdata[4]; if(regdata[5]>=128) y = y - (1 << 16);
       z = 256 * regdata[3] + regdata[2]; if(regdata[3]>=128) z = z - (1 << 16);

       xmag = (float) (x - 0.5*(xmax+xmin));
       ymag = (float) (y - 0.5*(ymax+ymin));
       zmag = (float) (z - 0.5*(zmax+zmin));
       xmag = (float) (xmag / 0.5*(xmax-xmin));
       ymag = (float) (ymag / 0.5*(ymax-ymin));
       zmag = (float) (zmag / 0.5*(zmax-zmin));
       phi = 90 + 57.3 * atan(ymag/xmag);
       if (phi < xmag) phi = phi + 180;
       phii = (int) (phi - caloff);
       phii = (360 + phii) % 360;
       vTaskDelay(50);
    }
}

