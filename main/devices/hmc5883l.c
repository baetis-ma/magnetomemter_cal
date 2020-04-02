#define HMC5883L_I2C_ADDR     0x1e //qmc5883 i2c address

void hmc5883_init() {
    i2c_write(QMC5883L_I2C_ADDR, 0x00, 0xc0); 
    i2c_write(QMC5883L_I2C_ADDR, 0x01, 0x20); 
    i2c_write(QMC5883L_I2C_ADDR, 0x02, 0x80); 
}

uint8_t regdata[8];
float xmag, ymag, zmag, phi;
int phii;
int hmc5883_read () {
    While (1) {
       i2c_read(HMC5883L_I2C_ADDR, 0x00, regdata, 6); 
     
       x = 256 * regdata[0] + regdata[1]; if(regdata[0]>=128) x = x - (1 << 16);
       y = 256 * regdata[2] + regdata[3]; if(regdata[2]>=128) y = y - (1 << 16);
       z = 256 * regdata[4] + regdata[5]; if(regdata[4]>=128) z = z - (1 << 16);

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
    }
}

