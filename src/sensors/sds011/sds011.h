//
// Created by viciu on 13.01.2020.
//

#ifndef NAMF_SENSORS_SDS011
#define NAMF_SENSORS_SDS011

#include "Arduino.h"
#include "variables.h"
#include "helpers.h"
#include "system/debug.h"
#include "html-content.h"   //just for sensor name, to be removed later
namespace SDS011 {


    void readSingleSDSPacket(int *pm10_serial, int *pm25_serial);


/*****************************************************************
 * send SDS011 command (start, stop, continuous mode, version    *
 *****************************************************************/
    void SDS_rawcmd(const uint8_t cmd_head1, const uint8_t cmd_head2, const uint8_t cmd_head3);

    bool SDS_cmd(PmSensorCmd cmd);

    bool SDS_checksum_valid(const uint8_t (&data)[8]);


/*****************************************************************
 * read SDS011 sensor serial and firmware date                   *
 *****************************************************************/
    String SDS_version_date();


/*****************************************************************
 * read SDS011 sensor values                                     *
 *****************************************************************/
    static void sensorSDS(String &s) ;
}

#endif //NAMF_SENSORS_SDS011
