
int bignum_to_string(const uint8_t *buf, int len, uint8_t *out, int outlen);
int string_to_bignum(const uint8_t *str, int len, uint8_t *out, int outlen);

bool arguments_to_json(char *buf, int bufsize, const char *types, va_list ap);
