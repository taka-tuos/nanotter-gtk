/*
 * nanotter-gtk alpha-0.1
 * 
 * made by kagura1050 at 2017
 * 
 * TODO : GTK関係をスパゲティからそうめんにする
 */

#ifndef __NANOTTER_H__
#define __NANOTTER_H__

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

#include "stb_image.h"
#include "stb_image_write.h"

//　TL表示関数に渡す構造体
typedef struct {
	char *name;
	char *screen_name;
	char *text;
	unsigned char *profile;
	uint64_t id;
	size_t len;
} addTweet_t;

void			apiCheckAndDisplayError();
int				apiStreamingCallback(char* ptr, size_t size, size_t nmemb, void* data);
void*			apiStreamingThread(void* pParam);
void			apiOAuthTokenPIN();

void			timelineAddTweet(addTweet_t tweet);
void			timelienDoFavFav(GtkMenuItem *menuitem, gpointer user_data);
void			timelienDoUnFavFav(GtkMenuItem *menuitem, gpointer user_data);
void			timelienDoRetweet(GtkMenuItem *menuitem, gpointer user_data);
void			timelienDoUnRetweet(GtkMenuItem *menuitem, gpointer user_data);
int				timelineShowPopUp(GtkWidget *widget, GdkEvent *event);
void			timelineSelectedCallback(GtkListBox *box, GtkListBoxRow *row, gpointer id);
void			timelineWriteJson(char *str);

int				gtkAlphaMix(int c1, int c2, int alpha);
stbi_uc*		gtkAlphaNone(stbi_uc *bpp32, int w, int h);
char*			gtkGetTextTextview(GtkWidget *text_view);
void			gtkClickedTweetButton(GtkWidget *button, gpointer user_data);
void			gtkAddSeparatorListBox(GtkListBoxRow *row, GtkListBoxRow *before, gpointer data);
void			gtkMainWindowInit();

GdkPixbuf*		gdkLoadImage(const gchar *filename);

char*			strEncodeMarkup(char *s);
unsigned long	strGenerateHash(const char *sz);

size_t			curlDownloadCallbackfunction(void *ptr, size_t size, size_t nmemb, void* userdata);
bool			curlDownloadFile(unsigned char *work, const char* url);

// 人によっては怒る(場所的に)おまじない
using namespace std;

#endif

// 作者の一言：void関数多すぎない?
