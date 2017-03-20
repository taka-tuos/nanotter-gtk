#include "nanotter.hpp"

using namespace std;

// twitcurlオブジェクト　ストリーム用とその他用に分けてある
twitCurl g_twitterObj_Stream;
twitCurl g_twitterObj;

// コンシューマーキーとかそのへん　実は下２つはずっと変わらない
string g_strConsumerKey       = "Brbukpk7aHle3rIbSlpTvvleU";
string g_strConsumerSecret    = "PcoimOIhAhwHVK3SZ0NlSifX8PLjBcLT90FnKaiL5vgzVHG1hK";
string g_strAccessTokenKey    = "nil";
string g_strAccessTokenSecret = "nil";

// 上から順に タイムライン(GtkListBox),ツイート欄(GtkTextView),メインウィンドウ(GtkWindow)
GtkWidget *g_boxTimeline;
GtkWidget *g_boxTweet;
GtkWidget *g_objWindow;

// 下はストリームスレッドのコンテキスト。上のミューテックスは使ってないが残す(たぶんいつか使う)
pthread_mutex_t g_objMutex;
pthread_t g_objTid1;

int main(int argc, char *argv[])
{
	gtk_init(&argc,&argv);
	
	// PINとかそのへんの処理
	apiOAuthTokenPIN();
	
	// GTKのWindow準備
	gtkMainWindowInit();
	
	// TLを流す(別スレッド)
	pthread_create(&g_objTid1, NULL, apiStreamingThread, NULL);
	
	// 特にやることなしのメインスレッド
	gtk_main();
	
	return 0;
}
