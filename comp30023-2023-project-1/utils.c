//
// Created by Chris on 2023/4/2.
//

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void getBigEndian(int num, uint8_t *buffer){
    /*
     * This function is used to convert a 32-bit integer to big endian
     */
    buffer[0] = (uint8_t)(num / 16777216);
    buffer[1] = (uint8_t)((num / 65536) % 256);
    buffer[2] = (uint8_t)((num / 256) % 256);
    buffer[3] = (uint8_t)(num % 256);
}