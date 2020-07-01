#include "dongleComm.c"
#include "ledger-api.h"

#define OFFSET_CDATA 0x04

#define APDU_P1_FIRST 0x01
#define APDU_P1_LAST  0x02

/******************************************************************************/

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
){
  dongleHandle dongle = NULL;
  unsigned char in[260];
  int apduSize;
  int result;
  int sw = 0;

  if (error) error[0] = 0;

  result = initDongle(error);
  if (result != 0) goto loc_exit;

  /* open the device */

  dongle = getFirstDongle();
  if (dongle == NULL) {
    sw = -1;
    result = -1;
    goto loc_exit;
  }

  /* send the transaction to be signed */

  result = 0;
  while (result == 0) {
    int offset = 0;
    while (offset < datalen) {
      int len;
      unsigned char p1;

      /* build the request packet */

      len = datalen - offset;
      if (len > 250) {
        len = 250;
      }

      if (apdu_p1) {
        p1 = apdu_p1;
      } else {
        p1 = 0;
        if (offset == 0) {
          p1 |= APDU_P1_FIRST;
        }
        if (offset + len == datalen) {
          p1 |= APDU_P1_LAST;
        }
      }

      apduSize = 0;
      in[apduSize++] = apdu_cla;
      in[apduSize++] = apdu_ins;
      in[apduSize++] = p1;
      in[apduSize++] = 0x00;
      in[apduSize++] = 0x00;
      memcpy(in + apduSize, data + offset, len);
      apduSize += len;
      in[OFFSET_CDATA] = (apduSize - 5);

      /* send the request and wait for the result */

      result = sendApduDongle(dongle, in, apduSize, out, outsize, &sw);
      if (result < 0 || sw != SW_OK) goto loc_exit;
      if (result > 0) break;

      offset += len;
    }

  }

loc_exit:
  if (psw) *psw = sw;
  if (dongle != NULL) {
    closeDongle(dongle);
  }
  exitDongle();
  return result;
}
