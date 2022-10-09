#include "../include/misc.h"
#include <linux/i2c-dev.h>


AriesErrorType svbSwitchRead(
        int handle,
        int reg)
{
    uint8_t cmd;
    uint8_t values[1];
    int rc;

    if (reg == 1)
    {
        cmd = 0x1;
    }
    else if (reg ==2)
    {
        cmd  = 0x2;
    }
    else {
        return ARIES_INVALID_ARGUMENT;
    }

    rc = asteraI2CWriteBlockData(handle, cmd, 0, values);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}
