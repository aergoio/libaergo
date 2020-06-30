#ifndef HIDAPI_H__
#define HIDAPI_H__

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif


struct hid_device_;
typedef struct hid_device_ hid_device; /**< opaque hidapi structure */

/** hidapi info structure */
struct hid_device_info {
	/** Platform-specific device path */
	char *path;
	/** Device Vendor ID */
	unsigned short vendor_id;
	/** Device Product ID */
	unsigned short product_id;
	/** Serial Number */
	wchar_t *serial_number;
	/** Device Release Number in binary-coded decimal,
		also known as Device Version Number */
	unsigned short release_number;
	/** Manufacturer String */
	wchar_t *manufacturer_string;
	/** Product string */
	wchar_t *product_string;
	/** Usage Page for this Device/Interface
		(Windows/Mac only). */
	unsigned short usage_page;
	/** Usage for this Device/Interface
		(Windows/Mac only).*/
	unsigned short usage;
	/** The USB interface which this logical device
		represents.

		* Valid on both Linux implementations in all cases.
		* Valid on the Windows implementation only if the device
			contains more than one interface.
		* Valid on the Mac implementation if and only if the device
			is a USB HID device. */
	int interface_number;

	/** Pointer to the next device */
	struct hid_device_info *next;
};


/*

int hid_init(void);
int hid_exit(void);

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number);
void hid_close(hid_device *dev);

int hid_write(hid_device *dev, const unsigned char *data, size_t length);
int hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds);

//int hid_read(hid_device *dev, unsigned char *data, size_t length);

//const wchar_t* hid_error(hid_device *dev);

*/



typedef int (*fn_hid_init)(void);

typedef int (*fn_hid_exit)(void);

typedef hid_device* (*fn_hid_open)(
  unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number
);

typedef void (*fn_hid_close)(hid_device *dev);

typedef int (*fn_hid_write)(
  hid_device *dev, const unsigned char *data, size_t length
);

typedef int (*fn_hid_read_timeout)(
  hid_device *dev, unsigned char *data, size_t length, int milliseconds
);



#ifdef __cplusplus
}
#endif

#endif
