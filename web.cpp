#include "icecream.h"

#include <errno.h>
#include <curl/curl.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

struct FetchInfo {
	char** string;
	int length;
};

static CURL* curl;

size_t fetchurlstr(char *ptr, size_t size, size_t nmemb, void *userdata) {
	struct FetchInfo* fi = (struct FetchInfo*)userdata;

	int newlen = fi->length + (size * nmemb);

	*(fi->string) = (char*)realloc(*(fi->string), newlen+1);

	memcpy(*(fi->string)+fi->length, ptr, size*nmemb);
	(*(fi->string))[newlen] = 0;

	fi->length = newlen;
	return size * nmemb;
}

size_t fetchurlfile(char *ptr, size_t size, size_t nmemb, void *userdata) {
	return fwrite(ptr, size, nmemb, (FILE*)userdata);
}

int progresscb(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	pbupdate(dlnow, dltotal);
	return 0;
}

static void curlsetup() {
	curl = curl_easy_init();
	if (!curl) die("cURL init failed.");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "IceCream/"VERSION" (http://icecream.maeyanie.com)");
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &progresscb);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	if (settings.verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	#if defined(__WIN32__) || defined(__CYGWIN__)
	curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");
	#endif
}

char* fetchurl(const char* url) {
	char* ret = NULL;
	struct FetchInfo fi;
	long rc;

	if (!curl) curlsetup();
	
	fi.string = &ret;
	fi.length = 0;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &fetchurlstr);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fi);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	rc = curl_easy_perform(curl);
	if (rc) {
		log("Could not fetch URL '%s'\n", url);
		if (ret) free(ret);
		return NULL;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
	if (rc >= 400) {
		log("Could not fetch URL '%s': HTTP %ld\n", url, rc);
		if (ret) free(ret);
		return NULL;
	}
	
	return ret;
}

int fetchurl(const char* url, const char* file) {
	FILE* fp;
	long rc;

	if (!curl) curlsetup();
	
	fp = fopen(file, "wb");
	if (!fp) die("Could not open file '%s' for writing: %s\n", file, strerror(errno));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &fetchurlfile);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	pbstart(file, url);
	rc = curl_easy_perform(curl);
	pbdone();
	if (rc) {
		log("Could not fetch URL '%s' to file '%s'\n", url, file);
		fclose(fp);
		return 0;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
	if (rc >= 400) {
		log("Could not fetch URL '%s': HTTP %ld\n", url, rc);
		fclose(fp);
		return 0;
	}
		
	fclose(fp);
	return 1;
}



// EOF

