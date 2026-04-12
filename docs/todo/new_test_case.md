
# test case: validate device temperature in range



Add a new test phase called verify_temperature_in_range.
Use SerialConsolePlug to send the 'temp' command and capture the response.
Measure 'Temperature' and validate it is within the expected range.
Use validators.in_range.

Follow the same structure as validate_firmware_version.


Example log:

```

> temp
vref_raw=0x296 vdda=3000mV calraw=0x41A ts_raw=0x293

Temperature: -36 C (raw=0x293)

> 


```

