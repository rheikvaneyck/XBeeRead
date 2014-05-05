#ifndef __READXBEE_H__
#define __READXBEE_H__

#include <stdint.h>

uint8_t calc_chksum(uint8_t* data, uint64_t len)
{
  uint64_t i;
  uint64_t sum = 0;

  for(i=0;i<len;i++){
    sum += data[i];
  }
  return (0xFF - (sum & 0xFF));
} 

uint8_t verify_chksum(uint8_t* data, uint64_t len, uint8_t chksum)
{
  uint64_t i;
  uint64_t sum = 0;

  for(i=0;i<len;i++){
    sum += data[i];
  }

  sum = sum & 0xFF;

  if ((sum + chksum) == 0xFF)
    return (1);
  else
    return (0);  
}

struct xbee_frame {
  uint8_t type;
  union {
    uint64_t value;
    uint8_t bytes[8];
  } src_addr;
  union {
    uint16_t value;
    uint8_t bytes[2];
  } src_net;
  uint8_t rx_opt;
  uint8_t* data;
};

struct xbee_frame load_xbee_frame(uint8_t* data, uint64_t len)
{
  struct xbee_frame xfrm;

  xfrm.type = data[0];
  xfrm.src_addr.bytes[7] = data[1];
  xfrm.src_addr.bytes[6] = data[2];
  xfrm.src_addr.bytes[5] = data[3];
  xfrm.src_addr.bytes[4] = data[4];
  xfrm.src_addr.bytes[3] = data[5];
  xfrm.src_addr.bytes[2] = data[6];
  xfrm.src_addr.bytes[1] = data[7];
  xfrm.src_addr.bytes[0] = data[8];
  xfrm.src_net.bytes[1] = data[9];
  xfrm.src_net.bytes[0] = data[10];
  xfrm.rx_opt = data[11];
  xfrm.data = (uint8_t *) malloc(len);
  memcpy(xfrm.data,&data[12], len);

  return xfrm;
}

#endif