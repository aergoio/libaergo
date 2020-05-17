#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>  /* socket, connect */
#include <sys/select.h>
#include <netinet/in.h>  /* struct sockaddr_in, struct sockaddr */
#include <netinet/tcp.h> /* TCP_NODELAY */
#include <netinet/ip.h>
#include <netdb.h>       /* struct hostent, gethostbyname */
#include <unistd.h>      /* read, write, close */
#define SOCKET int
#define INVALID_SOCKET  (~0)
#define SOCKET_ERROR    (-1)
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#endif


#include "secp256k1-vrf.h"



typedef struct request request;

typedef bool (*process_response_cb)(aergo *instance, request *request);

struct request {
  struct request *next;
  void *data;
  int size;
  struct aergo_account *account;
  char txn_hash[32];
  void *callback;
  void *arg;
  void *return_ptr;
  int return_size;
  process_response_cb process_response;
  process_response_cb process_error;
  SOCKET sock;
  char *response;
  int response_size;
  int received;
  bool processed;
  bool response_ok;
  bool keep_active;
};


// Aergo connection instance
struct aergo {
  char host[32];
  int port;
  secp256k1_context *ecdsa;
  uint8_t blockchain_id_hash[32];
  int timeout;
  error_handler_cb error_handler;
  void *error_handler_arg;
  request *requests;
};


static request * new_request(aergo *instance);
void free_request(aergo *instance, struct request *request);


uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size);
