
#ifndef __UART_H
#define __UART_H

extern void UART_SendPacket_raw(void *data, unsigned short count);

extern void UARTInitialize(unsigned int);
extern void UART1Initialize(unsigned int baud);
extern void UARTWriteChar(unsigned char);
extern void UART1WriteChar(unsigned char);
extern unsigned char UARTReadChar(void);
extern unsigned char UART1ReadChar(void);
extern void __putchar(int);
extern void UART_send(char *, unsigned char);
extern void UART1_send(unsigned char *, unsigned char);
extern void mdv_output(unsigned int);
extern void UART_send_ringbuffer(void);
extern void UART1_send_ringbuffer(void);
extern int ringbuffer(unsigned char, unsigned char*, unsigned int);
extern int ringbuffer1(unsigned char, unsigned char*, unsigned int);
extern void uart0ISR(void);
extern void uart1ISR(void);
extern void UART_SendPacket(void *, unsigned short, unsigned char);
extern void check_chksum(void);
extern void GPS_configure(void);

extern unsigned char send_buffer[16];
extern unsigned short crc16(void *, unsigned short);
extern unsigned short crc_update (unsigned short, unsigned char);
extern unsigned char chksum_trigger;
extern unsigned char UART_CalibDoneFlag;
extern unsigned char trigger_transmission;
extern unsigned char transmission_running;

#define RBREAD 0
#define RBWRITE 1
#define RBFREE  2 
#define RINGBUFFERSIZE	384

#define RX_IDLE 0
#define RX_ACTSYNC1 1
#define RX_ACTSYNC2 2
#define RX_ACTDATA 3
#define RX_ACTCHKSUM 4

#define GPSCONF_TIMEOUT 200

#endif //__UART_H

