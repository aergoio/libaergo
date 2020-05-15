#include "linked_list.c"

void close_socket(SOCKET sock) {
  shutdown(sock, 2);
#ifdef _WIN32
  closesocket(sock);
#else
  close(sock);
#endif
}

SOCKET http_connect(char *host, unsigned short int port){
  struct hostent *server;
  struct sockaddr_in serv_addr;
  SOCKET sock;
  int on = 1;

  /* lookup the ip address */
  server = gethostbyname(host);
  if (server == NULL) goto loc_failed;

  /* create the socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) goto loc_failed;

  /* set the TCP_NODELAY option */
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

  /* fill in the structure */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  /* connect the socket */
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    goto loc_failed;

  return sock;
loc_failed:
  return INVALID_SOCKET;
}

int http_send_request(SOCKET sock, char *request, int request_size) {
  int bytes, sent, total;

  printf("Request:\n-----\n%s\n-----\n", request);

  /* send the request */

  total = request_size;
  sent = 0;

  do {
    bytes = write(sock, request+sent, total-sent);
    if (bytes < 0)
      return -1;
    if (bytes == 0)
      break;
    sent += bytes;
  } while (sent < total);

  return sent;
}


/*
** Returns:
** < 0 - error
** = 0 - there is more data to be read
** > 0 - all data was read
*/
int http_get_response(request *request) {
  int bytes, total;

  if (!request->response) {
    request->response = malloc(2048);
    if (!request->response) return -1;
    request->response_size = 2048;
    memset(request->response, 0, request->response_size);
  }

loc_again:

  total = request->response_size - 1;
  if (total == 0) goto loc_realloc;

  do {
    bytes = read(request->sock, request->response + request->received, total - request->received);
printf("received %d bytes\n", bytes);
    if (bytes < 0) {
      int err = errno;
      if (err == EAGAIN || err == EWOULDBLOCK) {
        return 0;  /* there is more data to be read */
      } else {
        return -1;
      }
    }
    if (bytes == 0)
      break;
    request->received += bytes;
  } while (request->received < total);

  if (request->received == total) {
    int new_size;
    void *ptr;
  loc_realloc:
    new_size = (total + 1) * 2;
    ptr = realloc(request->response, new_size);
    if (!ptr) { free(request->response); return -1; }
    request->response = ptr;
    request->response_size = new_size;
    memset(request->response + request->received, 0, new_size - request->received);
    goto loc_again;
  }

  return request->received;
}

static int aergo_process_requests__int(aergo *instance, int timeout) {
  fd_set readset;
  struct timeval tv;
  unsigned int max;
  int ret, num_active_requests;
  struct request *request;

  if (!instance) return -1;
  if (!instance->requests) return 0;

  // get the list of active sockets and put them in the readfs
  FD_ZERO(&readset);
  max = 0;
  for (request=instance->requests; request; request=request->next) {
    if (request->sock != INVALID_SOCKET) {
      FD_SET(request->sock, &readset);
      if (request->sock > max) max = request->sock;
    }
  }
  if (max == 0) goto loc_exit;

  // set the timeout
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;

  // check if some socket has data to be read
  ret = select(max+1, &readset, NULL, NULL, &tv);
  // error
  if (ret < 0) goto loc_failed;
  // no data to be read
  if (ret == 0) goto loc_exit;

  // for each "set" socket, process the incoming data
  for (request=instance->requests; request; request=request->next) {
    if (request->sock != INVALID_SOCKET && FD_ISSET(request->sock, &readset)) {
      /* read data from socket */
      ret = http_get_response(request);
      if (ret > 0) {
        /* parse the received data */
        request->process_response(instance, request);
        /* mark to release the request */
        ret = -1;
      }
      if (ret < 0) {
        /* release the allocated memory */
        free(request->response);
        /* close the socket */
        close_socket(request->sock);
        request->sock = INVALID_SOCKET;
      }
    }
  }

loc_exit:

  /* release processed requests */
  for (request=instance->requests; request; request=request->next) {
    if (request->sock == INVALID_SOCKET) {
      //instance->requests->next = request->next;
      llist_remove(&instance->requests, request);
      free(request);
      goto loc_exit; /* start again from the beginning */
    }
  }

  /* count how many active requests remaining */
  num_active_requests = 0;
  for (request=instance->requests; request; request=request->next) {
    if (request->sock != INVALID_SOCKET) {
      num_active_requests++;
    }
  }
  return num_active_requests;

loc_failed:
  return -1;

}

int aergo_process_requests(aergo *instance) {
  /* no timeout, just check and return immediately */
  return aergo_process_requests__int(instance, 0);
}

uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size){
  // no compression
  buffer[0] = 0;
  // insert the size in the stream as big endian 32-bit integer
  copy_be32((uint32_t*)&buffer[1], &content_size);
  // return the frame size
  return content_size + 5;
}

bool send_grpc_request(aergo *instance, char *service, struct request *request, process_response_cb response_callback) {
  const char *raw_request =
    "POST /types.AergoRPCService/%s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "Connection: Close\r\n"
    "User-Agent: herac/1.0\r\n"
    "Accept: */*\r\n"
    "Content-Type: application/grpc-web+proto\r\n"
    "X-Grpc-Web: 1\r\n"
    "Content-Length: %d\r\n"
    "\r\n";
  char http_request[4096];
  int header_size, ret;
  char *p;

  /* connect to the host */
  request->sock = http_connect(instance->host, instance->port);
  if (request->sock == INVALID_SOCKET) return false;

  /* prepare the HTTP request */
  snprintf(http_request, sizeof(http_request), raw_request, service,
    instance->host, instance->port, request->size);
  header_size = strlen(http_request);
  p = http_request + header_size;
  memcpy(p, request->data, request->size);

  /* send the request */
  ret = http_send_request(request->sock, http_request, header_size + request->size);
  if (ret <= 0) return false;


  /* save the response handler */
  request->process_response = response_callback;

  /* if the call is synchronous, wait for the response */
  if (!request->callback) {
    aergo_process_requests__int(instance, instance->timeout);
  }

  //DEBUG_PRINTF("Request done. returning\n");
  return true;
}
