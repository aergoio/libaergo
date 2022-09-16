/*
*******************************************************************************    
*   BTChip Bitcoin Hardware Wallet C test interface
*   (c) 2014 BTChip - 1BTChip7VfTnrPra5jqci7ejnMguuHogTn
*   
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*   limitations under the License.
********************************************************************************/

#include "ledgerLayer.c"
#include "dongleCommHidapi.h"

#define LEDGER_VID 0x2C97
#define NANOS_PID  0x1011
#define NANOX_PID  0x4011
#define NANOSP_PID 0x5011

#define TIMEOUT 60000
#define SW1_DATA 0x61
#define MAX_BLOCK 64

/*****************************************************************************/

#include "../dynamic-lib.c"

fn_hid_init  hid_init = NULL;
fn_hid_exit  hid_exit = NULL;

fn_hid_open  hid_open = NULL;
fn_hid_close hid_close = NULL;

fn_hid_write hid_write = NULL;
fn_hid_read_timeout hid_read_timeout = NULL;

/*****************************************************************************/

bool get_function_pointer(void *h, char *function_name, void **fn_ptr, char *error) {
  *fn_ptr = dylib_sym(h, function_name);
  if (*fn_ptr == NULL) {
    if (error) {
      char errmsg[256];
      dylib_error(sizeof errmsg, errmsg);
      snprintf(error, 256, "failed to get function [%s] pointer from hidapi library: %s",
          function_name, errmsg);
    }
    return false;
  }
  return true;
}

/*****************************************************************************/

bool load_hidapi_library(char *error){
  char *zFile1, *zFile2, *zFile3;

  if (hid_init != NULL) return true;

#ifdef _WIN32
  zFile1 = "hidapi.dll";
  zFile2 = "hidapi-hidraw.dll";
  zFile3 = "hidapi-libusb.dll";
#elif __APPLE__
  zFile1 = "libhidapi.dylib";
  zFile2 = "libhidapi-hidraw.dylib";
  zFile3 = "libhidapi-libusb.dylib";
#else
  zFile1 = "libhidapi.so";
  zFile2 = "libhidapi-hidraw.so";
  zFile3 = "libhidapi-libusb.so";
#endif

  /* load the library */
  void *h = dylib_open(zFile1);
  if (h == NULL) {
    h = dylib_open(zFile2);
  }
  if (h == NULL) {
    h = dylib_open(zFile3);
  }
  if (h == NULL) {
    if (error) {
      char errmsg[256];
      dylib_error(sizeof errmsg, errmsg);
      snprintf(error, 256, "failed to load the hidapi library: %s", errmsg);
    }
    return false;
  }

  /* get the pointer to the functions */
  if (get_function_pointer(h, "hid_init", (void**) &hid_init, error) == false) goto loc_failed;
  if (get_function_pointer(h, "hid_exit", (void**) &hid_exit, error) == false) goto loc_failed;
  if (get_function_pointer(h, "hid_open", (void**) &hid_open, error) == false) goto loc_failed;
  if (get_function_pointer(h, "hid_close", (void**) &hid_close, error) == false) goto loc_failed;
  if (get_function_pointer(h, "hid_write", (void**) &hid_write, error) == false) goto loc_failed;
  if (get_function_pointer(h, "hid_read_timeout", (void**) &hid_read_timeout, error) == false) goto loc_failed;

  return true;
loc_failed:
  dylib_close(h);
  hid_init = NULL;
  hid_exit = NULL;
  hid_open = NULL;
  hid_close = NULL;
  hid_write = NULL;
  hid_read_timeout = NULL;
  return false;
}

/*****************************************************************************/

int initHidapi(char *error) {

  /* load the hidapi library */
  if (load_hidapi_library(error) == false) {
    return -1;
  }

  return hid_init();
}

/*****************************************************************************/

int exitHidapi() {
  if (!hid_exit) return -1;
  return hid_exit();
}

/*****************************************************************************/

hid_device* getFirstDongleHidapi() {
  hid_device *result;
  result = hid_open(LEDGER_VID, NANOS_PID, NULL);
  if (result != NULL) {
    return result;
  }
  result = hid_open(LEDGER_VID, NANOSP_PID, NULL);
  if (result != NULL) {
    return result;
  }
  result = hid_open(LEDGER_VID, NANOX_PID, NULL);
  if (result != NULL) {
    return result;
  }
  return NULL;
}

/*****************************************************************************/

void closeDongleHidapi(hid_device *handle) {
  if (hid_close) {
    hid_close(handle);
  }
}

/*****************************************************************************/

int sendApduHidapi(hid_device *handle, const unsigned char *apdu, size_t apduLength, unsigned char *out, size_t outLength, int *sw) {
  unsigned char buffer[400];
  unsigned char paddingBuffer[MAX_BLOCK];
  int result;
  int length;
  int swOffset;
  int remaining = apduLength;
  int offset = 0;

  result = wrapCommandAPDU(DEFAULT_LEDGER_CHANNEL, apdu, apduLength, LEDGER_HID_PACKET_SIZE, buffer, sizeof(buffer));
  if (result < 0) {
    return result;
  }
  remaining = result;

  while (remaining > 0) {
    int blockSize = (remaining > MAX_BLOCK ? MAX_BLOCK : remaining);
    memset(paddingBuffer, 0, MAX_BLOCK);
    memcpy(paddingBuffer, buffer + offset, blockSize);
    result = hid_write(handle, paddingBuffer, blockSize);
    if (result < 0) {
      return result;
    }    
    offset += blockSize;
    remaining -= blockSize;
  }

  result = hid_read_timeout(handle, buffer, MAX_BLOCK, TIMEOUT);
  if (result < 0) {
    return result;
  }  

  offset = MAX_BLOCK;
  while (1) {
    result = unwrapReponseAPDU(DEFAULT_LEDGER_CHANNEL, buffer, offset, LEDGER_HID_PACKET_SIZE, out, outLength);
    if (result < 0) {
      return result;
    }
    if (result != 0) {
      length = result - 2;
      swOffset = result - 2;
      break;
    }
    result = hid_read_timeout(handle, buffer + offset, MAX_BLOCK, TIMEOUT);
    if (result < 0) {
      return result;
    }
    offset += MAX_BLOCK;
  }

  if (sw != NULL) {
    *sw = (out[swOffset] << 8) | out[swOffset + 1];
  }    

  return length;
}
