#include "restclient.h"
#include <iostream>
#include <string.h>
#include "curl/curl.h"
#include <dlfcn.h>

const char* RestClient::userAgent = "restclient-cpp/1.0";
void* dylib;
typedef CURLcode (*f_csetopt)(CURL* curl, CURLoption option, ...);

void RestClient::init() {
    dylib = dlopen("./libcurl.dylib", RTLD_LAZY);
}

CURL* cinit() {
    return ((CURL*(*)())dlsym(dylib, "curl_easy_init"))();
}

f_csetopt fcsetopt() {
    return (CURLcode(*)(CURL* curl, CURLoption option, ...))dlsym(dylib, "curl_easy_setopt");
}

CURLcode cperform(CURL* curl) {
    return ((CURLcode(*)(CURL* curl))dlsym(dylib, "curl_easy_perform"))(curl);
}

void ccleanup(CURL* curl) {
    return ((void(*)(CURL* curl))dlsym(dylib, "curl_easy_cleanup"))(curl);
}

void cgetinfo(CURL* curl, CURLINFO info, long* l) {
    return ((void(*)(CURL* curl, CURLINFO info, long* l))dlsym(dylib, "curl_easy_getinfo"))(curl, info, l);
}

void cglobalcleanup() {
    return ((void(*)())dlsym(dylib, "curl_global_cleanup"))();
}

curl_slist* clistappend(curl_slist* list, const char* c) {
    return ((curl_slist*(*)(curl_slist*, const char*))dlsym(dylib, "curl_slist_append"))(list, c);
}

void clistfreeall(curl_slist* list) {
    return ((void(*)(curl_slist* list))dlsym(dylib, "curl_slist_free_all"))(list);
}

RestClient::response RestClient::get(const std::string& url, const size_t timeout) {
    headermap emptyMap;
    return RestClient::get(url, emptyMap, timeout);
}

RestClient::response RestClient::get(const std::string& url, const headermap& headers, const size_t timeout) {
    RestClient::response ret = {};
    CURLcode res = CURLE_OK;
    CURL* curl = NULL;
    std::string header;
    curl = cinit();
    if (!curl) {
        ret.body = "curl initialization failed";
        return ret;
    }

    fcsetopt()(curl, CURLOPT_SSL_VERIFYPEER, 0);
    fcsetopt()(curl, CURLOPT_USERAGENT, RestClient::userAgent);
    fcsetopt()(curl, CURLOPT_URL, url.c_str());
    fcsetopt()(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
    fcsetopt()(curl, CURLOPT_WRITEDATA, &ret);
    fcsetopt()(curl, CURLOPT_HEADERFUNCTION, RestClient::header_callback);
    fcsetopt()(curl, CURLOPT_HEADERDATA, &ret);

    curl_slist* hList = NULL;
    for (headermap::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        header = it->first;
        header += ": ";
        header += it->second;
        hList = clistappend(hList, header.c_str());
    }
    fcsetopt()(curl, CURLOPT_HTTPHEADER, hList);
    if (timeout) {
        fcsetopt()(curl, CURLOPT_TIMEOUT, timeout);
        fcsetopt()(curl, CURLOPT_NOSIGNAL, 1); // dont want to get a sig alarm on timeout
    }
    res = cperform(curl);
    if (res != CURLE_OK) {
        if (res == CURLE_OPERATION_TIMEDOUT) {
            ret.code = res;
            ret.body = "Operation Timeout";
            return ret;
        }

        std::cout << "response " << res << '\n';
        ret.body = "Failed To Query";
        ret.code = -1;
        return ret;
    }

    long httpCode = 0;
    // cgetinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    ret.code = static_cast<int>(httpCode);
    clistfreeall(hList);
    ccleanup(curl);
    cglobalcleanup();
    return ret;
}

RestClient::response RestClient::post(const std::string& url, const std::string& ctype, const std::string& data, const size_t timeout) {
    headermap emptyMap;
    return post(url, ctype, data, emptyMap, timeout);
}

RestClient::response RestClient::post(const std::string& url, const std::string& ctype, const std::string& data, headermap& headers, const size_t timeout) {
    RestClient::response ret = {};
    std::string ctype_header = "Content-Type: " + ctype;
    std::string header;
    CURL* curl = NULL;
    CURLcode res = CURLE_OK;
    curl = cinit();
    if (!curl) {
        ret.body = "curl initialization failed";
        return ret;
    }
    fcsetopt()(curl, CURLOPT_SSL_VERIFYPEER, 0);
    fcsetopt()(curl, CURLOPT_USERAGENT, RestClient::userAgent);
    fcsetopt()(curl, CURLOPT_URL, url.c_str());
    fcsetopt()(curl, CURLOPT_POST, 1L);
    fcsetopt()(curl, CURLOPT_POSTFIELDS, data.c_str());
    fcsetopt()(curl, CURLOPT_POSTFIELDSIZE, data.size());
    fcsetopt()(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
    fcsetopt()(curl, CURLOPT_WRITEDATA, &ret);
    fcsetopt()(curl, CURLOPT_HEADERFUNCTION, RestClient::header_callback);
    fcsetopt()(curl, CURLOPT_HEADERDATA, &ret);
    curl_slist* hList = NULL;
    hList = clistappend(hList, ctype_header.c_str());
    for (headermap::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        header = it->first;
        header += ": ";
        header += it->second;
        hList = clistappend(hList, header.c_str());
    }
    fcsetopt()(curl, CURLOPT_HTTPHEADER, hList);
    if (timeout) {
        fcsetopt()(curl, CURLOPT_TIMEOUT, timeout);
        fcsetopt()(curl, CURLOPT_NOSIGNAL, 1); // dont want to get a sig alarm on timeout
    }
    res = cperform(curl);
    if (res != CURLE_OK)
    {
        if (res == CURLE_OPERATION_TIMEDOUT) {
            ret.code = res;
            ret.body = "Operation Timeout.";
            return ret;
        }
        ret.body = "Failed to query.";
        ret.code = -1;
        return ret;
    }
    long http_code = 0;
    // cgetinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    ret.code = static_cast<int>(http_code);
    clistfreeall(hList);
    ccleanup(curl);
    cglobalcleanup();
    return ret;
}

size_t RestClient::write_callback(void* data, size_t size, size_t nmemb, void* userdata) {
    RestClient::response* r;
    r = reinterpret_cast<RestClient::response*>(userdata);
    r->body.append(reinterpret_cast<char*>(data), size * nmemb);
    return (size * nmemb);
}

size_t RestClient::header_callback(void* data, size_t size, size_t nmemb, void* userdata) {
    RestClient::response* r;
    r = reinterpret_cast<RestClient::response*>(userdata);
    std::string header(reinterpret_cast<char*>(data), size * nmemb);
    size_t seperator = header.find_first_of(":");
    if (std::string::npos == seperator) {
        //roll with non seperated headers...
        trim(header);
        if (0 == header.length()) {
            return (size * nmemb); //blank line;
        }
        r->headers[header] = "present";
    }
    else {
        std::string key = header.substr(0, seperator);
        trim(key);
        std::string value = header.substr(seperator + 1);
        trim(value);
        r->headers[key] = value;
    }

    return (size * nmemb);
}

size_t RestClient::read_callback(void* data, size_t size, size_t nmemb, void* userdata) {
    /** get upload struct */
    RestClient::uploadObject* u;
    u = reinterpret_cast<RestClient::uploadObject*>(userdata);
    /** set correct sizes */
    size_t curl_size = size * nmemb;
    size_t copy_size = (u->length < curl_size) ? u->length : curl_size;
    /** copy data to buffer */
    memcpy(data, u->data, copy_size);
    /** decrement length and increment data pointer */
    u->length -= copy_size;
    u->data += copy_size;
    /** return copied size */
    return copy_size;
}
