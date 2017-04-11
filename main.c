/*
 * @author Michal Karm Babacek
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#define CONTENT_TYPE "Content-Type: application/octet-stream"
#define USER_AGENT "Hahahaha! 0.0"
#define die(X, ...) fprintf(stderr, "ERROR %s:%d: " X "\n", __FILE__, __LINE__, ##__VA_ARGS__);exit(EXIT_FAILURE);

CURL *curl;
CURLcode res;
struct my_response data;
FILE *headerfile;
struct curl_slist *headers = NULL;

struct my_response {
    size_t size;
    char* response_body;
};

size_t max_byes_to_read = 0;
char be_nice = 0;

void free_connection() {
    curl_slist_free_all(headers);
    if(curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    free(data.response_body);
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct my_response *mem = (struct my_response *)userp;

  //Not necessary, since we want to end abruptly anyway...
  mem->response_body = realloc(mem->response_body, mem->size + realsize + 1);
  if(mem->response_body == NULL) {
    die("Failed to allocate memory.\n");
  }

  memcpy(&(mem->response_body[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->response_body[mem->size] = 0;

  //printf("mem->size: %ld\n", mem->size);
  if(mem->size > max_byes_to_read) {
      printf("Maximum bytes reached: actual: %ld, max: %ld, exiting...\n", mem->size, max_byes_to_read);
      if(be_nice) {
          free_connection();
      }
      exit(0);
  }

  return realsize;
}

int main(int argc, char *argv[]) {

    if(argc != 4) {
        printf("Usage: %s http://localhost:8080/clusterbench/100M 13952533 <good|nogood>\n\
                i.e. downloads just around 13% of that 100M file and terminates\n\
                good = close connection, nogood = dies abruptly\n", argv[0]);
                return 0;
    }

    data.size = 0;
    data.response_body = malloc(2*atol(argv[2]));
    max_byes_to_read = atol(argv[2]);
    if(strncmp("good", argv[3], 4) == 0) {
        be_nice = 1;
    }

    if(NULL == data.response_body) {
       die("Failed to allocate memory.\n");
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 10000);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 1000);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        headers = curl_slist_append(headers, CONTENT_TYPE);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
        res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        printf("Response Code: %ld\n", http_code);
    } else {
        die("CURL wasn't initialized.\n");
    }
    if(be_nice) {
        free_connection();
    }
    printf("Done without interruption!\n");
    return 0;
}
