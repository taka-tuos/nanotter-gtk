#include "nanotter.hpp"

// ダウンロードの際のポインタ。オーバーフローチェックはしてないぜ。
size_t g_curlDownloadPtr = 0;

/*
 * size_t curlDownloadCallbackfunction(void *ptr, size_t size, size_t nmemb, void* userdata)
 * 
 * 引数：
 * CURLのリファレンス見てね(よくわかってないから)
 * 
 * 返り値：
 * CURLのリファレンス見てね(よくわかってないから)
 * 
 * ほぼコピペ。
 * curlでアイコンをDLする用だが他にも使えそう。
 */
size_t curlDownloadCallbackfunction(void *ptr, size_t size, size_t nmemb, void* userdata)
{
	char *stream = (char *)userdata;
	if (!stream)
	{
		printf("!!! No stream\n");
		return 0;
	}

	size_t written = size * nmemb;
	memcpy(g_curlDownloadPtr + stream, ptr, written);
	g_curlDownloadPtr += written;
	return written;
}

/*
 * size_t curlDownloadCallbackfunction(void *ptr, size_t size, size_t nmemb, void* userdata)
 * 
 * 引数：
 * unsigned char *work : 保存先バッファ
 * const char* url : DLしたいURL
 * 
 * 返り値：
 * bool : DL終わったかどうか
 * 
 * ほぼコピペ。
 * curlでアイコンをDLする用だが他にも使えそう。
 */
bool curlDownloadFile(unsigned char *work, const char* url)
{
	CURL* curlCtx = curl_easy_init();
	
	g_curlDownloadPtr = 0;
	
	curl_easy_setopt(curlCtx, CURLOPT_URL, url);
	curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, work);
	curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, curlDownloadCallbackfunction);
	curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);

	CURLcode rc = curl_easy_perform(curlCtx);
	if (rc)
	{
		printf("!!! Failed to download: %s\n", url);
		return false;
	}

	long res_code = 0;
	curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
	if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
	{
		printf("!!! Response code: %ld\n", res_code);
		return false;
	}

	curl_easy_cleanup(curlCtx);

	return true;
}
