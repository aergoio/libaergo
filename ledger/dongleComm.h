#ifndef __DONGLECOMM_H__
#define __DONGLECOMM_H__

typedef void* dongleHandle;

int initDongle(char *error);
int exitDongle(void);
int sendApduDongle(dongleHandle handle, const unsigned char *apdu, size_t apduLength, unsigned char *out, size_t outLength, int *sw);
dongleHandle getFirstDongle();
void closeDongle(dongleHandle handle);

#endif
