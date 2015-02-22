#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include "readxbee.h"

#define BAUDRATE B9600            
#define MODEMDEVICE "/dev/ttyUSB2"
#define _POSIX_SOURCE 1 

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

int main(int argc, char** argv)
{
  int fd, i, res;
  unsigned char start_byte;
  unsigned int len;
  struct termios oldtio,newtio;
  uint8_t buf[255];
  struct xbee_frame xfrm;
  uint8_t* data_for_chksum;
  uint8_t chksum;

  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
  if (fd <0) {
    perror(MODEMDEVICE); 
    exit(-1);
  }

 tcgetattr(fd,&oldtio);
 bzero(&newtio, sizeof(newtio));
 newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
 newtio.c_iflag = IGNPAR | ICRNL;
 newtio.c_oflag = 0;
 newtio.c_lflag = 0; // ICANON;
 newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
 newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
 newtio.c_cc[VERASE]   = 0;     /* del */
 newtio.c_cc[VKILL]    = 0;     /* @ */
 newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
 newtio.c_cc[VTIME]    = 1;     /* inter-character timer unused */
 newtio.c_cc[VMIN]     = 255;     /* blocking read until 1 character arrives */
 newtio.c_cc[VSWTC]    = 0;     /* '\0' */
 newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
 newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
 newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
 newtio.c_cc[VEOL]     = 0;     /* '\0' */
 newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
 newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
 newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
 newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
 newtio.c_cc[VEOL2]    = 0;     /* '\0' */

 tcflush(fd, TCIFLUSH);
 tcsetattr(fd,TCSANOW,&newtio);
 while (STOP==FALSE) {
    res = read(fd,buf,255);
    if (res>0) {
      buf[res]=0;
      for (i=0;i<res;i++) {
        if (buf[i] == 0x7E) {
          start_byte = i;
          len = (buf[i+1] << 8) + buf[i+2];
        }
        printf("%02X ", (unsigned char)buf[i]);
      }
      printf("\nFRAME LEN: %i BYTE\n", len);
      if (len > 0) {
        
        xfrm = load_xbee_frame(&buf[start_byte + 3], len);
        printf("FRAME TYPE: 0x%02X\n", xfrm.type);
        printf("SRC ADDR: 0x%jX\n", xfrm.src_addr.value);
        printf("SRC NET: 0x%02X\n", xfrm.src_net.value);
        printf("RECEIVE OPTS: 0x%02X\n", xfrm.rx_opt);

        xfrm.data[len - 12] = '\0'; // Just because I know, that Data is a string and the last character is 0x0A

        printf("DATA: %s\n",xfrm.data);

        data_for_chksum = (uint8_t *) malloc(len);
        memcpy(data_for_chksum, &buf[start_byte + 3], len);
        chksum = calc_chksum(data_for_chksum, len);

        printf("CHKSUM: %02X, VERIFY: %i", chksum, verify_chksum(data_for_chksum, len, buf[start_byte + 3 + len]));
        
        printf("\n");      
      }

      len = 0;      
    } 

    if (buf[0]=='z') STOP=TRUE;
  }
 tcsetattr(fd,TCSANOW,&oldtio);

 return (EXIT_SUCCESS);
}
