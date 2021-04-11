#pragma once
#include <Arduino.h>
#include "config.h"

typedef struct {
    private:
        char _cmd;
        char _reg;
        char _idx;
        char _col;
        char _arg[16];
    public:
        // get
        char cmd() { return _cmd; };
        char reg() { return _reg; };
        uint8_t idx() { return _idx - '1'; }
        bool col_empty() { return _col == 0; }
        bool has_arg() { return (_arg[0] != 0 && _col == ':'); }
        bool arg_bool() { return _arg[0] == 0; }
        int32_t arg_int32() { return atol(_arg); }
        float arg_float() { return atof(_arg); }
        // set
        //void cmd(char c) { _cmd = c; }
        //void reg(char c) { _reg = c; }
        //void idx(uint8_t i) { _idx = i + '0'; }
        //void arg(bool b) { _col = ':'; _arg[0]= b; _arg[1]=0; }
        //void arg(int32_t i) { _col = ':'; itoa(i, _arg, 10); }
        
} msg_t;

typedef union {
    msg_t msg;
    char buffer[sizeof(msg_t)];
} msg_buffer_t;

class MsgBuffer
{
    private:
        msg_buffer_t _packet;
        uint8_t _index = 0;
        uint8_t _length = 0;
    public:
        MsgBuffer(String name);
        bool parse(Stream &stream);
        msg_t getMsg() { return _packet.msg; }
};

