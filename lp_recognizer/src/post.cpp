#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "post.hpp"


#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1


size_t ignore_output( void *ptr, size_t size, size_t nmemb, void *stream)
{
    (void) ptr;
    (void) stream;
    return size * nmemb;
}

/* Post JSON data to a server.
name and value must be UTF-8 strings.
Returns TRUE on success, FALSE on failure.
*/
int PostHTTP(const std::string & url, const std::string & json)
{
    int retcode = FALSE;
    CURL *curl = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    char errbuf[CURL_ERROR_SIZE] = { 0, };
    struct curl_slist *headers = NULL;
    char agent[1024] = { 0, };

    curl = curl_easy_init();
    if(!curl) {
        fprintf(stderr, "Error: curl_easy_init failed.\n");
        goto cleanup;
    }

    /* CURLOPT_CAINFO
  To verify SSL sites you may need to load a bundle of certificates.

  You can download the default bundle here:
  https://raw.githubusercontent.com/bagder/ca-bundle/master/ca-bundle.crt

  However your SSL backend might use a database in addition to or instead of
  the bundle.
  http://curl.haxx.se/docs/ssl-compared.html
  */
    curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

    snprintf(agent, sizeof agent, "libcurl/%s",
             curl_version_info(CURLVERSION_NOW)->version);
    agent[sizeof agent - 1] = 0;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

    /* This is a test server, it fakes a reply as if the json object were
     created */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // "http://jsonplaceholder.typicode.com/posts"

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);


    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &ignore_output);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        size_t len = strlen(errbuf);
        fprintf(stderr, "\nlibcurl: (%d) ", res);
        if(len)
            fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
        fprintf(stderr, "%s\n\n", curl_easy_strerror(res));
        goto cleanup;
    }

    retcode = TRUE;

cleanup:
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return retcode;
}

