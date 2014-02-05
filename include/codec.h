#ifndef CODEC_H
#define CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CODEC_BUFFER_LEN  4096

#define CODEC_ENCODE            0
#define CODEC_DECODE            1

#define CODEC_OK          0
#define CODEC_GETTOOBITS  1
#define CODEC_SETTOOBITS  2
#define CODEC_MOVETOOBITS 3
#define CODEC_GETZEROBITS 4
#define CODEC_SETZEROBITS 5

/*common function*/
unsigned char getbyteslice(unsigned char byte, unsigned char begin, unsigned char end);
unsigned char getbytehi(unsigned char byte, unsigned char n);
unsigned char getbytelow(unsigned char byte, unsigned char n);
void setbyteslice(unsigned char *byte, unsigned char begin, unsigned char end, unsigned char value);
void setbytehi(unsigned char *byte, unsigned char n, unsigned char value);
void setbytelow(unsigned char *byte, unsigned char n, unsigned char value);
unsigned short getshortslice(unsigned short value, unsigned char begin, unsigned char end);
unsigned int getintslice(unsigned int value, unsigned char begin, unsigned char end);

/*byte stream*/
typedef struct tagT_ByteStream {
    unsigned char  *buffer;
    unsigned short totallen;
    unsigned short curbyte;  /*current byte index*/
    unsigned short error;
    unsigned short reserved;
} T_ByteStream;

typedef struct tagT_InputByteStream {
    T_ByteStream bytes;
} T_InputByteStream;

typedef struct tagT_OutputByteStream {
    T_ByteStream bytes;
} T_OutputByteStream;

/*bit stream*/
typedef struct tagT_BitStream {
    unsigned char  *buffer;
    unsigned short totallen;
    unsigned short curbyte; /*current byte index*/
    unsigned short curbit;  /*current bit index*/
    unsigned short error;
} T_BitStream;

typedef struct tagT_InputBitStream {
    T_BitStream bits;
} T_InputBitStream;

typedef struct tagT_OutputBitStream {
    T_BitStream bits;
} T_OutputBitStream;

/*input byte stream function*/
void ibytes_init(T_InputByteStream *buf, unsigned char *msg, unsigned short totallen);
void ibytes_forward(T_InputByteStream *buf, unsigned short n);
void ibytes_back(T_InputByteStream *buf, unsigned short n);
unsigned short ibytes_geterror(T_InputByteStream *buf);
unsigned short ibytes_getlen(T_InputByteStream *buf);
unsigned short ibytes_getcurpos(T_InputByteStream *buf);
unsigned char ibytes_getbyte(T_InputByteStream *buf);
unsigned short ibytes_getword(T_InputByteStream *buf);
unsigned int ibytes_getdword(T_InputByteStream *buf);
void ibytes_getbitstream(T_InputByteStream *bytes, unsigned short n, T_InputBitStream *bits);

/*output byte stream function*/
void obytes_init(T_OutputByteStream *buf, unsigned char *msg, unsigned short totallen);
unsigned short obytes_geterror(T_OutputByteStream *buf);
unsigned short obytes_getlen(T_OutputByteStream *buf);
unsigned short obytes_getcurpos(T_OutputByteStream *buf);
void obytes_setbyte(T_OutputByteStream *buf, unsigned char value);
void obytes_setword(T_OutputByteStream *buf, unsigned short value);
void obytes_setdword(T_OutputByteStream *buf, unsigned int value);
void obytes_setbyteslice(T_OutputByteStream *buf, unsigned char begin, unsigned char end, unsigned char value);
void obytes_setbytebypos(T_OutputByteStream *buf, unsigned short pos, unsigned char value);
void obytes_setwordbypos(T_OutputByteStream *buf, unsigned short pos, unsigned short value);
void obytes_setdwordbypos(T_OutputByteStream *buf, unsigned short pos, unsigned int value);
unsigned char * obytes_getbuf(T_OutputByteStream *buf);

/*input bit stream function*/
void ibits_init(T_InputBitStream *buf, unsigned char *msg, unsigned short totallen);
void ibits_forward(T_InputBitStream *buf, unsigned short n);
unsigned short ibits_geterror(T_InputBitStream *buf);
unsigned short ibits_getlen(T_InputBitStream *buf);
unsigned short ibits_getcurpos(T_InputBitStream *buf);
unsigned int ibits_getbit(T_InputBitStream *buf, unsigned char n);
unsigned char ibits_getbyte(T_InputBitStream *buf);
unsigned short ibits_getword(T_InputBitStream *buf);
unsigned int ibits_getdword(T_InputBitStream *buf);

/*output bit stream function*/
void obits_init(T_OutputBitStream *buf, unsigned char *msg, unsigned short totallen);
unsigned short obits_geterror(T_OutputBitStream *buf);
unsigned short obits_getlen(T_OutputBitStream *buf);
unsigned short obits_getcurpos(T_OutputBitStream *buf);
void obits_setbit(T_OutputBitStream *buf, unsigned char len, unsigned int value);
void obits_setbyte(T_OutputBitStream *buf, unsigned char value);
void obits_setword(T_OutputBitStream *buf, unsigned short value);
void obits_setdword(T_OutputBitStream *buf, unsigned int value);
void obits_setbitbypos(T_OutputBitStream *buf, unsigned short pos, unsigned char len, unsigned int value);
void obits_setbytebypos(T_OutputBitStream *buf, unsigned short pos, unsigned char value);
void obits_setwordbypos(T_OutputBitStream *buf, unsigned short pos, unsigned short value);
void obits_setdwordbypos(T_OutputBitStream *buf, unsigned short pos, unsigned int value);
unsigned char * obits_getbuf(T_OutputBitStream *buf);


#ifdef __cplusplus
}
#endif

#endif
