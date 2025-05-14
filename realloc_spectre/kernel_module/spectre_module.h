#ifndef _SPECTRE_MODULE_H
#define _SPECTRE_MODULE_H

#define SPECTRE_MODULE_DEVICE_NAME "spectre_module"
#define SPECTRE_MODULE_DEVICE_PATH "/dev/" SPECTRE_MODULE_DEVICE_NAME

#define SPECTRE_MODULE_IOCTL_MAGIC_NUMBER (long)0x325123

struct spectre_gadget_params {
    uint64_t offset;
    uint8_t* user_buf;
    int bit;
};

// execute the gadget with given offset and buffer base
#define CMD_GADGET _IOR(SPECTRE_MODULE_IOCTL_MAGIC_NUMBER,  1, struct spectre_gadget_params*)

// allocate new buffer and return physical addresses
#define CMD_INFO  _IOR(SPECTRE_MODULE_IOCTL_MAGIC_NUMBER,  2, uint64_t*)

#endif /* _SPECTRE_MODULE_H */
