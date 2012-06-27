#include "packets.h"
#include <qextserialport.h>
#include <sys/time.h>

namespace Mav {
namespace Pelican {

#define PELICAN_READ_TIMEOUT    1000
#define PELICAN_WRITE_TIMEOUT   1000

enum BaudType {
  BAUD_9600, BAUD_14400, BAUD_19200, BAUD_38400,
  BAUD_56000, BAUD_57600, BAUD_115200
};
enum BitsType { DATA5, DATA6, DATA7, DATA8 };

enum FlowTyp { FLOWOFF, FLOWHARDWARE, FLOWXONXOFF };

enum  ParityTyp {P_NONE, P_ODD, P_EVEN, P_MARK, P_SPACE };

enum  	StopBits { STOP1, STOP1_5, STOP2 };

long timevaldiff(struct timeval *starttime, struct timeval *finishtime);

bool initserial(const char *port, BaudType baud, BitsType databits, ParityTyp parity, StopBits stopbits,FlowTyp flowcontrol,double timeoutSeconds);
bool closeserial();
bool flush_port();
bool readData(char *data, int len);
bool writeData(char *data, int len);
unsigned short ntohUShort(unsigned short s);

}
}
