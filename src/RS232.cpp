#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

char   baudr[64];
HANDLE Cport[16];
char   comports[16][10] = {"\\\\.\\COM1", "\\\\.\\COM2", "\\\\.\\COM3", "\\\\.\\COM4",
                           "\\\\.\\COM5", "\\\\.\\COM6", "\\\\.\\COM7", "\\\\.\\COM8",
                           "\\\\.\\COM9", "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                           "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16"};

int RS232_OpenComport(int comport_number, int baudrate)
{
    if ((comport_number > 15) || (comport_number < 0)) {
        printf("illegal comport number\n\n");
        return (1);
    }

    switch (baudrate) {
    case 110:
        strcpy(baudr, "baud=110 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 300:
        strcpy(baudr, "baud=300 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 600:
        strcpy(baudr, "baud=600 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 1200:
        strcpy(baudr, "baud=1200 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 2400:
        strcpy(baudr, "baud=2400 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 4800:
        strcpy(baudr, "baud=4800 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 9600:
        strcpy(baudr, "baud=9600 data=8 parity=N stop=1 dtr=off rts=off");
        break;
    case 19200:
        strcpy(baudr, "baud=19200 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 38400:
        strcpy(baudr, "baud=38400 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 57600:
        strcpy(baudr, "baud=57600 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 115200:
        strcpy(baudr, "baud=115200 data=8 parity=N stop=1 dtr=off rts=off");
        break;
    case 128000:
        strcpy(baudr, "baud=128000 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 256000:
        strcpy(baudr, "baud=256000 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 500000:
        strcpy(baudr, "baud=500000 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    case 1000000:
        strcpy(baudr, "baud=1000000 data=8 parity=N stop=1 dtr=on rts=on");
        break;
    default:
        printf("invalid baudrate\n");
        return (1);
        break;
    }

    Cport[comport_number] = CreateFileA(comports[comport_number],
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,    /* no share  */
                                        NULL, /* no security */
                                        OPEN_EXISTING,
                                        0,     /* no threads */
                                        NULL); /* no templates */

    if (Cport[comport_number] == INVALID_HANDLE_VALUE) {
        printf("unable to open comport!\n\n");
        return (1);
    }

    DCB port_settings;
    memset(&port_settings, 0, sizeof(port_settings)); /* clear the new struct  */
    port_settings.DCBlength = sizeof(port_settings);

    if (!BuildCommDCBA(baudr, &port_settings)) {
        printf("unable to set comport dcb settings\n\n");
        CloseHandle(Cport[comport_number]);
        return (1);
    }

    if (!SetCommState(Cport[comport_number], &port_settings)) {
        printf("unable to set comport cfg settings\n\n");
        CloseHandle(Cport[comport_number]);
        return (1);
    }

    COMMTIMEOUTS Cptimeouts;

    Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
    Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
    Cptimeouts.ReadTotalTimeoutConstant    = 0;
    Cptimeouts.WriteTotalTimeoutMultiplier = 0;
    Cptimeouts.WriteTotalTimeoutConstant   = 0;

    if (!SetCommTimeouts(Cport[comport_number], &Cptimeouts)) {
        printf("unable to set comport time-out settings\n\n");
        CloseHandle(Cport[comport_number]);
        return (1);
    }

    return (0);
}

int RS232_PollComport(int comport_number, unsigned char *buf, int size)
{
    int n;

    if (size > 4096) size = 4096;
    /* added the void pointer cast, otherwise gcc will complain about */
    /* "warning: dereferencing type-punned pointer will break strict aliasing rules" */

    ReadFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL);

    return (n);
}

int RS232_ReadBlock(int comport_number, unsigned char *buf, size_t size, int timeout)
{

    size_t read_left = size;
    size_t len;
    time_t t_start = time(NULL);

    while (read_left > 0) {
        do {
            ReadFile(Cport[comport_number], &buf[size - read_left], read_left, (LPDWORD)((void *)&len), NULL);
        } while (len < 0 && errno == EINTR);
        if (len < 0) {
            printf("error reading serial port: %d (%d)\n", len, errno);
            exit(1);
        }
        if (timeout > 0) {
            if ((time(NULL) - t_start) > timeout) {
                printf("timeout reading serial port\n");
                exit(2);
            }
        }
        read_left -= len;
    }
    return size;
}

int RS232_SendByte(int comport_number, unsigned char byte)
{
    int n;

    WriteFile(Cport[comport_number], &byte, 1, (LPDWORD)((void *)&n), NULL);

    if (n < 0) return (1);

    return (0);
}

int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
    int n;

    if (WriteFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL)) {
        return (n);
    }

    return (-1);
}

void RS232_CloseComport(int comport_number)
{
    CloseHandle(Cport[comport_number]);
}

int RS232_IsCTSEnabled(int comport_number)
{
    int status;

    GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

    if (status & MS_CTS_ON)
        return (1);
    else
        return (0);
}

int RS232_IsDSREnabled(int comport_number)
{
    int status;

    GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

    if (status & MS_DSR_ON)
        return (1);
    else
        return (0);
}

void RS232_enableDTR(int comport_number)
{
    EscapeCommFunction(Cport[comport_number], SETDTR);
}

void RS232_disableDTR(int comport_number)
{
    EscapeCommFunction(Cport[comport_number], CLRDTR);
}

void RS232_enableRTS(int comport_number)
{
    EscapeCommFunction(Cport[comport_number], SETRTS);
}

void RS232_disableRTS(int comport_number)
{
    EscapeCommFunction(Cport[comport_number], CLRRTS);
}

void RS232_cputs(int comport_number, const char *text) /* sends a string to serial port */
{
    while (*text != 0)
        RS232_SendByte(comport_number, *(text++));
}

#ifdef __cplusplus
} /* extern "C" */
#endif
