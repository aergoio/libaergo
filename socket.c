#include "linked_list.c"

static void sleep_ms(int milliseconds){ // cross-platform sleep function
#ifdef WIN32
  Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
#else
  if (milliseconds >= 1000)
    sleep(milliseconds / 1000);
  usleep((milliseconds % 1000) * 1000);
#endif
}

static bool http_strip_header(aergo *instance, request *request){
  char *ptr, *error_msg;

  DEBUG_PRINTLN("http_strip_header");

  error_msg = strstr(request->response, "grpc-message: ");
  if (error_msg) {
    error_msg += strlen("grpc-message: ");
    ptr = strstr(error_msg, "\r\n");
    if (ptr) *ptr = 0;
    request->error_msg = error_msg;
    goto loc_error;
  }

  return true;

loc_error:

  DEBUG_PRINTF("\n  ERROR: %s\n\n", request->error_msg);

  return false;
}


static int aergo_process_requests__int(
  aergo *instance,
  int timeout,
  request *main_request,
  bool *psuccess
){
  struct request *request;
  int still_running;

  if (!instance) return -1;
  if (!instance->requests) return 0;  //!

  DEBUG_PRINTF("aergo_process_requests timeout=%d\n", timeout);

loc_again:

  if (instance->transfers > 0) {
    int numfds;
    CURLMcode mcode;
    struct CURLMsg *m;

    mcode = curl_multi_perform(instance->multi, &still_running);
    printf("curl_multi_perform ret=%d\n", mcode);
    if (mcode) goto loc_failed;

    mcode = curl_multi_wait(instance->multi, NULL, 0, timeout, &numfds);
    printf("curl_multi_wait ret=%d\n", mcode);
    if (mcode) goto loc_failed;

    if (numfds == 0 && timeout > 0) {
      sleep_ms(timeout / 10);
    }

    /*
     * When doing server push, libcurl itself created and added one or more
     * easy handles but *we* need to clean them up when they are done.
     */
    do {
      int msgq = 0;
      m = curl_multi_info_read(instance->multi, &msgq);
      printf("curl_multi_info_read ret=%p\n", m);
      if (m && (m->msg == CURLMSG_DONE)) {
        CURL *easy = m->easy_handle;
        puts("REQUEST DONE.");
#if 0
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &request);
        if (request) {

        }
#endif
        /* remove the CURL request */
        curl_multi_remove_handle(instance->multi, easy);
        curl_easy_cleanup(easy);
        instance->transfers--;
      }
    } while(m);

  }

  if (main_request) {
    if (main_request->processed) {
      if (psuccess) {
        *psuccess = main_request->success;
      }
    } else if (instance->transfers > 0) {
      goto loc_again;
    }
  }



#if 0

  // get the list of active sockets and put them in the readfs
  FD_ZERO(&readset);
  max = 0;
  for (request=instance->requests; request; request=request->next) {
    if (request->sock != INVALID_SOCKET && !request->processed) {
      DEBUG_PRINTLN("  add request to set");
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
        request->processed = true;
        /* remove the HTTP header */
        if (http_strip_header(instance, request)) {
          /* parse the received data */
          request->response_ok = true;
          bool success = request->process_response(instance, request);
          if (request == main_request && psuccess) {
            *psuccess = success;
          }
        }
        if (!request->keep_active) {
          /* mark to release the request */
          ret = -1;
        }
      }
      if (ret < 0) {
        request->processed = true;
        if (!request->error_msg) {
          request->error_msg = "connection or allocation error";
        }
        if (!request->response_ok && request->process_error) {
          /* it uses the return value here because the error handler
          ** can retry the request. this is used for receipts */
          bool success = request->process_error(instance, request);
          if (request == main_request && psuccess) {
            *psuccess = success;
          }
        }
        /* close the socket */
        close_socket(request->sock);
        request->sock = INVALID_SOCKET;
      }
    }
  }

#endif




loc_exit:

  /* release processed requests */
  for (request=instance->requests; request; request=request->next) {
    if (request->processed && !request->keep_active) {
      free_request(instance, request);
      goto loc_exit; /* start again from the beginning */
    }
  }

#if 0
  /* count how many active requests remaining */
  num_active_requests = 0;
  for (request=instance->requests; request; request=request->next) {
    if (request->sock != INVALID_SOCKET) {
      num_active_requests++;
    }
  }
  return num_active_requests;
#endif
  return instance->transfers;

loc_failed:
  return -1;

}

EXPORTED int aergo_process_requests(aergo *instance, int timeout) {
  /* no timeout, just check and return immediately */
  return aergo_process_requests__int(instance, timeout, NULL, NULL);
}

uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size){
  // no compression
  buffer[0] = 0;
  // insert the size in the stream as big endian 32-bit integer
  copy_be32((uint32_t*)&buffer[1], &content_size);
  // return the frame size
  return content_size + 5;
}

static void process_request_error(struct request *request, char *error_msg) {

  request->processed = true;
  if (!request->error_msg) {
    request->error_msg = error_msg;
  }
  if (request->process_error) {
    /* it uses the return value here because the error handler
    ** can retry the request. this is used for receipts */
    request->success = request->process_error(request->instance, request);
  }

}

static size_t server_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  struct request *request = (struct request *) userdata;
  printf("HEADER %s\n", buffer);
  if (strstr(buffer, "grpc-message: ") != NULL) {
    puts("processing error message...");
    char *error_msg = buffer + 14;  //strlen("grpc-message: ");
    char *ptr = strstr(error_msg, "\r\n");
    if (ptr) *ptr = 0;
    process_request_error(request, error_msg);
  }
  return nitems * size;
}

static size_t server_response_callback(void *contents, size_t num_blocks, size_t block_size, void *userp){
  struct request *request = (struct request *) userp;
  size_t size = num_blocks * block_size;
  size_t to_process;
  char *ptr = contents;

  printf("server_response_callback size=%zu\n", size);

  if (!request) return 0;

loc_again:

  /* is this a new response? */
  if (request->response == NULL) {
    unsigned int msg_size;
    /* gets the size of the msg on the first part */
    copy_be32(&msg_size, (int*)(ptr + 1));
    msg_size += 5;
    /* more than 1 msg on this callback? */
    if (size > msg_size) {
      to_process = msg_size;
      request->remaining_size = 0;
      size -= to_process;
    } else {
      to_process = size;
      request->remaining_size = msg_size - size;
      size = 0;
    }
    /* allocate space for the entire message */
    request->response = malloc(msg_size);
    if (!request->response) {
      //printf("not enough memory (malloc returned NULL)\n");
      process_request_error(request, "out of memory");
      return 0;
    }
  } else {
    /* more than 1 msg on this callback? */
    if (size > request->remaining_size) {
      to_process = request->remaining_size;
      request->remaining_size = 0;
      size -= to_process;
    } else {
      to_process = size;
      request->remaining_size -= size;
      size = 0;
    }
  }

  memcpy(&(request->response[request->response_size]), ptr, to_process);
  request->response_size += to_process;
  request->received += to_process;

  /* is there the whole message? */
  if (request->remaining_size == 0) {
    /* process the message */
    request->processed = true;
    /* parse the received data */
    request->success = request->process_response(request->instance, request);
  }

  /* was the whole message processed? */
  if (request->remaining_size == 0) {
    /* release the memory */
    free(request->response);
    request->response = NULL;
    request->response_size = 0;
    request->received = 0;
    //
    ptr += to_process;
    if (size > 0) goto loc_again;
  }

  return num_blocks * block_size; //size;
}

static bool new_http_request(aergo *instance, char *url, struct request *request){
  struct curl_slist *headers = NULL;
  CURL *easy;

  printf("new_request: %s\n", url);

  if (instance->multi == NULL) {
    instance->multi = curl_multi_init();
    if (instance->multi == NULL) return false;
    /* set options */
    curl_multi_setopt(instance->multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    //curl_multi_setopt(multi, CURLMOPT_PUSHFUNCTION, server_push_callback);
    //curl_multi_setopt(multi, CURLMOPT_PUSHDATA, &transfers);
  }

  easy = curl_easy_init();
  if (!easy) return false;

  /* set the same URL */
  curl_easy_setopt(easy, CURLOPT_URL, url);

  /* custom HTTP headers */
  headers = curl_slist_append(headers, "User-Agent: libaergo/0.1");
  headers = curl_slist_append(headers, "Content-Type: application/grpc");
  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

  /* now specify the POST data */
  curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request->data);
  curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, request->size);

  /* use HTTP/2 */
  //curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
  curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);

  // for debug only
  curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);

  /* set the server response callback */
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, server_response_callback);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, request);

  /* set the server header callback */
  curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, server_header_callback);
  curl_easy_setopt(easy, CURLOPT_HEADERDATA, request);

  /* pointer to the request */
  //curl_easy_setopt(easy, CURLOPT_PRIVATE, request);

  /* wait for pipe connection to confirm */
  curl_easy_setopt(easy, CURLOPT_PIPEWAIT, 1L);

  /* add the easy handle to the multi handle */
  curl_multi_add_handle(instance->multi, easy);
  instance->transfers++;

  //request->easy = easy;

  return true;
}

bool send_grpc_request(
  aergo *instance,
  char *service,
  struct request *request,
  process_response_cb response_callback
){
  char url[256];
  bool success = false;

  DEBUG_PRINTLN("send_grpc_request");

  /* build the URI, eg:
  ** "http://testnet-api.aergo.io:7845/types.AergoRPCService/ListBlockStream" */
  snprintf(url, sizeof(url), "http://%s:%d/types.AergoRPCService/%s",
           instance->host, instance->port, service);

  /* prepare the HTTP request */
  //new_http_request(multi, url, request->data, request->size);
  if (new_http_request(instance, url, request) == false) {
    return false;
  }

  /* save the response handler */
  request->process_response = response_callback;

  if (request->callback) {
    /* if the call is asynchronous, just return */
    success = true;
  } else {
    /* if the call is synchronous, wait for the response */
    aergo_process_requests__int(instance, instance->timeout, request, &success);
  }

  DEBUG_PRINTF("send_grpc_request %s\n", success ? "OK" : "FAILED");
  return success;
}
