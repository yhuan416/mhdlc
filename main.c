#include "mhdlc.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define buffer_size (256)
static uint8_t buffer[buffer_size];

mhdlc hdlc;

void _output(const uint8_t *data, uint16_t len, void *priv)
{
    // fwrite(data, 1, len, stdout);
    mhdlc_input(&hdlc, data, len);
}

void _flush(void *priv)
{
}

void _on_frame(const uint8_t *frame, uint16_t size, void *priv)
{
    printf("Recv: ");
    fwrite(frame, 1, size, stdout);
    printf("\n");
}

int main(int argc, char const *argv[])
{
    int i;
    char buff[10];

    mhdlc_init(&hdlc, _output, _flush, _on_frame, buffer, buffer_size, NULL);

    mhdlc_send(&hdlc, "Hello, World!", 13);

    for (i = 0; i < 13; i++)
    {
        memset(buff, 0, 10);
        sprintf(buff, "%d\r\n", i);
        mhdlc_send(&hdlc, buff, strlen(buff));
    }

    return 0;
}
