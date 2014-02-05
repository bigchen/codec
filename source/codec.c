#include <memory.h>
#include "codec.h"

static const unsigned int highmask[] = {
    1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff,
    0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff,
    0x1ffff, 0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
    0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
    };

/*byte stream function*/
static void bytes_init(T_ByteStream *buf, unsigned char *msg, unsigned short totallen, unsigned char mode);
static void bytes_forward(T_ByteStream *buf, unsigned short n);
static void bytes_back(T_ByteStream *buf, unsigned short n);
static unsigned short bytes_geterror(T_ByteStream *buf);
static unsigned short bytes_getlen(T_ByteStream *buf);
static unsigned short bytes_getcurpos(T_ByteStream *buf);
static unsigned char * bytes_getbuf(T_ByteStream *buf);

/*decode function*/
static unsigned char bytes_getbyte(T_ByteStream *buf);
static unsigned short bytes_getword(T_ByteStream *buf);
static unsigned int bytes_getdword(T_ByteStream *buf);
static void bytes_getbitstream(T_ByteStream *bytes, unsigned short n, T_BitStream *bits);

/*encode function*/
static void bytes_setbyte(T_ByteStream *buf, unsigned char value);
static void bytes_setword(T_ByteStream *buf, unsigned short value);
static void bytes_setdword(T_ByteStream *buf, unsigned int value);
static void bytes_setbyteslice(T_ByteStream *buf, unsigned char begin, unsigned char end, unsigned char value);

static void bytes_setbytebypos(T_ByteStream *buf, unsigned short pos, unsigned char value);
static void bytes_setwordbypos(T_ByteStream *buf, unsigned short pos, unsigned short value);
static void bytes_setdwordbypos(T_ByteStream *buf, unsigned short pos, unsigned int value);


/*bit stream function*/
static void bits_init(T_BitStream *buf, unsigned char *msg, unsigned short totallen, unsigned char mode);
static void bits_forward(T_BitStream *buf, unsigned short n);
static unsigned short bits_geterror(T_BitStream *buf);
static unsigned short bits_getlen(T_BitStream *buf);
static unsigned short bits_getcurpos(T_BitStream *buf);
static unsigned char * bits_getbuf(T_BitStream *buf);

/*decode function*/
static unsigned int bits_getbit(T_BitStream *buf, unsigned char n);
static unsigned char bits_getbyte(T_BitStream *buf);
static unsigned short bits_getword(T_BitStream *buf);
static unsigned int bits_getdword(T_BitStream *buf);

/*encode function*/
static void bits_setbit(T_BitStream *buf, unsigned char len, unsigned int value);
static void bits_setbyte(T_BitStream *buf, unsigned char value);
static void bits_setword(T_BitStream *buf, unsigned short value);
static void bits_setdword(T_BitStream *buf, unsigned int value);

static void bits_setbitbypos(T_BitStream *buf, unsigned short pos, unsigned char len, unsigned int value);
static void bits_setbytebypos(T_BitStream *buf, unsigned short pos, unsigned char value);
static void bits_setwordbypos(T_BitStream *buf, unsigned short pos, unsigned short value);
static void bits_setdwordbypos(T_BitStream *buf, unsigned short pos, unsigned int value);


/*helper function*/
unsigned char getbyteslice(unsigned char byte, unsigned char begin, unsigned char end)
{
    unsigned char temp = 0;

    if(begin < end)
    {
        temp = begin;
        begin = end;
        end = temp;
    }

    if(begin > 7)
    {
        begin = 7;
    }

    return (byte & (unsigned char)(highmask[begin])) >> end;
}

unsigned char getbytehi(unsigned char byte, unsigned char n)
{
    return getbyteslice(byte, 7, 8 - n);
}

unsigned char getbytelow(unsigned char byte, unsigned char n)
{
    return getbyteslice(byte, n - 1, 0);
}

void setbyteslice(unsigned char *byte, unsigned char begin, unsigned char end, unsigned char value)
{
    unsigned char temp = 0;
    unsigned char temp_hi, temp_low;

    if(begin < end)
    {
        temp = begin;
        begin = end;
        end = temp;
    }

    if(begin > 7)
    {
        begin = 7;
    }

    temp_hi = getbytehi(*byte, 7 - begin);
    temp_low = getbytelow(*byte, end);

    *byte = (temp_hi << (begin + 1)) | (value << end) | (temp_low);

}

void setbytehi(unsigned char *byte, unsigned char n, unsigned char value)
{
    setbyteslice(byte, 7, 8 - n, value);
}

void setbytelow(unsigned char *byte, unsigned char n, unsigned char value)
{
    setbyteslice(byte, n - 1, 0, value);
}

unsigned short getshortslice(unsigned short value, unsigned char begin, unsigned char end)
{
    unsigned char temp = 0;

    if(begin < end)
    {
        temp = begin;
        begin = end;
        end = temp;
    }

    return (value & (unsigned short)(highmask[begin])) >> end;

}

unsigned int getintslice(unsigned int value, unsigned char begin, unsigned char end)
{
    unsigned char temp = 0;

    if(begin < end)
    {
        temp = begin;
        begin = end;
        end = temp;
    }

    return (value & highmask[begin]) >> end;

}

void bytes_init(T_ByteStream *buf, unsigned char *msg, unsigned short totallen, unsigned char mode)
{
    buf->buffer = msg;
    buf->totallen = totallen;
    buf->curbyte = 0;
    buf->error = CODEC_OK;

    if(mode == CODEC_ENCODE)
    {
        memset(buf->buffer, 0, totallen);
    }
}

void bytes_forward(T_ByteStream *buf, unsigned short n)
{
    if(buf->curbyte + n >= buf->totallen)
    {
        buf->error = CODEC_MOVETOOBITS;
        return;
    }

    buf->curbyte += n;

}

void bytes_back(T_ByteStream *buf, unsigned short n)
{
    if(buf->curbyte < n)
    {
        buf->error = CODEC_MOVETOOBITS;
        return;
    }

    buf->curbyte -= n;
}

unsigned short bytes_geterror(T_ByteStream *buf)
{
    return buf->error;
}

unsigned short bytes_getlen(T_ByteStream *buf)
{
    return buf->curbyte;
}

unsigned short bytes_getcurpos(T_ByteStream *buf)
{
    return buf->curbyte;
}

unsigned char * bytes_getbuf(T_ByteStream *buf)
{
    return buf->buffer;
}

unsigned char bytes_getbyte(T_ByteStream *buf)
{
    unsigned char r = 0;

    if(buf->curbyte + 1 > buf->totallen)
    {
        buf->error = CODEC_GETTOOBITS;
        return 0;
    }

    r = buf->buffer[buf->curbyte];
    buf->curbyte ++;
    return r;
}

unsigned short bytes_getword(T_ByteStream *buf)
{
    unsigned char n1, n2;

    if(buf->curbyte + 2 > buf->totallen)
    {
        buf->error = CODEC_GETTOOBITS;
        return 0;
    }

    n1 = bytes_getbyte(buf);
    n2 = bytes_getbyte(buf);

    return (n1<<8) | n2;
}

unsigned int bytes_getdword(T_ByteStream *buf)
{
    unsigned char n1, n2, n3, n4;

    if(buf->curbyte + 4 > buf->totallen)
    {
        buf->error = CODEC_GETTOOBITS;
        return 0;
    }

    n1 = bytes_getbyte(buf);
    n2 = bytes_getbyte(buf);
    n3 = bytes_getbyte(buf);
    n4 = bytes_getbyte(buf);

    return (n1<<24) | (n2<<16) | (n3<<8) | n4;
}

void bytes_getbitstream(T_ByteStream *bytes, unsigned short n, T_BitStream *bits)
{
    bits_init(bits, &bytes->buffer[bytes->curbyte], n, CODEC_DECODE);
}

void bytes_setbyte(T_ByteStream *buf, unsigned char value)
{
    if(buf->curbyte + 1 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    buf->buffer[buf->curbyte] = value;
    buf->curbyte ++;
    return;
}

void bytes_setword(T_ByteStream *buf, unsigned short value)
{
    unsigned char hi, low;

    if(buf->curbyte + 2 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 8;
    low = value & 0xFF;
    bytes_setbyte(buf, hi);
    bytes_setbyte(buf, low);
    return;

}

void bytes_setdword(T_ByteStream *buf, unsigned int value)
{
    unsigned short hi, low;

    if(buf->curbyte + 4 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 16;
    low = value & 0xFFFF;
    bytes_setword(buf, hi);
    bytes_setword(buf, low);
    return;

}

void bytes_setbyteslice(T_ByteStream *buf, unsigned char begin, unsigned char end, unsigned char value)
{
    unsigned char temp = 0;

    if(begin < end)
    {
        temp = begin;
        begin = end;
        end = temp;
    }

    if(begin > 7)
    {
        buf->error = CODEC_SETTOOBITS;
        begin = 7;
    }

    if(buf->curbyte == 0)
        return;

    setbyteslice(&buf->buffer[buf->curbyte - 1], begin, end, value);
    return;
}

void bytes_setbytebypos(T_ByteStream *buf, unsigned short pos, unsigned char value)
{
    if(pos + 1 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    buf->buffer[pos] = value;
    return;
}

void bytes_setwordbypos(T_ByteStream *buf, unsigned short pos, unsigned short value)
{
    unsigned char hi, low;

    if(pos + 2 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 8;
    low = value & 0xFF;
    bytes_setbytebypos(buf, pos, hi);
    bytes_setbytebypos(buf, (pos+1), low);
    return;
}

void bytes_setdwordbypos(T_ByteStream *buf, unsigned short pos, unsigned int value)
{
    unsigned short hi, low;

    if(pos + 4 > buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 16;
    low = value & 0xFFFF;
    bytes_setwordbypos(buf, pos, hi);
    bytes_setwordbypos(buf, (pos+2), low);
    return;
}


/*bit stream*/
void bits_init(T_BitStream *buf, unsigned char *msg, unsigned short totallen, unsigned char mode)
{
    buf->buffer = msg;
    buf->totallen = totallen;
    buf->curbyte = 0;
    buf->curbit = 0;
    buf->error = CODEC_OK;

    if(mode == CODEC_ENCODE)
    {
        memset(buf->buffer, 0, totallen);
    }
}

void bits_forward(T_BitStream *buf, unsigned short n)
{
    unsigned short curbit, curbyte;

    curbit = buf->curbit;
    curbyte = buf->curbyte;

    curbit += n;
    curbyte = curbit >> 3;

    if(curbit >= 8 * buf->totallen)
    {
        buf->error = CODEC_MOVETOOBITS;
        return;
    }

    buf->curbit = curbit;
    buf->curbyte = curbyte;

}

unsigned short bits_geterror(T_BitStream *buf)
{
    return buf->error;
}

unsigned short bits_getlen(T_BitStream *buf)
{
    //return (buf->curbit % 8 == 0) ? buf->curbyte : (buf->curbyte + 1);
    return buf->curbit;
}

unsigned short bits_getcurpos(T_BitStream *buf)
{
    return buf->curbit;
}

unsigned char * bits_getbuf(T_BitStream *buf)
{
    return buf->buffer;
}

unsigned int bits_getbit(T_BitStream *buf, unsigned char n)
{
    unsigned short firstbit, firstbyte, lastbit, lastbyte;
    unsigned char offset1, offset2, offset;
    int i;
    unsigned int r = 0;
    unsigned char count = 0;      /*1-4*/
    unsigned char bytes[8] = {0};

    if( n < 1)
    {
        buf->error = CODEC_GETZEROBITS;
        return 0;
    }

    if(n > 32)
    {
        buf->error = CODEC_GETTOOBITS;
        return 0;
    }

    firstbit = buf->curbit;
    firstbyte = buf->curbyte;

    lastbit = buf->curbit + n;
    lastbyte = lastbit >> 3;
    if(lastbit > 8 * buf->totallen)
    {
        buf->error = CODEC_GETTOOBITS;
        return 0;
    }

    buf->curbit = lastbit;
    buf->curbyte = lastbyte;

    lastbit = buf->curbit - 1;
    if(lastbit == lastbyte * 8 - 1)
        lastbyte --;

    if(firstbyte == lastbyte)
    {
        offset1 = 7 - firstbit % 8;
        offset2 = 7 - lastbit % 8;
        return getbyteslice(buf->buffer[firstbyte], offset1, offset2);
    }

    for(i=firstbyte, count=0; i<=lastbyte; i++, count++)
        bytes[count] = buf->buffer[i];

    bytes[0] = getbytelow(bytes[0], 8 - firstbit % 8);
    bytes[count - 1] = getbytehi(bytes[count - 1], lastbit % 8 + 1);

    offset = lastbit % 8 + 1;
    for(i=0, r=bytes[0]; i<count-1; i++)
    {
        r = (r << offset) + bytes[i+1];
    }

    return r;
}

unsigned char bits_getbyte(T_BitStream *buf)
{
    return bits_getbit(buf, 8);
}

unsigned short bits_getword(T_BitStream *buf)
{
    return bits_getbit(buf, 16);
}

unsigned int bits_getdword(T_BitStream *buf)
{
    return bits_getbit(buf, 32);
}

void bits_setbit(T_BitStream *buf, unsigned char len, unsigned int value)
{
    unsigned short firstbit, firstbyte, lastbit, lastbyte;
    unsigned char remain, temp;
    int i;
    unsigned char begin, end;

    if( len < 1)
    {
        buf->error = CODEC_SETZEROBITS;
        return;
    }

    if(len > 32)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    firstbit = buf->curbit;
    firstbyte = buf->curbyte;

    lastbit = buf->curbit + len;
    lastbyte = lastbit >> 3;
    if(lastbit > 8 * buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    buf->curbit = lastbit;
    buf->curbyte = lastbyte;

    lastbit = buf->curbit - 1;
    if(lastbit == lastbyte * 8 - 1)
        lastbyte --;

    remain = 8 - firstbit % 8;
    if(remain >= len)
    {
        setbyteslice(&buf->buffer[firstbyte], remain - 1, remain - len, (unsigned char)value);
        return;
    }

    if(remain > 0)
    {
        begin = len - 1;
        end = len - remain;
        temp = (unsigned char)getintslice(value, begin, end);
        setbyteslice(&buf->buffer[firstbyte], remain - 1, 0, temp);
        begin = end - 1;
        end = end > 8 ? end - 8 : 0;
    }
    else
    {
        begin = len - 1;
        end = len > 8 ? len - 8 : 0;
    }

    for(i=firstbyte+1; i<=lastbyte; i++)
    {
        temp = (unsigned char)getintslice(value, begin, end);
        if(i != lastbyte)
        {
            buf->buffer[i] = temp;
        }
        else
        {
            buf->buffer[i] = temp << (7 - lastbit % 8);
            break;
        }
        begin = end - 1;
        end = end > 8 ? end - 8 : 0;
    }

    return;
}

void bits_setbyte(T_BitStream *buf, unsigned char value)
{
    bits_setbit(buf, 8, value);
}

void bits_setword(T_BitStream *buf, unsigned short value)
{
    bits_setbit(buf, 16, value);
}

void bits_setdword(T_BitStream *buf, unsigned int value)
{
    bits_setbit(buf, 32, value);
}

void bits_setbitbypos(T_BitStream *buf, unsigned short pos, unsigned char len, unsigned int value)
{
    unsigned short firstbit, firstbyte, lastbit, lastbyte;
    unsigned char remain, temp;
    int i;
    unsigned char begin, end;

    if( len < 1)
    {
        buf->error = CODEC_SETZEROBITS;
        return;
    }

    if(len > 32)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    firstbit = pos;
    firstbyte = firstbit >> 3;

    lastbit = firstbit + len;
    lastbyte = lastbit >> 3;
    if(lastbit > 8 * buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    lastbit = lastbit - 1;
    if(lastbit == lastbyte * 8 - 1)
        lastbyte --;

    remain = 8 - firstbit % 8;
    if(remain >= len)
    {
        temp = (unsigned char)getintslice(value, len - 1, 0);
        setbyteslice(&buf->buffer[firstbyte], remain - 1, remain - len, temp);
        return;
    }

    if(remain > 0)
    {
        begin = len - 1;
        end = len - remain;
        temp = (unsigned char)getintslice(value, begin, end);
        setbyteslice(&buf->buffer[firstbyte], remain - 1, 0, temp);
        begin = end - 1;
        end = end > 8 ? end - 8 : 0;
    }
    else
    {
        begin = len - 1;
        end = len > 8 ? len - 8 : 0;
    }

    for(i=firstbyte+1; i<=lastbyte; i++)
    {
        temp = (unsigned char)getintslice(value, begin, end);
        if(i != lastbyte)
        {
            buf->buffer[i] = temp;
        }
        else
        {
            buf->buffer[i] = temp << (7 - lastbit % 8);
            break;
        }
        begin = end - 1;
        end = end > 8 ? end - 8 : 0;
    }

    return;
}

void bits_setbytebypos(T_BitStream *buf, unsigned short pos, unsigned char value)
{
    if(pos + 8 > 8 * buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    bits_setbitbypos(buf, pos, 8, value);
    return;
}

void bits_setwordbypos(T_BitStream *buf, unsigned short pos, unsigned short value)
{
    unsigned char hi, low;

    if(pos + 16 > 8 * buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 8;
    low = value & 0xff;
    bits_setbytebypos(buf, pos, hi);
    bits_setbytebypos(buf, (pos+8), low);
}

void bits_setdwordbypos(T_BitStream *buf, unsigned short pos, unsigned int value)
{
    unsigned short hi, low;

    if(pos + 32 > 8 * buf->totallen)
    {
        buf->error = CODEC_SETTOOBITS;
        return;
    }

    hi = value >> 16;
    low = value & 0xffff;
    bits_setwordbypos(buf, pos, hi);
    bits_setwordbypos(buf, (pos+16), low);
    return;
}

/*input byte stream function*/
void ibytes_init(T_InputByteStream *buf, unsigned char *msg, unsigned short totallen)
{
    bytes_init(&buf->bytes, msg, totallen, CODEC_DECODE);
}

void ibytes_forward(T_InputByteStream *buf, unsigned short n)
{
    bytes_forward(&buf->bytes, n);
}

void ibytes_back(T_InputByteStream *buf, unsigned short n)
{
    bytes_back(&buf->bytes, n);
}

unsigned short ibytes_geterror(T_InputByteStream *buf)
{
    return bytes_geterror(&buf->bytes);
}

unsigned short ibytes_getlen(T_InputByteStream *buf)
{
    return bytes_getlen(&buf->bytes);
}

unsigned short ibytes_getcurpos(T_InputByteStream *buf)
{
    return bytes_getcurpos(&buf->bytes);
}

unsigned char ibytes_getbyte(T_InputByteStream *buf)
{
    return bytes_getbyte(&buf->bytes);
}

unsigned short ibytes_getword(T_InputByteStream *buf)
{
    return bytes_getword(&buf->bytes);
}

unsigned int ibytes_getdword(T_InputByteStream *buf)
{
    return bytes_getdword(&buf->bytes);
}

void ibytes_getbitstream(T_InputByteStream *bytes, unsigned short n, T_InputBitStream *bits)
{
    bytes_getbitstream(&bytes->bytes, n, &bits->bits);
}

/*output byte stream function*/
void obytes_init(T_OutputByteStream *buf, unsigned char *msg, unsigned short totallen)
{
    bytes_init(&buf->bytes, msg, totallen, CODEC_ENCODE);
}

unsigned short obytes_geterror(T_OutputByteStream *buf)
{
    return bytes_geterror(&buf->bytes);
}

unsigned short obytes_getlen(T_OutputByteStream *buf)
{
    return bytes_getlen(&buf->bytes);
}

unsigned short obytes_getcurpos(T_OutputByteStream *buf)
{
    return bytes_getcurpos(&buf->bytes);
}

void obytes_setbyte(T_OutputByteStream *buf, unsigned char value)
{
    bytes_setbyte(&buf->bytes, value);
}

void obytes_setword(T_OutputByteStream *buf, unsigned short value)
{
    bytes_setword(&buf->bytes, value);
}

void obytes_setdword(T_OutputByteStream *buf, unsigned int value)
{
    bytes_setdword(&buf->bytes, value);
}

void obytes_setbyteslice(T_OutputByteStream *buf, unsigned char begin, unsigned char end, unsigned char value)
{
    bytes_setbyteslice(&buf->bytes, begin, end, value);
}

void obytes_setbytebypos(T_OutputByteStream *buf, unsigned short pos, unsigned char value)
{
    bytes_setbytebypos(&buf->bytes, pos, value);
}

void obytes_setwordbypos(T_OutputByteStream *buf, unsigned short pos, unsigned short value)
{
    bytes_setwordbypos(&buf->bytes, pos, value);
}

void obytes_setdwordbypos(T_OutputByteStream *buf, unsigned short pos, unsigned int value)
{
    bytes_setdwordbypos(&buf->bytes, pos, value);
}

unsigned char * obytes_getbuf(T_OutputByteStream *buf)
{
    return bytes_getbuf(&buf->bytes);
}

/*input bit stream function*/
void ibits_init(T_InputBitStream *buf, unsigned char *msg, unsigned short totallen)
{
    bits_init(&buf->bits, msg, totallen, CODEC_DECODE);
}

void ibits_forward(T_InputBitStream *buf, unsigned short n)
{
    bits_forward(&buf->bits, n);
}

unsigned short ibits_geterror(T_InputBitStream *buf)
{
    return bits_geterror(&buf->bits);
}

unsigned short ibits_getlen(T_InputBitStream *buf)
{
    return bits_getlen(&buf->bits);
}

unsigned short ibits_getcurpos(T_InputBitStream *buf)
{
    return bits_getcurpos(&buf->bits);
}

unsigned int ibits_getbit(T_InputBitStream *buf, unsigned char n)
{
    return bits_getbit(&buf->bits, n);
}

unsigned char ibits_getbyte(T_InputBitStream *buf)
{
    return bits_getbyte(&buf->bits);
}

unsigned short ibits_getword(T_InputBitStream *buf)
{
    return bits_getword(&buf->bits);
}

unsigned int ibits_getdword(T_InputBitStream *buf)
{
    return bits_getdword(&buf->bits);
}

/*output bit stream function*/
void obits_init(T_OutputBitStream *buf, unsigned char *msg, unsigned short totallen)
{
    bits_init(&buf->bits, msg, totallen, CODEC_ENCODE);
}

unsigned short obits_geterror(T_OutputBitStream *buf)
{
    return bits_geterror(&buf->bits);
}

unsigned short obits_getlen(T_OutputBitStream *buf)
{
    return bits_getlen(&buf->bits);
}

unsigned short obits_getcurpos(T_OutputBitStream *buf)
{
    return bits_getcurpos(&buf->bits);
}


void obits_setbit(T_OutputBitStream *buf, unsigned char len, unsigned int value)
{
    bits_setbit(&buf->bits, len, value);
}

void obits_setbyte(T_OutputBitStream *buf, unsigned char value)
{
    bits_setbyte(&buf->bits, value);
}

void obits_setword(T_OutputBitStream *buf, unsigned short value)
{
    bits_setword(&buf->bits, value);
}

void obits_setdword(T_OutputBitStream *buf, unsigned int value)
{
    bits_setdword(&buf->bits, value);
}

void obits_setbitbypos(T_OutputBitStream *buf, unsigned short pos, unsigned char len, unsigned int value)
{
    bits_setbitbypos(&buf->bits, pos, len, value);
}

void obits_setbytebypos(T_OutputBitStream *buf, unsigned short pos, unsigned char value)
{
    bits_setbytebypos(&buf->bits, pos, value);
}

void obits_setwordbypos(T_OutputBitStream *buf, unsigned short pos, unsigned short value)
{
    bits_setwordbypos(&buf->bits, pos, value);
}

void obits_setdwordbypos(T_OutputBitStream *buf, unsigned short pos, unsigned int value)
{
    bits_setdwordbypos(&buf->bits, pos, value);
}

unsigned char * obits_getbuf(T_OutputBitStream *buf)
{
    return bits_getbuf(&buf->bits);
}
