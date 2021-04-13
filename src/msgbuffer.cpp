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
        Serial.print(b);
#endif
        // check if start of new packet
        if (b == '%' || b == '$') {
            _index = 0;
            _packet.buffer[3] = 0;
            _packet.buffer[4] = 0;
            _packet.buffer[_index++] = b;
#ifdef DEBUG
            Serial.println(" - start");
#endif
        }
        // check if end of packet
        else if (_index > 2 && b == ';') {
#ifdef DEBUG
            Serial.println(" - end");
#endif
            _packet.buffer[_index++] = 0;
            _length = _index;
            _index=0;
                return true;
        }
        // check for overflow
        else if (_index == sizeof(msg_t)-1) {
#ifdef DEBUG
            Serial.println(" - overflow");
#endif
            _index = 0;
        }
        // copy into buffer
        else if (_index > 0) {
#ifdef DEBUG
            Serial.println(" - data");
#endif
            _packet.buffer[_index++] = b;
        }
    }
    return false;
}
