#include <iostream>
#include <fstream>
#include <string>
#include <twitcurl.h>
#include <json/json.h>
#include <locale.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <malloc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
	char *name;
	char *screen_name;
	char *text;
	unsigned char *profile;
	uint64_t id;
	size_t len;
} addTweet_t;

using namespace std;

twitCurl g_twitterObj_Stream;
twitCurl g_twitterObj;

string g_strConsumerKey       = "Brbukpk7aHle3rIbSlpTvvleU";
string g_strConsumerSecret    = "PcoimOIhAhwHVK3SZ0NlSifX8PLjBcLT90FnKaiL5vgzVHG1hK";
string g_strAccessTokenKey    = "nil";
string g_strAccessTokenSecret = "nil";

string g_strRequestTokenUrl;

char *g_StreamBuffer = NULL;

size_t g_curlDownloadPtr = 0;

GtkWidget *g_boxTimeline;
GtkWidget *g_boxTweet;
GtkWidget *g_objWindow;

pthread_mutex_t g_objMutex;
pthread_t g_objTid1;

uint64_t g_idSelectedTL = 0;

uint64_t *g_arrayIdDListTL = NULL;
size_t g_arrayIdDListMax = 0;

int shaderAlphaMix(int c1, int c2, int alpha)
{
	int r, g, b;
	int r1, g1, b1;
	int r2, g2, b2;
	int i = 2;
	
	r1 = c1 >> 16;
	g1 = (c1 >> 8) & 0xff;
	b1 = c1 & 0xff;
	
	r2 = c2 >> 16;
	g2 = (c2 >> 8) & 0xff;
	b2 = c2 & 0xff;
	
	r = (r2 * alpha + r1 * (255 - alpha)) / 255;
	g = (g2 * alpha + g1 * (255 - alpha)) / 255;
	b = (b2 * alpha + b1 * (255 - alpha)) / 255;
	
	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	
	
	return (r << 16) | (g << 8) | b;
}

stbi_uc *renderAlphaNone(stbi_uc *bpp32, int w, int h)
{
	stbi_uc *bpp24 = (stbi_uc *)malloc(w*3*h);
	
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int di = (x + w * y) * 3;
			int si = (x + w * y);
			unsigned int src = ((unsigned int *)bpp32)[si];
			
			unsigned int dst = shaderAlphaMix(0xffffff, src & 0xffffff, src >> 24);
			
			int r1 = dst >> 16;
			int g1 = (dst >> 8) & 0xff;
			int b1 = dst & 0xff;
			
			bpp24[di+0] = b1;
			bpp24[di+1] = g1;
			bpp24[di+2] = r1;
		}
	}
	
	return bpp24;
}

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

GdkPixbuf *gdkLoadImage(const gchar *filename)
{
	GdkPixbuf *pixbuf;
	GError *error = NULL;
	
	pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	
	if (!pixbuf) {
		fprintf(stderr, "%s\n", error->message);
		g_error_free(error);
	}
	
	return pixbuf;
}

void timelienDoFavFav(GtkMenuItem *menuitem, gpointer user_data)
{
	ostringstream o;
	o << g_idSelectedTL;
	g_twitterObj.favoriteCreate(o.str());
	
	string resp;
	
	g_twitterObj.getLastWebResponse(resp);
	
	apiCheckAndDisplayError();
	
	cout << resp << endl;
}

void timelienDoUnFavFav(GtkMenuItem *menuitem, gpointer user_data)
{
	ostringstream o;
	o << g_idSelectedTL;
	g_twitterObj.favoriteDestroy(o.str());
	
	string resp;
	
	g_twitterObj.getLastWebResponse(resp);
	
	apiCheckAndDisplayError();
	
	cout << resp << endl;
}

void timelienDoRetweet(GtkMenuItem *menuitem, gpointer user_data)
{
	ostringstream o;
	o << g_idSelectedTL;
	g_twitterObj.retweetById("a");
	
	string resp;
	
	g_twitterObj.getLastWebResponse(resp);
	
	apiCheckAndDisplayError();
	
	cout << resp << endl;
}

void timelienDoUnRetweet(GtkMenuItem *menuitem, gpointer user_data)
{
	ostringstream o;
	o << g_idSelectedTL;
	g_twitterObj.unretweetById(o.str());
	
	string resp;
	
	g_twitterObj.getLastWebResponse(resp);
	
	apiCheckAndDisplayError();
	
	cout << resp << endl;
}

int timelineShowPopUp(GtkWidget *widget, GdkEvent *event)
{
	
	const gint RIGHT_CLICK = 3;
		
	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *bevent = (GdkEventButton *) event;
		
		if (bevent->button == RIGHT_CLICK) {      
			gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
		}
		return TRUE;
	}

	return FALSE;
}

void timelineDetachMenu(GtkWidget *attach_widget, GtkMenu *menu)
{
	return;
}

void timelineSelectedCallback(GtkListBox *box, GtkListBoxRow *row, gpointer id)
{
	int ind = gtk_list_box_row_get_index(row);
	
	g_idSelectedTL = g_arrayIdDListTL[(g_arrayIdDListMax-1) - ind];
	printf("id : %ld\n", g_idSelectedTL);
}

char *encodeMarkupText(char *s)
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

void addTweetToTimeLine(addTweet_t tweet)
{
	g_arrayIdDListTL = (uint64_t *)realloc(g_arrayIdDListTL,g_arrayIdDListMax*8);
	g_arrayIdDListTL[g_arrayIdDListMax] = tweet.id;
	g_arrayIdDListMax++;
	
	char *final_name = (char *)malloc(512);
	
	sprintf(final_name, "%s (@%s)", tweet.name, tweet.screen_name);
	
	int x,y,bpp;
	
	stbi_uc *dat = stbi_load_from_memory((stbi_uc *)tweet.profile,tweet.len,&x,&y,&bpp,4);
	
	stbi_uc *bpp24 = renderAlphaNone(dat,x,y);
	
	GdkPixbuf *pix = gdk_pixbuf_new_from_bytes(g_bytes_new(bpp24,malloc_usable_size(bpp24)),GDK_COLORSPACE_RGB,FALSE,8,x,y,x*3);
	
	GtkWidget *image = /*gtk_image_new_from_file("profile.png");*/gtk_image_new_from_pixbuf(pix);
	
	stbi_write_png("SIGSEGV.png",x,y,3,bpp24,x*3);
	
	printf("x,y,bpp = %d,%d,%d\nstbi_uc = %p\nbpp24 = %p\nGdkPixbuf = %p\nGtkWidget = %p\n",x,y,bpp,dat,bpp24,pix,image);
	
	GtkWidget *label_name = gtk_label_new(final_name);
	GtkWidget *label_text = gtk_label_new(NULL);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_label_set_use_markup(GTK_LABEL(label_text),TRUE);
	
	gtk_label_set_label(GTK_LABEL(label_text), encodeMarkupText(tweet.text));
	
	gtk_label_set_line_wrap(GTK_LABEL(label_text),true);
	
	gtk_box_pack_start(GTK_BOX(vbox), label_name, FALSE, FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(vbox), label_text, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	
	gtk_widget_set_halign(label_name, GTK_ALIGN_START);
	gtk_widget_set_halign(label_text, GTK_ALIGN_START);
	gtk_widget_set_valign(label_name, GTK_ALIGN_START);
	gtk_widget_set_valign(label_text, GTK_ALIGN_START);
	
	gtk_label_set_line_wrap(GTK_LABEL(label_text), TRUE);
	
	//gtk_box_pack_start(GTK_BOX(g_boxTimeline), hbox, FALSE, FALSE, 0);
	
	//gtk_list_item_new();
	
	gtk_list_box_insert(GTK_LIST_BOX(g_boxTimeline), hbox, 0);
	
	g_signal_connect(G_OBJECT(g_boxTimeline), "row-selected", G_CALLBACK(timelineSelectedCallback), NULL);
	
	gtk_widget_show_all(g_objWindow);
	gtk_widget_show_all(g_boxTimeline);
}

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

void writeDataToTimeLine(char *str)
{
	struct json_object *obj = json_tokener_parse(str);
	
	struct json_object *text,*user,*name,*screen_name,*profile_image_url,*id;
	json_object_object_get_ex(obj,"text",&text);
	if(text) {
		char *json_text,*json_name,*json_screen_name;
		int64_t json_id;
		
		json_text = (char *)json_object_get_string(text);
		
		json_object_object_get_ex(obj,"user",&user);
		
		json_object_object_get_ex(obj,"id",&id);
		json_id = json_object_get_int64(id);
		
		json_object_object_get_ex(user,"name",&name);
		json_name = (char *)json_object_get_string(name);
		
		json_object_object_get_ex(user,"screen_name",&screen_name);
		
		json_object_object_get_ex(user,"profile_image_url",&profile_image_url);
		
		json_screen_name = (char *)json_object_get_string(screen_name);
		
		unsigned char *work = (unsigned char *)malloc(65536);
		
		curlDownloadFile(work, json_object_get_string(profile_image_url));
		
		addTweet_t tweet;
		
		tweet.name = json_name;
		tweet.screen_name = json_screen_name;
		tweet.text = json_text;
		tweet.profile = work;
		tweet.len = g_curlDownloadPtr;
		tweet.id = json_id;
		
		addTweetToTimeLine(tweet);
		
		free(work);
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

static void addSeparatorListBox(GtkListBoxRow *row, GtkListBoxRow *before, gpointer data)
{
	if (!before)
		return;

	gtk_list_box_row_set_header (row, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
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
	
	apiCheckAndDisplayError();
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
				
				string resp;
			
				g_twitterObj.getLastWebResponse(resp);
				
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
	
	GdkPixbuf *icon = gdkLoadImage("res/logo.png");
	gtk_window_set_icon(GTK_WINDOW(g_objWindow),icon);
	
	GtkWidget *vbox;
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_container_add(GTK_CONTAINER(g_objWindow), vbox);
	
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scroll_window;
	
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	
	g_boxTimeline = gtk_list_box_new();
	
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(g_boxTimeline), GTK_SELECTION_SINGLE);
	gtk_list_box_set_header_func (GTK_LIST_BOX(g_boxTimeline), addSeparatorListBox, NULL, NULL);
	
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
	
	
	GtkWidget *pmenu;
	GtkWidget *favMi,*retMi,*unfavMi,*unretMi;
	
	pmenu = gtk_menu_new();
	
	favMi = gtk_menu_item_new_with_label("ふぁぼふぁぼする");
	gtk_widget_show(favMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), favMi);
	
	unfavMi = gtk_menu_item_new_with_label("あんふぁぼ");
	gtk_widget_show(unfavMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), unfavMi);
	
	retMi = gtk_menu_item_new_with_label("りつりつする");
	gtk_widget_show(retMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), retMi);
	
	unretMi = gtk_menu_item_new_with_label("あんりつ");
	gtk_widget_show(unretMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(pmenu), unretMi);
	
	//gtk_menu_attach_to_widget(GTK_MENU(pmenu), g_boxTimeline, timelineDetachMenu);
	g_signal_connect_swapped(G_OBJECT(scroll_window), "button-press-event", G_CALLBACK(timelineShowPopUp), pmenu);
	g_signal_connect_swapped(G_OBJECT(favMi), "activate", G_CALLBACK(timelienDoFavFav), NULL);
	g_signal_connect_swapped(G_OBJECT(unfavMi), "activate", G_CALLBACK(timelienDoUnFavFav), NULL);
	g_signal_connect_swapped(G_OBJECT(retMi), "activate", G_CALLBACK(timelienDoRetweet), NULL);
	g_signal_connect_swapped(G_OBJECT(unretMi), "activate", G_CALLBACK(timelienDoUnRetweet), NULL);
	
	gtk_widget_show_all(g_objWindow);
	
	pthread_create(&g_objTid1, NULL, apiStreamingThread, NULL);
	
	gtk_main();
	
	return 0;
}
