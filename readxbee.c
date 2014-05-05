#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include "readxbee.h"

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE B9600            
/* change this definition for the correct port */
#define MODEMDEVICE "/dev/ttyUSB2"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

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
/* 
  Open modem device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/
 fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
 if (fd <0) {perror(MODEMDEVICE); exit(-1); }

 tcgetattr(fd,&oldtio); /* save current serial port settings */
 bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

/* 
  BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
  CRTSCTS : output hardware flow control (only used if the cable has
            all necessary lines. See sect. 7 of Serial-HOWTO)
  CS8     : 8n1 (8bit,no parity,1 stopbit)
  CLOCAL  : local connection, no modem contol
  CREAD   : enable receiving characters
*/
 newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
 
/*
  IGNPAR  : ignore bytes with parity errors
  ICRNL   : map CR to NL (otherwise a CR input on the other computer
            will not terminate input)
  otherwise make device raw (no other input processing)
*/
 newtio.c_iflag = IGNPAR | ICRNL;
 
/*
 Raw output.
*/
 newtio.c_oflag = 0;
 
/*
  ICANON  : enable canonical input
  disable all echo functionality, and don't send signals to calling program
*/
 newtio.c_lflag = 0; // ICANON;
 
/* 
  initialize all control characters 
  default values can be found in /usr/include/termios.h, and are given
  in the comments, but we don't need them here
*/
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

/* 
  now clean the modem line and activate the settings for the port
*/
 tcflush(fd, TCIFLUSH);
 tcsetattr(fd,TCSANOW,&newtio);

/*
  terminal settings done, now handle input
  In this example, inputting a 'z' at the beginning of a line will 
  exit the program.
*/
 while (STOP==FALSE) {     /* loop until we have a terminating condition */
 /* read blocks program execution until a line terminating character is 
    input, even if more than 255 chars are input. If the number
    of characters read is smaller than the number of chars available,
    subsequent reads will return the remaining chars. res will be set
    to the actual number of characters actually read */
    res = read(fd,buf,255);
    if (res>0) {
      buf[res]=0;             /* set end of string, so we can printf */
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
 /* restore the old port settings */
 tcsetattr(fd,TCSANOW,&oldtio);

 return (EXIT_SUCCESS);
}