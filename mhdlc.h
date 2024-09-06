#ifndef _M_HDLC_H_
#define _M_HDLC_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef void (*mhdlc_output)(const uint8_t *data, uint16_t len, void *priv);
typedef void (*mhdlc_flush)(void *priv);
typedef void (*mhdlc_on_frame)(const uint8_t *frame, uint16_t size, void *priv);

typedef struct mhdlc
{
    mhdlc_output output;
    mhdlc_flush flush;
    mhdlc_on_frame on_frame;

    uint16_t mtu;

    uint8_t escape;
    uint16_t pos;
    uint16_t crc;

    uint8_t *frame;

    void *priv;
} mhdlc;

int32_t mhdlc_init(mhdlc *hdlc, mhdlc_output output, mhdlc_flush flush, mhdlc_on_frame on_frame, uint8_t *buffer, uint16_t buffer_size, void *priv);

int32_t mhdlc_send(mhdlc *hdlc, const char *frame, uint16_t size);

int32_t mhdlc_input_byte(mhdlc *hdlc, uint8_t data);

int32_t mhdlc_input(mhdlc *hdlc, const uint8_t *data, uint16_t size);

uint16_t hdlc_mtu(mhdlc *hdlc);

#endif // _M_HDLC_H_
