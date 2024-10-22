#ifndef HP_READ_H
#define HP_READ_H

#include <hypercalls/hypercall.h>
#include <stdint.h>

int hp_read(int fildes, uint64_t phy_addr, uint64_t nbyte);

#endif
