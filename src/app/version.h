/**
  ******************************************************************************
  * @file    version.h
  * @brief   Firmware version and build information
  ******************************************************************************
  */

#ifndef __VERSION_H
#define __VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Firmware version information */
#define FW_NAME        "Test Demo Firmware"
#define FW_VERSION     "v0.1.0"

/* Build date and time (automatically set during compilation) */
#define BUILD_DATE     __DATE__
#define BUILD_TIME     __TIME__

#ifdef __cplusplus
}
#endif

#endif /* __VERSION_H */
