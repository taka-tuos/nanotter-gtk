#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "nanotter.hpp"

extern GtkWidget *g_boxTimeline;
extern GtkWidget *g_boxTweet;
extern GtkWidget *g_objWindow;

extern twitCurl g_twitterObj_Stream;
extern twitCurl g_twitterObj;

extern size_t g_curlDownloadPtr;

// TLでいま選択されているツイートのID
uint64_t g_idSelectedTL = 0;

// TLのツイートのIDリスト
uint64_t *g_arrayIdDListTL = NULL;
size_t g_arrayIdDListMax = 0;


/*
 * void timelineAddTweet(addTweet_t tweet);
 * 
 * 引数：
 * addTweet_t tweet : tweet内容の構造体
 * 
 * 返り値：
 * なし
 * 
 * 分けてあるからと侮るなかれ
 * 和風スパである。
 * ツイートをTLに追加する関数。
 */
void timelineAddTweet(addTweet_t tweet)
{
	g_arrayIdDListTL = (uint64_t *)realloc(g_arrayIdDListTL,g_arrayIdDListMax*8);
	g_arrayIdDListTL[g_arrayIdDListMax] = tweet.id;
	g_arrayIdDListMax++;
	
	char *final_name = (char *)malloc(512);
	
	sprintf(final_name, "%s (@%s)", tweet.name, tweet.screen_name);
	
	int x,y,bpp;
	
	stbi_uc *dat = stbi_load_from_memory((stbi_uc *)tweet.profile,tweet.len,&x,&y,&bpp,4);
	
	stbi_uc *bpp24 = gtkAlphaNone(dat,x,y);
	
	GdkPixbuf *pix = gdk_pixbuf_new_from_bytes(g_bytes_new(bpp24,malloc_usable_size(bpp24)),GDK_COLORSPACE_RGB,FALSE,8,x,y,x*3);
	
	GtkWidget *image = /*gtk_image_new_from_file("profile.png");*/gtk_image_new_from_pixbuf(pix);
	
	stbi_write_png("SIGSEGV.png",x,y,3,bpp24,x*3);
	
	printf("x,y,bpp = %d,%d,%d\nstbi_uc = %p\nbpp24 = %p\nGdkPixbuf = %p\nGtkWidget = %p\n",x,y,bpp,dat,bpp24,pix,image);
	
	GtkWidget *label_name = gtk_label_new(final_name);
	GtkWidget *label_text = gtk_label_new(NULL);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	
	gtk_label_set_use_markup(GTK_LABEL(label_text),TRUE);
	
	gtk_label_set_label(GTK_LABEL(label_text), strEncodeMarkup(tweet.text));
	
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
	
	gtk_list_box_insert(GTK_LIST_BOX(g_boxTimeline), hbox, 0);
	
	g_signal_connect(G_OBJECT(g_boxTimeline), "row-selected", G_CALLBACK(timelineSelectedCallback), NULL);
	
	gtk_widget_show_all(g_objWindow);
	gtk_widget_show_all(g_boxTimeline);
}

/*
 * void timelienDoFavFav(GtkMenuItem *menuitem, gpointer user_data);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * なし
 * 
 * ふぁぼふぁぼする関数。
 * ふぁぼ爆モードとかはない。
 */
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

/*
 * void timelienDoUnFavFav(GtkMenuItem *menuitem, gpointer user_data);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * なし
 * 
 * あんふぁぼ関数。
 * 冷め切ったふぁぼを回収する関数。
 * テストはしてない。
 */
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

/*
 * void timelienDoRetweet(GtkMenuItem *menuitem, gpointer user_data);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * なし
 * 
 * リツイート関数。
 * ておくれツイートを拡散する関数。
 * みんな　そろって　ておくれろ！
 */
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

/*
 * void timelienDoUnRetweet(GtkMenuItem *menuitem, gpointer user_data);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * なし
 * 
 * アンリツイート関数。
 * 炎上してておくれている時につかう関数(焼け石に水)。
 * 広がったら二度と消えないのがネットである。
 */
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

/*
 * int timelineShowPopUp(GtkWidget *widget, GdkEvent *event);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * ふぁぼったり拡散するためのメニューを出す関数。
 * これは「ミクッタラーの命」。
 */
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

/*
 * void timelineSelectedCallback(GtkListBox *box, GtkListBoxRow *row, gpointer id);
 * 
 * 引数：
 * GTKのリファレンス見てね(わかってないから)
 * 
 * 返り値：
 * なし
 * 
 * TLで選択したツイートのIDを保存する関数。
 * なんか複数回呼ばれてるけどキニシナイ。
 */
void timelineSelectedCallback(GtkListBox *box, GtkListBoxRow *row, gpointer id)
{
	int ind = gtk_list_box_row_get_index(row);
	
	g_idSelectedTL = g_arrayIdDListTL[(g_arrayIdDListMax-1) - ind];
	printf("id : %ld\n", g_idSelectedTL);
}

/*
 * void timelineWriteJson(char *str);
 * 
 * 引数：
 * char *str : JSON
 * 
 * 返り値：
 * なし
 * 
 * 結合された厚切りJSONたちを解析してTLに投げる関数。
 * かなり乱暴な仕組み。
 */
void timelineWriteJson(char *str)
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
		
		timelineAddTweet(tweet);
		
		free(work);
	}
}
