#include "nanotter.hpp"

/*
 * char *strEncodeMarkup(char *s);
 * 
 * 引数：
 * char *s : 自動マークアップしてほしい文字列
 * 
 * 返り値：
 * char * : マークアップ化された文字列
 * 
 * 文字列内のURLを検知してマークアップする関数。
 * 適当に作ったので処理が荒い。
 */
char *strEncodeMarkup(char *s)
{
	string str = "";
	string uri = "";
	int mode = 0;
	while(*s) {
		if(strncmp(s,"http",4) == 0) {
			mode = 1;
			str += "<a href=\"";
		}
		
		if(mode) uri += *s;
		
		if(*s == 0x0d || *s == 0x0a || *s == ' ') {
			if(mode) {
				mode = 0;
				str += "\">" + uri + "</a>";
				uri = "";
			}
		}
		str += *s;
		s++;
	}
	
	if(mode) str += "\">" + uri + "</a>";
	
	cout << str << endl;
	
	char *ret = (char *)malloc(512);
	
	strcpy(ret,str.c_str());
	
	return ret;
}

/*
 * unsigned long strGenerateHash(const char *sz);
 * 
 * 引数：
 * const char *sz : ハッシュ化したい文字列
 * 
 * 返り値：
 * unsigned long : ハッシュ値
 * 
 * アルゴリズム?知らんな。
 * ここまでやったらダブらないだろうという謎ハッシュ関数。
 * はっきり言って深夜のノリである。
 * たぶん今は使ってない。
 */
unsigned long strGenerateHash(const char *sz)
{
	unsigned long r = 0, s = 0, t = 0;
	size_t len = strlen(sz);
	
	for(;len;sz++,len--) {
		r += *sz;
		s ^= *sz << (t & 23);
		r ^= s;
	}
	
	return r << 32 | s;
}
