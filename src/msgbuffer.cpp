#include "msgbuffer.h"

MsgBuffer::MsgBuffer(String name)
{
    Serial.print("# Initializing packet parser ");
    Serial.println(name);
}

bool MsgBuffer::parse(Stream &stream) {
    while(stream.available()) {
        char b = stream.read();
#ifdef DEBUG
        stream.print(b);
#endif DEBUG
        // check if start of new packet
        if (b == '%' || b == '$') {
            _index = 0;
            _packet.buffer[3] = 0;
            _packet.buffer[4] = 0;
            _packet.buffer[_index++] = b;
#ifdef DEBUG
            stream.println(" - start");
#endif
        }
        // check if end of packet
        else if (_index > 2 && b == ';') {
#ifdef DEBUG
            stream.println(" - end");
#endif
            _packet.buffer[_index++] = 0;
            _length = _index;
            _index=0;
                return true;
        }
        // check for overflow
        else if (_index == sizeof(msg_t)-1) {
#ifdef DEBUG
            stream.println(" - overflow");
#endif
            _index = 0;
        }
        // copy into buffer
        else if (_index > 0) {
#ifdef DEBUG
            stream.println(" - data");
#endif
            _packet.buffer[_index++] = b;
        }
    }
    return false;
}
