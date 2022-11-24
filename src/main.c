#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "RS232.h"


#define SBEGIN  0x01
#define SDATA   0x02
#define SRSP    0x03
#define SEND    0x04
#define ERRO    0x05
#define CHIP_ID 0x11
#define SDUMP   0x12
#define FBLOCK  0x13

FILE *pfile = NULL;
long fsize = 0;
int BlkTot = 0;
int Remain = 0;
int BlkNum = 0;
int DownloadProgress = 0;
int com = -1;
int end = 0;
int baud = 0;

void ProcessProgram(int cmd);

/*
* argv[0]----.exe file name
* argv[1]----serial port
* argv[2]----baud rate
* argv[3]----serial device
* argv[4]----command: id, read, write
* argv[5]----filename
* argv[6]----verify mode | start block
* argv[7]----size of blocks
*/

#define CMD_ID 1
#define CMD_READ 2
#define CMD_WRITE 3


void usage_help(const char *prgname)
{
  printf("Invalid parameters.\n");
  printf("Usage: %s <serialport> <baud> <device> <command> [<bin file> [<verify> | <read_start_block> <read_blocks>]]\n",
	 prgname);
  printf("Examples:\n");
  printf("Read Chip ID: %s 3 115200 1 id\n", prgname);
  printf(" Write Flash: %s 5 115200 1 write flash.bin 1\n", prgname);
  printf("  Read Flash: %s 7 115200 1 read dump.bin 0 512\n", prgname);
  printf("Example: %s 11 57600 1 id\n", prgname);
  printf("        <Baudrate>:  57600 -- ATmega328 on 8 MHz\n");
  printf("                    115200 -- ESP8266 / ESP32\n");
  printf("          <device>: 0 -- Default (e.g. UNO)\n");
  printf("                    1 -- Leonardo/Mini Pro/etc...\n");
  printf("         <command>: id    -- read chip ID");
  printf("                    write -- flash file\n");
  printf("                    read  -- read file, needs start block and number of blocks\n");
  printf("          <verify>: 0 -- No verify (default)\n");
  printf("                    1 -- Verify (when flashing)\n");
  printf("<read_start_block>: 0 -- Start flash dump from block (0 = beginning of the flash)\n");
  printf("     <read_blocks>: 512 -- How many blocks to read (512 = 256Kb)\n");
  exit(1);
}

int main(int arg, char *argv[])
{	
  int device = 0;
  unsigned char cmd_buf[5] = { 0,0,0,0,0 };
  int verify = 0;

  const char *command = argv[4];
  const char *filename = argv[5];
  
  if (arg <=4) usage_help(argv[0]);

  if (strncmp(command,"id",2)==0) {             // ------------------ CHIP ID Mode -------------------
    cmd_buf[0]=CHIP_ID;
  }
  else if (strncmp(command,"read",4)==0) {      // ------------------ READ Mode -------------------
    int read_size = 0;
    int start_block = 0;
    if (arg <= 7) usage_help(argv[0]);
    if (strlen(filename) < 1) {
      printf("invalid filename: %s\n",filename);
      return 1;
    }
    pfile = fopen(filename, "wb");              // Read mode so create new file for to save flash into...
    if(NULL == pfile) {
      printf("cannot create file: %s (%d)\n",filename,errno);
      return 1;
    }
    read_size = atoi(argv[7]);
    if (read_size < 1 || read_size > 1024) {
      printf("Invalid number of blocks to read  specified: %d\n",read_size);
      return 1;
    }
    start_block=verify;
    if (start_block < 0 || start_block > 1024) {
      printf("Invalid starting block specified: %d\n", start_block);
      return 1;
    }
    BlkTot=read_size;
    fsize=BlkTot*512;
    cmd_buf[0]=SDUMP;
    cmd_buf[1]=(BlkTot >> 8) & 0xff;
    cmd_buf[2]=BlkTot & 0xff;
    cmd_buf[3]=(start_block >> 8) & 0xff;
    cmd_buf[4]=start_block & 0xff;
  }
  else if (strncmp(command,"write",5)==0) {   // ------------------ WRITE Mode -------------------
    if (arg <= 6) usage_help(argv[0]);
    if (strlen(filename) < 1) {
      printf("invalid filename: %s\n",filename);
      return 1;
    }
    pfile = fopen(filename, "rb");            // read only file
    if(NULL == pfile) {
      printf("cannot create file: %s (%d)\n",filename,errno);
      return 1;
    }
    verify=atoi(argv[6]);
    cmd_buf[0]=SBEGIN;
    cmd_buf[1]=(verify > 0 ? 1 : 0);
    if (verify > 0) {
      printf("Verify enabled (flashing process will take longer)\n");
    }
    fseek(pfile,0,SEEK_SET);
    fseek(pfile,0,SEEK_END);
    fsize = ftell(pfile);
    fseek(pfile,0,SEEK_SET);
    if (fsize < 1 && cmd_buf[0] != CHIP_ID) {
      printf("Cannot flash empty file!\n");
      return 1;
    }
    Remain = fsize % 512;
    if(Remain != 0) {
      BlkTot = fsize / 512 + 1;
      printf("Warning: file size isn't the integer multiples of 512, last bytes will be set to 0xFF\n");
    } else {
      BlkTot = fsize / 512;
    }
  }
  else {
    printf("Invalid command: %s\n",command);
    return 1;
  }

  com = atoi(argv[1]) - 1;
  baud = atoi(argv[2]);
  if(1 == RS232_OpenComport(com, baud)) {
    return 1;	// Open comport error
  }
  printf("Serial port: COM%d\n",com+1);
  device = atoi(argv[3]);
  if(device == 0) {
    printf("Device  : Default (e.g. UNO)\n");
    printf("Baud:%d data:8 parity:none stopbit:1 DTR:off RTS:off\n", baud);
    RS232_disableDTR(com);
  }  else {
    printf("Device: Leonardo\n");
    printf("Baud:%d data:8 parity:none stopbit:1 DTR:on RTS:off\n", baud);
    RS232_enableDTR(com);
  }
  RS232_disableRTS(com);
  sleep(1);   //wait to stabilize UART Port, required for ATmega328 on 8MHz

  if (cmd_buf[0] != CHIP_ID) {
    printf("Image file: %s\n",filename);
    printf("Total blocks: %d (%ld bytes)\n", BlkTot, fsize);
  }
  BlkNum = 0;

  printf("\nSend command: %s\n",command);
  
  if(RS232_SendBuf(com, cmd_buf, sizeof(cmd_buf)) != sizeof(cmd_buf)) {
    printf("Communication failure!\n");
    if (cmd_buf[0] != CHIP_ID){
      fclose(pfile);
    }
    RS232_CloseComport(com);
    return 1;
  }
  
  printf("Waiting for response...\n\n");
	
  while(!end) {
    ProcessProgram(cmd_buf[0]);
  }
  printf("\nClosing serial port\n");
  BlkNum = 0;
  DownloadProgress = 0;
  if (cmd_buf[0] != CHIP_ID){
    fclose(pfile);
  }
  RS232_CloseComport(com);
  
  return 0;
}

void ProcessProgram(int cmd)
{
  int len;
  unsigned char rx;
  len = RS232_PollComport(com, &rx, 1);
  if(len > 0) {
    switch(rx) {
      case CHIP_ID:
        {
          unsigned char buf[2];
          len = RS232_ReadBlock(com, buf, 2, 5);
          if (len < 2) {
            end = 1;
            printf("Did not receive chip ID (%d)\n", len);
          } else {
            printf("      Chip ID: 0x%x (CC25%02x)\n",buf[0],buf[0]);
            printf("Chip Revision: 0x%x\n\n",buf[1]);
            if (cmd == CHIP_ID) {
              end = 1;
            }
          }
        }
        break;
      case FBLOCK:
        {
          unsigned char buf[514];
          size_t len;
          unsigned short checksum1 = 0;
          unsigned short checksum2 = 0;
          int i;

          memset(buf,0,sizeof(buf));
          BlkNum++;
          if (BlkNum == 1)
            printf("Reading flash...");
          printf(" %d",BlkNum);
          fflush(stdout);

          len = RS232_ReadBlock(com, buf, 514, 10);
          for (i=0; i<512; i++) {
            checksum1 += buf[i];
          }
          checksum2 = (buf[512]<<8 | buf[513]);
          if (checksum1 != checksum2) {
            printf("\nBlock %d: checksum mismatch: %04x vs %04x\n",BlkNum,checksum1,checksum2);
          }
          
          len=fwrite(buf,1,512,pfile);
          if (len < 512) {
            printf("failed to write block to file: %d\n",errno);
            exit(1);
          }

          if (BlkNum >= BlkTot) {
            printf("\nFlash Dump Complete\n");
            end = 1;
            break;
          }
        }
        break;
        
      case SRSP:
        {
          if (BlkNum == BlkTot) {
            unsigned char temp = SEND;
            RS232_SendByte(com, temp);
            end = 1;
            printf("\nEnd programming\n");
          } else  {
            if(BlkNum == 0) {	
              printf("Begin programming...\n");
            }
            DownloadProgress = 1;
            unsigned char buf[515];
            buf[0] = SDATA;
            if((BlkNum == (BlkTot-1)) && (Remain != 0)) {
              fread(buf+1, Remain, 1, pfile);
              int filled = 512 - Remain;
              //int i = 0;
              for(int i = 0; i<filled; i++) {
                buf[Remain+1+i] = 0xFF;
              }
            } else {
              fread(buf+1, 512, 1, pfile);
            }
            unsigned short CheckSum = 0x0000;
            for(unsigned int i=0; i<512; i++) {
              CheckSum += (unsigned char)buf[i+1];
            }
            buf[513] = (CheckSum >> 8) & 0x00FF;
            buf[514] = CheckSum & 0x00FF;
            RS232_SendBuf(com, buf, 515);
            BlkNum++;
            printf("%d  ", BlkNum);
            fflush(stdout);
          }
        }
        break;
      case ERRO:
        {
          if(DownloadProgress == 1) {
            end = 1;
            printf("Verify failed!\n");
          } else {
            end = 1;
            printf("No chip detected!\n");
          }
        }
        break;
      default:
        printf("Unknown response: %x\n",rx);
        break;
      }
    len = 0;
  }
}



#ifdef __cplusplus
} /* extern "C" */
#endif
