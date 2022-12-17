#ifndef __LEDGER_API_H__
#define __LEDGER_API_H__

EXPORTED int ledger_send_apdu(
  unsigned char apdu_cla,
  unsigned char apdu_ins,
  unsigned char apdu_p1,
  const char *data,
  unsigned int datalen,
  unsigned char *out,
  unsigned int outsize,
  int *psw,
  char *error
);

#define SW_OK 0x9000
#define SW_UNKNOWN 0x6D00

#endif
