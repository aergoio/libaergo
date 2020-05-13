
#define AddressLength         33
#define EncodedAddressLength  52

#define AddressVersion      0x42
#define PrivKeyVersion      0xAA

bool sha256(void *hash, const void *data, size_t len);
bool encode_address(const void *data, size_t datasize, char *out, size_t outsize);
bool decode_address(const char *encoded, size_t encsize, void *out, size_t outsize);

