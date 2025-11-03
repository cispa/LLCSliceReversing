#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "spectre_module.h"

MODULE_AUTHOR("Lorenz Hetterich");
MODULE_DESCRIPTION("Spectre PoC Kernel Module");
MODULE_LICENSE("GPL");

#define BUFFER_SIZE 20

#define maccess(x) asm volatile("mov (%0), %%al" ::"r"(x) : "%rax")
#define flush(x) asm volatile("clflush (%0)" ::"r"(x))
#define mfence() asm volatile("mfence")

#define aa(x) kmalloc(x, GFP_KERNEL)
#define ff kfree

uint8_t *buffer;

volatile uint64_t offsets[4096];

static int device_open(struct inode *inode, struct file *file) {
  /* Lock module */
  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode *inode, struct file *file) {
  /* Unlock module */
  module_put(THIS_MODULE);
  return 0;
}

static uint64_t page_walk(uint64_t address);

static long device_ioctl(struct file *file, unsigned int ioctl_num,
                         unsigned long ioctl_param) {

  struct spectre_gadget_params params;

  switch (ioctl_num) {

  case CMD_GADGET: {
    copy_from_user(&params, (void *)ioctl_param, sizeof(params));

    offsets[3137] = 1024;
    offsets[1024] = 2048;
    offsets[2048] = BUFFER_SIZE;

    mfence();

    flush(&offsets[3137]);
    flush(&offsets[2048]);
    flush(&offsets[1024]);

    mfence();

    asm volatile("stac");
    if(params.offset < offsets[offsets[offsets[3137]]] /* make sure the speculative window is long enough */) {
      maccess(
          &params.user_buf[((buffer[params.offset] >> params.bit) & 1) * 256]);
    }
    asm volatile("clac");
    break;
  }

  case CMD_INFO: {
    uint64_t phys = page_walk((uint64_t)buffer);
    copy_to_user((void *)(ioctl_param), &phys, 8);
    break;
  }
  }
  return 0;
}

static struct file_operations f_ops = {.unlocked_ioctl = device_ioctl,
                                       .open = device_open,
                                       .release = device_release};

static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = SPECTRE_MODULE_DEVICE_NAME,
    .fops = &f_ops,
    .mode = S_IRWXUGO,
};

static uint64_t cr3(void) {
  uint64_t value;
  asm volatile("mov %%cr3, %0" : "=r"(value));
  return value;
}

static uint64_t page_walk(uint64_t address) {
  int i;
  uint64_t entry = cr3();

  uint16_t b[4] = {(address >> 39) & 0b111111111, (address >> 30) & 0b111111111,
                   (address >> 21) & 0b111111111,
                   (address >> 12) & 0b111111111};

  for (i = 0; i < 4; i++) {
    entry = ((uint64_t *)__va(entry & 0x7fffffff000ull))[b[i]];
  }

  return (entry & 0x7fffffff000ull) + (address & 0xfff);
}

int init_module(void) {
  int r;

  /* Register device */
  r = misc_register(&misc_dev);
  if (r != 0) {
    printk(KERN_ALERT "[spectre-poc] Failed registering device with %d\n", r);
    return 1;
  }

  buffer = aa(4096);

  buffer[0] = 0x1;

  buffer[21] = 0x7f;

  printk(KERN_INFO "[spectre-poc] loaded 0x%llx\n",
         page_walk((uint64_t)buffer));

  return 0;
}

void cleanup_module(void) {
  misc_deregister(&misc_dev);

  ff(buffer);

  printk(KERN_INFO "[spectre-poc] Removed.\n");
}
