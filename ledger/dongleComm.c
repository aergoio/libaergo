#include "dongleCommHidapi.c"
#include "dongleComm.h"

#ifdef DEBUG_COMM
void displayBinary(const char *label, const unsigned char *buffer, size_t length) {
	size_t i;
  printf("%s", label);
	for (i=0; i<length; i++) {
		printf("%.2x", buffer[i]);
	}
	printf("\n");
}
#endif

int initDongle(char *error) {
  return initHidapi(error);
}

int exitDongle(void) {
  return exitHidapi();
}

dongleHandle getFirstDongle() {
  return (dongleHandle) getFirstDongleHidapi();
}

void closeDongle(dongleHandle handle) {
  closeDongleHidapi((hid_device*)handle);
}

int sendApduDongle(dongleHandle handle, const unsigned char *apdu, size_t apduLength, unsigned char *out, size_t outLength, int *sw){
  int result;
#ifdef DEBUG_COMM
  displayBinary("=> ", apdu, apduLength);
#endif
  result = sendApduHidapi((hid_device*)handle, apdu, apduLength, out, outLength, sw);
#ifdef DEBUG_COMM
  if (result >= 0) {
    displayBinary("<= ", out, result);
  }
#endif
  return result;
}
