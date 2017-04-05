#include "nanotter.hpp"

extern twitCurl g_twitterObj_Stream;
extern twitCurl g_twitterObj;

extern string g_strConsumerKey;
extern string g_strConsumerSecret;
extern string g_strAccessTokenKey;
extern string g_strAccessTokenSecret;

extern GtkWidget *g_boxTimeline;
extern GtkWidget *g_boxTweet;
extern GtkWidget *g_objWindow;

// PIN認証のURL グローバルにすんなっていう意見が来そうである
string g_strRequestTokenUrl;

// 厚切りJSONの仕様がわかったのでその連結用バッファ
char *g_StreamBuffer = NULL;

/*
 * void apiCheckAndDisplayError();
 * 
 * 引数：
 * なし
 * 
 * 返り値：
 * なし
 * 
 * 直前のAPIでエラーがあったらダイアログを出す関数。
 * 仕組みはかなり乱暴。
 */
void apiCheckAndDisplayError()
{
	string json;
	
	g_twitterObj.getLastWebResponse(json);
	
	struct json_object *obj = json_tokener_parse(json.c_str());
	struct json_object *errors;
	json_object_object_get_ex(obj,"errors",&errors);
	
	if(errors) {
		for (int i = 0; i < json_object_array_length(errors); ++i) {
			struct json_object *ctx = json_object_array_get_idx(errors, i);
			struct json_object *message, *code;
			
			json_object_object_get_ex(ctx,"message",&message);
			json_object_object_get_ex(ctx,"code",&code);
			
			char *json_message = (char *)json_object_get_string(message);
			int json_code = json_object_get_int(code);
			
			GtkWidget *messagedialog;
			
			messagedialog = gtk_message_dialog_new(GTK_WINDOW(g_objWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error\n%s\n Code : %d",json_message,json_code);

			gtk_dialog_run(GTK_DIALOG(messagedialog));
			gtk_widget_destroy(messagedialog);
		}
	}
}

/*
 * int apiStreamingCallback(char* ptr, size_t size, size_t nmemb, void* data);
 * 
 * 引数：
 * CURLのリファレンス見てね(よくわかってないから)
 * 
 * 返り値：
 * CURLのリファレンス見てね(よくわかってないから)
 * 
 * JSONを受信するときのコールバック。
 * 厚切りJSONの対応もしてある。
 * 受け取ったJSONはすべてタイムラインに投げるというヤンチャ仕様。
 */
int apiStreamingCallback(char* ptr, size_t size, size_t nmemb, void* data)
{
	if (size * nmemb == 0)
		return 0;

	size_t realsize = size * nmemb;
	char str[32768];
	
	memcpy(str, ptr, realsize);
	
	if(realsize == 2) return realsize;
	
	str[realsize] = '\0';
	
	int cr = str[realsize - 2];
	int lf = str[realsize - 1];
	int len = 0;
	
	if(g_StreamBuffer) len = strlen(g_StreamBuffer);
	
	g_StreamBuffer = (char *)realloc(g_StreamBuffer, len + realsize + 1);
	
	memcpy(g_StreamBuffer + len, str, realsize + 1);
	
	if(cr == 0x0d && lf == 0x0a)
	{
		timelineWriteJson(g_StreamBuffer);
		free(g_StreamBuffer);
		g_StreamBuffer = NULL;
	}

	return realsize;
}

/*
 * void* apiStreamingThread(void* pParam);
 * 
 * 引数：
 * void* pParam : 未使用
 * 
 * 返り値：
 * なし
 * 
 * Streaming APIとTL表示に使うスレッド。
 * それにしか使ってない贅沢なスレッドである。
 */
void* apiStreamingThread(void* pParam)
{
	g_twitterObj_Stream.timelineHomeGetStream(apiStreamingCallback);
}

/*
 * void apiOAuthTokenPIN();
 * 
 * 引数：
 * なし
 * 
 * 返り値：
 * なし
 * 
 * PIN認証などを一括でやってくれるすごいやつ。
 * 初期から使ってるのでめたくそ汚い。
 */
void apiOAuthTokenPIN()
{
	g_twitterObj.getOAuth().setConsumerKey(g_strConsumerKey);
	g_twitterObj.getOAuth().setConsumerSecret(g_strConsumerSecret);
	
	string token,tokensecret;
	
	// PINとかそのへんの処理
	ifstream tokenfile(".nanotter");
	
	if(tokenfile.fail()) {
		g_twitterObj.oAuthRequestToken(g_strRequestTokenUrl);
		bool isValidPIN = false;
		while(isValidPIN == false) {
			GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_widget_set_size_request(window, 320, 200);
			
			GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
			GtkWidget *label = gtk_label_new(NULL);
			GtkWidget *button = gtk_button_new_with_label("OK");
			GtkWidget *entry = gtk_entry_new();
			
			string uri = "以下のページにアクセスして、\n"
						"表示されたPINコードを入力してください。\n"
						"<a href=\"" + 
						g_strRequestTokenUrl + 
						"\">連携アプリ設定ページ</a>";
			gtk_label_set_markup (GTK_LABEL(label), uri.c_str());
			
			gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);
			
			gtk_container_add(GTK_CONTAINER(window), vbox);
			
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(gtk_main_quit), NULL);
			g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
			
			gtk_widget_show_all(window);
			
			gtk_main();
			
			gtk_window_close(GTK_WINDOW(window));
			
			string PIN = (char *)gtk_entry_get_text(GTK_ENTRY(entry));
			
			g_twitterObj.getOAuth().setOAuthPin(PIN);
			
			g_twitterObj.oAuthAccessToken();
			
			string resp;
		
			g_twitterObj.getLastWebResponse(resp);
			
			apiCheckAndDisplayError();
			
			isValidPIN = (strncmp(resp.c_str(),"oauth",5) == 0);
			
			if(isValidPIN) {
				ofstream newtoken(".nanotter");
				
				g_twitterObj.getOAuth().getOAuthTokenKey(token);
				g_twitterObj.getOAuth().getOAuthTokenSecret(tokensecret);
				
				newtoken << token << endl;
				newtoken << tokensecret << endl;
			}
		}
	} else {
		tokenfile >> token;
		tokenfile >> tokensecret;
		
		g_twitterObj.getOAuth().setOAuthTokenKey(token);
		g_twitterObj.getOAuth().setOAuthTokenSecret(tokensecret);
	}
	
	g_twitterObj_Stream = *g_twitterObj.clone();
}
