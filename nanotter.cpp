#include <iostream>
#include <fstream>
#include <string>
#include <twitcurl.h>
#include <json/json.h>
#include <locale.h>
#include <pthread.h>
#include <gtk/gtk.h>

using namespace std;

twitCurl g_twitterObj_Stream;
twitCurl g_twitterObj;

string g_strConsumerKey       = "Brbukpk7aHle3rIbSlpTvvleU";
string g_strConsumerSecret    = "PcoimOIhAhwHVK3SZ0NlSifX8PLjBcLT90FnKaiL5vgzVHG1hK";
string g_strAccessTokenKey    = "nil";
string g_strAccessTokenSecret = "nil";

string g_strRequestTokenUrl;

char *g_StreamBuffer = NULL;

GtkWidget *g_boxTimeline;
GtkWidget *g_boxTweet;
GtkWidget *g_objWindow;

pthread_mutex_t g_objMutex;
pthread_t g_objTid1;

void addTweetToTimeLine(char *name, char *screen_name, char *text)
{
	char *final_name = (char *)malloc(512);
	
	sprintf(final_name, "%s (@%s)", name, screen_name);
	
	GtkWidget *image = gtk_image_new_from_file("profile.png");
	
	GtkWidget *label_name = gtk_label_new(final_name);
	GtkWidget *label_text = gtk_label_new(text);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_box_pack_start(GTK_BOX(vbox), label_name, FALSE, FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(vbox), label_text, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	
	gtk_widget_set_halign(label_name, GTK_ALIGN_START);
	gtk_widget_set_halign(label_text, GTK_ALIGN_START);
	gtk_widget_set_valign(label_name, GTK_ALIGN_START);
	gtk_widget_set_valign(label_text, GTK_ALIGN_START);
	
	gtk_box_pack_start(GTK_BOX(g_boxTimeline), hbox, FALSE, FALSE, 0);
	
	gtk_widget_show_all(g_boxTimeline);
}

void writeDataToTimeLine(char *str)
{
	struct json_object *obj = json_tokener_parse(str);
	
	struct json_object *text,*user,*name,*screen_name;
	json_object_object_get_ex(obj,"text",&text);
	if(text) {
		char *json_text,*json_name,*json_screen_name;
		
		json_text = (char *)json_object_get_string(text);
		
		json_object_object_get_ex(obj,"user",&user);
		
		json_object_object_get_ex(user,"name",&name);
		json_name = (char *)json_object_get_string(name);
		
		json_object_object_get_ex(user,"screen_name",&screen_name);
		
		json_screen_name = (char *)json_object_get_string(screen_name);
		
		addTweetToTimeLine(json_name, json_screen_name, json_text);
	}
}

int apiStreamingCallback(char* ptr, size_t size, size_t nmemb, void* data) {
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
		writeDataToTimeLine(g_StreamBuffer);
		free(g_StreamBuffer);
		g_StreamBuffer = NULL;
	}

	return realsize;
}

void* apiStreamingThread(void* pParam)
{
	g_twitterObj_Stream.timelineHomeGetStream(apiStreamingCallback);
}

char *getTextofTextview(GtkWidget *text_view) {
	GtkTextIter start, end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gchar *text;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	return text;
}

static void clickedTweetButton(GtkWidget *button, gpointer user_data)
{
	g_twitterObj.statusUpdate(getTextofTextview(g_boxTweet));
}

int main(int argc, char *argv[])
{
	g_twitterObj.getOAuth().setConsumerKey(g_strConsumerKey);
	g_twitterObj.getOAuth().setConsumerSecret(g_strConsumerSecret);
	
	string token,tokensecret;
	
	gtk_init(&argc,&argv);
	
	// PINとかそのへんの処理
	{
		ifstream tokenfile(".nanotter");
		
		if(tokenfile.fail()) {
			g_twitterObj.oAuthRequestToken(g_strRequestTokenUrl);
			bool isValidPIN = false;
			while(isValidPIN == false) {
				GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
				gtk_widget_set_size_request(window, 320, 200);
				
				GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
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
				
				isValidPIN = g_twitterObj.oAuthAccessToken() && PIN.length();
				
				if(isValidPIN && PIN.length()) {
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
	}
	
	g_twitterObj_Stream.getOAuth().setConsumerKey(g_strConsumerKey);
	g_twitterObj_Stream.getOAuth().setConsumerSecret(g_strConsumerSecret);
	g_twitterObj_Stream.getOAuth().setOAuthTokenKey(token);
	g_twitterObj_Stream.getOAuth().setOAuthTokenSecret(tokensecret);
	
	g_objWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(g_objWindow, 640, 480);
	
	// TLを流す
	
	g_signal_connect(g_objWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show(g_objWindow);
	
	gtk_window_set_title(GTK_WINDOW(g_objWindow), "nanotter");
	gtk_container_set_border_width(GTK_CONTAINER(g_objWindow), 10);
	
	GtkWidget *vbox;
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_container_add(GTK_CONTAINER(g_objWindow), vbox);
	
	GtkWidget *hbox;
	
	GtkWidget *button;
	
	GtkWidget *scroll_window;
	
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	
	g_boxTimeline = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	
	g_boxTweet = gtk_text_view_new();
	button = gtk_button_new_with_label("ついーと");
	
	gtk_box_pack_start(GTK_BOX(hbox), g_boxTweet, TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(scroll_window), g_boxTimeline);
	
	gtk_box_pack_start(GTK_BOX(vbox), scroll_window, TRUE, TRUE, 0); 
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clickedTweetButton), NULL);
	
	gtk_widget_show_all(g_objWindow);
	
	pthread_create(&g_objTid1, NULL, apiStreamingThread, NULL);
	
	gtk_main();
	
	return 0;
}
