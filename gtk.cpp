#include "nanotter.hpp"

extern GtkWidget *g_boxTimeline;
extern GtkWidget *g_boxTweet;
extern GtkWidget *g_objWindow;

extern twitCurl g_twitterObj_Stream;
extern twitCurl g_twitterObj;

/*
 * int gtkAlphaMix(int c1, int c2, int alpha);
 * 
 * 引数：
 * int c1 : 合成したい色(背景)
 * int c2 : 合成したい色(前景)
 * int alpha : アルファ値
 * 
 * 返り値：
 * int : ブレンドした色
 * 
 * 色をアルファブレンディングする関数、32bpp用。
 * 32bpp->24bppにする際に白とブレンドしてるとこで使用。
 */
int gtkAlphaMix(int c1, int c2, int alpha)
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

/*
 * stbi_uc *gtkAlphaNone(stbi_uc *bpp32, int w, int h);
 * 
 * 引数：
 * stbi_uc *bpp32 : 32bpp画像
 * int w : 画像幅
 * int h : 画像高さ
 * 
 * 返り値：
 * stbi_uc * : 24bppの画像
 * 
 * 32bpp->24bpp変換関数。
 * TLの画像表示で使用。
 */
stbi_uc *gtkAlphaNone(stbi_uc *bpp32, int w, int h)
{
	stbi_uc *bpp24 = (stbi_uc *)malloc(w*3*h);
	
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int di = (x + w * y) * 3;
			int si = (x + w * y);
			unsigned int src = ((unsigned int *)bpp32)[si];
			
			unsigned int dst = gtkAlphaMix(0xffffff, src & 0xffffff, src >> 24);
			
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

/*
 * GdkPixbuf *gdkLoadImage(const gchar *filename);
 * 
 * 引数：
 * const gchar *filename : 画像ファイル名
 * 
 * 返り値：
 * GdkPixbuf * : 読み込んだPixBuf
 * 
 * 画像読み込み関数。
 * …使ってたっけこいつ？
 */
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

/*
 * char *gtkGetTextTextview(GtkWidget *text_view);
 * 
 * 引数：
 * GtkWidget *text_view : GtkTextViewのWidget
 * 
 * 返り値：
 * char * : text_viewのテキスト
 * 
 * ツイート関数で使用。
 * stringで返してもよかった気もしないでもない
 */
char *gtkGetTextTextview(GtkWidget *text_view)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gchar *text;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	return text;
}

/*
 * void gtkClickedTweetButton(GtkWidget *button, gpointer user_data);
 * 
 * 引数：
 * GTKのリファレンス見てね(しっかりはわかってないから)
 * 
 * 返り値：
 * なし
 * 
 * ツイート関数で使用。
 * ツイートする機能を集約した、謂わば「ツイッタラーの命」。
 */
void gtkClickedTweetButton(GtkWidget *button, gpointer user_data)
{
	g_twitterObj.statusUpdate(gtkGetTextTextview(g_boxTweet));
	
	apiCheckAndDisplayError();
}

/*
 * void gtkAddSeparatorListBox(GtkListBoxRow *row, GtkListBoxRow *before, gpointer data);
 * 
 * 引数：
 * コピペだからよくわかんない
 * 
 * 返り値：
 * なし
 * 
 * TL表示で使用。
 * コピペなので中身はよくわからない。
 */
void gtkAddSeparatorListBox(GtkListBoxRow *row, GtkListBoxRow *before, gpointer data)
{
	if (!before)
		return;

	gtk_list_box_row_set_header(row, gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
}

/*
 * void gtkMainWindowInit();
 * 
 * 引数：
 * なし
 * 
 * 返り値：
 * なし
 * 
 * GTkの初期化とか全部やってくれるすごいやつ。
 * 但しあんかけスパゲッティ。
 */
void gtkMainWindowInit()
{
	g_objWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(g_objWindow, 640, 480);
	
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
	gtk_list_box_set_header_func (GTK_LIST_BOX(g_boxTimeline), gtkAddSeparatorListBox, NULL, NULL);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	
	g_boxTweet = gtk_text_view_new();
	button = gtk_button_new_with_label("ついーと");
	
	gtk_box_pack_start(GTK_BOX(hbox), g_boxTweet, TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(scroll_window), g_boxTimeline);
	
	gtk_box_pack_start(GTK_BOX(vbox), scroll_window, TRUE, TRUE, 0); 
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(gtkClickedTweetButton), NULL);
	
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
}
