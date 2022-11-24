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

#define MAX_FILE_SIZE 0xFFFF

void usage_help(const char *prgname);

/* Command line parameter
 *
 * argv[0]----.exe file name
 * argv[1]----serial port
 * argv[2]----baud rate
 * argv[3]----device,
 * argv[4]----filename
 *
 */

int main(int arg, char *argv[])
{
    int         com      = atoi(argv[1]) - 1;
    int         baud     = atoi(argv[2]);
    int         device   = atoi(argv[3]);
    const char *filename = argv[4];

    FILE         *pfile      = NULL;
    unsigned char cmd_buf[5] = {0, 0, 0, 0, 0};
    unsigned char buf[MAX_FILE_SIZE];
    int           end   = 0;
    long          fsize = 0;

    if (arg <= 4) usage_help(argv[0]);

    int read_size   = 0;
    int start_block = 0;
    if (strlen(filename) < 1) {
        printf("Invalid filename: %s\n", filename);
        exit(1);
    }
    pfile = fopen(filename, "rb"); // Read mode
    if (NULL == pfile) {
        printf("Cannot open file: %s (%d)\n", filename, errno);
        exit(1);
    }

    // calculate the size of the file
    fseek(pfile, 0, SEEK_SET);
    fseek(pfile, 0, SEEK_END);
    fsize = ftell(pfile);
    fseek(pfile, 0, SEEK_SET);
    if (fsize < 1) {
        printf("Cannot send empty file!\n");
        exit(1);
    }
    if (fsize > MAX_FILE_SIZE) {
        printf("File too big to copy into internal buffer!\n");
        exit(1);
    }
    fread(buf, fsize, 1, pfile);
    fclose(pfile);
    printf("We have copied the file into the buffer with length of %d Bytes\n", fsize);

    // open COM port for sending the commands to MF board
    if (1 == RS232_OpenComport(com, baud)) {
        // printf("Error open ComPort!\n");
        printf("Just printing to terminal!\n");
    } else {
        printf("Serial port: COM%d\n", com + 1);
        if (device == 0) {
            printf("Device  : Default (e.g. UNO)\n");
            printf("Baud:%d data:8 parity:none stopbit:1 DTR:off RTS:off\n", baud);
            RS232_disableDTR(com);
        } else {
            printf("Device: ProMicro\n");
            printf("Baud:%d data:8 parity:none stopbit:1 DTR:on RTS:off\n", baud);
            RS232_enableDTR(com);
        }
        RS232_disableRTS(com);
        Sleep(1000); // wait to boot up the Firmware
    }

    unsigned long adress             = 0;
    char          charDelayTime[100] = {0};

    printf("\nStarting to send the commands!\n\n");

    do {
        // send all characters (except spaces) including the command terminating ";"
        if (buf[adress] != ';') {
            // do not send the space character
            if (buf[adress] != ' ') {
                printf("%c", buf[adress]);
                if (RS232_SendByte(com, buf[adress])) {
                    printf("Communication failure!\n");
                    break;
                }
            }
            adress++;
        } else {
            // print also the ';'
            printf("%c", buf[adress]);
            if (RS232_SendByte(com, buf[adress++])) {
                printf("Communication failure!\n");
                break;
            }

            // next read in the delay time
            unsigned short i = 0;
            while (buf[adress++] != ':') {
                charDelayTime[i++] = buf[adress];
            }
            // terminate with 0 as the ':' was also copied
            charDelayTime[i] = 0;
            // convert string into long
            unsigned long delayTime = atoi(charDelayTime);
            // and do not send it to the COM port, just the terminal
            printf("   -> Delay: %dms\n", delayTime);
            Sleep(delayTime);
            // skip CR and LF
            if (buf[adress] == 13) adress++;
            if (buf[adress] == 10) adress++;
        }
    } while (adress < fsize);

    printf("\nAll commands send!\n");

    RS232_CloseComport(com);

    exit(1);
}

void usage_help(const char *prgname)
{
    printf("\n\n    Invalid parameters!\n");
    printf("        Usage: %s <serialport> <baud> <txt file>\n", prgname);
    printf("        Example 1:\n");
    printf("            Reading test.txt and sending to COM3 with 115200 baud to ProMicro\n");
    printf("            %s 3 115200 1 test.txt\n", prgname);
    printf("        Example 2:\n");
    printf("            Reading test.txt and sending to COM3 with 115200 baud to Mega/Uno\n");
    printf("            %s 3 115200 0 test.txt\n\n\n", prgname);
    exit(1);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
