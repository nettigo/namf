//
// Created by viciu on 9/13/23.
//

#ifndef NAMF_LORAWAN_H
#define NAMF_LORAWAN_H

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include <SPI.h>
#include <CayenneLPP.h>

namespace LoRaWan {


#define LORAWAN_APP_DATA_BUFF_SIZE 64  /**< Size of the data to be transmitted. */
#define LORAWAN_APP_TX_DUTYCYCLE 10000 /**< Defines the application data transmission duty cycle. 10s, value in [ms]. */
#define APP_TX_DUTYCYCLE_RND 1000       /**< Defines a random delay for application data transmission duty cycle. 1s, value in [ms]. */
#define JOINREQ_NBTRIALS 3               /**< Number of trials for the join request. */





// Foward declaration
    extern void lorawan_has_joined_handler(void);

    extern void lorawan_rx_handler(lmh_app_data_t *app_data);

    extern void lorawan_confirm_class_handler(DeviceClass_t Class);

    extern void lorawan_join_failed_handler(void);

    extern void send_lora_frame(void);

    extern uint32_t timers_init(void);

    void setup(void);
    void send_lora_frame(void);

}

#endif //NAMF_LORAWAN_H
