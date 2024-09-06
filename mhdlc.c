#include "mhdlc.h"

/* HDLC Asynchronous framing */
/* The frame boundary octet is 01111110, (7E in hexadecimal notation) */
#define FRAME_BOUNDARY_OCTET (0x7E)

/* A "control escape octet", has the bit sequence '01111101', (7D hexadecimal) */
#define CONTROL_ESCAPE_OCTET (0x7D)

/* If either of these two octets appears in the transmitted data, an escape octet is sent, */
/* followed by the original data octet with bit 5 inverted */
#define INVERT_OCTET (0x20)

/* The frame check sequence (FCS) is a 16-bit CRC-CCITT */
/* AVR Libc CRC function is _crc_ccitt_update() */
/* Corresponding CRC function in Qt (www.qt.io) is qChecksum() */
#define CRC16_CCITT_INIT_VAL (0xFFFF)

/* 16bit low and high bytes copier */
#define low(x) ((x) & 0xFF)
#define high(x) (((x) >> 8) & 0xFF)

#define lo8(x) ((x) & 0xff)
#define hi8(x) ((x) >> 8)

#define TRUE (1)
#define FALSE (0)

/*
 Polynomial: x^16 + x^12 + x^5 + 1 (0x8408) Initial value: 0xffff
 This is the CRC used by PPP and IrDA.
 See RFC1171 (PPP protocol) and IrDA IrLAP 1.1
 */
static uint16_t _crc_ccitt_update(uint16_t crc, uint8_t data)
{
    data ^= lo8(crc);
    data ^= data << 4;

    return ((((uint16_t)data << 8) | hi8(crc)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

/* Function to send a byte throug USART, I2C, SPI etc.*/
static inline void _mhdlc_sendchar(mhdlc *hdlc, uint8_t data)
{
    if (hdlc->output)
    {
        (hdlc->output)(&data, 1, hdlc->priv);
    }
}

static inline void _mhdlc_flush(mhdlc *hdlc)
{
    if (hdlc->flush)
    {
        (hdlc->flush)(hdlc->priv);
    }
}

static inline void _mhdlc_on_frame(mhdlc *hdlc, const uint8_t *frame, uint16_t size)
{
    if (hdlc->on_frame)
    {
        (hdlc->on_frame)(frame, size, hdlc->priv);
    }
}

int32_t mhdlc_init(mhdlc *hdlc, mhdlc_output output, mhdlc_flush flush, mhdlc_on_frame on_frame, uint8_t *buffer, uint16_t buffer_size, void *priv)
{
    if (hdlc == NULL || buffer == NULL)
    {
        return -1;
    }

    hdlc->output = output;
    hdlc->flush = flush;
    hdlc->on_frame = on_frame;

    hdlc->mtu = buffer_size - 1;
    hdlc->frame = buffer;

    hdlc->priv = priv;

    return 0;
}

int32_t mhdlc_input_byte(mhdlc *hdlc, uint8_t data)
{
    /* 帧头/帧尾 */
    if (data == FRAME_BOUNDARY_OCTET)
    {
        if (hdlc->escape == TRUE) // 异常状态
        {
            hdlc->escape = FALSE;
        }
        /* If a valid frame is detected */ // 遇到帧尾判断校验和
        else if ((hdlc->pos >= 2) &&
                 (hdlc->crc == ((hdlc->frame[hdlc->pos - 1] << 8) |
                                (hdlc->frame[hdlc->pos - 2] & 0xff)))) // (msb << 8 ) | (lsb & 0xff)
        {
            /* Call the user defined function and pass frame to it */
            _mhdlc_on_frame(hdlc, hdlc->frame, hdlc->pos - 2);
        }

        // 重置状态
        hdlc->pos = 0;
        hdlc->crc = CRC16_CCITT_INIT_VAL;

        return 0;
    }

    // 清除转义标志, 并且处理数据
    if (hdlc->escape)
    {
        hdlc->escape = FALSE;
        data ^= INVERT_OCTET;
    }
    else if (data == CONTROL_ESCAPE_OCTET) // 遇到转义字符
    {
        hdlc->escape = TRUE; // 设置转义标志
        return 0;
    }

    // 追加数据
    hdlc->frame[hdlc->pos] = data;

    // 更新校验和
    if (hdlc->pos >= 2)
    {
        hdlc->crc = _crc_ccitt_update(hdlc->crc,
                                      hdlc->frame[hdlc->pos - 2]);
    }

    // 更新偏移量
    hdlc->pos++;

    // 检查是否超出缓冲区
    if (hdlc->pos == hdlc->mtu)
    {
        hdlc->pos = 0;
        hdlc->crc = CRC16_CCITT_INIT_VAL;
    }

    return 0;
}

int32_t mhdlc_input(mhdlc *hdlc, const uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        mhdlc_input_byte(hdlc, data[i]);
    }
    return 0;
}

int32_t mhdlc_send(mhdlc *hdlc, const char *frame, uint16_t size)
{
    uint8_t data;
    uint16_t fcs = CRC16_CCITT_INIT_VAL;

    // start frame
    _mhdlc_sendchar(hdlc, (uint8_t)FRAME_BOUNDARY_OCTET);

    // payload
    while (size)
    {
        data = *frame++;
        fcs = _crc_ccitt_update(fcs, data);
        if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
        {
            _mhdlc_sendchar(hdlc, (uint8_t)CONTROL_ESCAPE_OCTET);
            data ^= INVERT_OCTET;
        }
        _mhdlc_sendchar(hdlc, (uint8_t)data);
        size--;
    }

    // FCS
    data = low(fcs);
    if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
    {
        _mhdlc_sendchar(hdlc, (uint8_t)CONTROL_ESCAPE_OCTET);
        data ^= (uint8_t)INVERT_OCTET;
    }
    _mhdlc_sendchar(hdlc, (uint8_t)data);

    data = high(fcs);
    if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
    {
        _mhdlc_sendchar(hdlc, CONTROL_ESCAPE_OCTET);
        data ^= INVERT_OCTET;
    }
    _mhdlc_sendchar(hdlc, data);

    // end frame
    _mhdlc_sendchar(hdlc, FRAME_BOUNDARY_OCTET);

    // flush the buffer
    _mhdlc_flush(hdlc);
    return 0;
}

uint16_t hdlc_mtu(mhdlc *hdlc)
{
    return hdlc->mtu;
}
