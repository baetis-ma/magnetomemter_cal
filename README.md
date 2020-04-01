# Magnetometer Demonstration

I use this app to calibrate my magnetometer.

The magnetometer need to be in place where it will be used,
any ferromagnetic materials in area can cause big warpage of
the earths magnetic field (which is big but of a smallish magnetude).

Startup webpage, clear caliration and let measurements be made in
three degrees of freedom. Not that min and max values are being
accumulated for each axis. After you have done this for a while
(the magnitude should stay pretty close to 1), test
the calibration and look at the measurement resultsi on the plane
of operation that you will be opering on - if this module
is going to be run pretty much on a flat surface with only a few
degrees of pitch and roll this type of calibration is pretty close
and only the phi value need be used for orientation calculationi
(phi is your orientation). 
If you need accurate orientation over more than +/-15degrees of
pitch and roll you need to do some spherical coordinte system mapping
to local magnetic field inclination and declination, in that case
you would need to calculate orientation from both the phi and theta
results to figure orientation. 

When satisfied with cal save the 6 min/max calibration values,
point the board to what you want to be north and record the
phi value. These are your seven calibration values for this
module in this mounting position.


