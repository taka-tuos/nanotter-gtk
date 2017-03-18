# nanotter-gtk
とりあえずalpha版ということで。  
C99出身C99育ちですので脆弱性とかわかんないです。  
あったらissueに投げていただけると嬉しいです。  
Power for ておくれ。  
そのうちluaプラグイン実装します。  

## 必須ライブラリ
OpenSSL  
libcursesw  
libjson  
libcurl  
libgtk3  

## ビルド時の注意
twitcurlは付属のものを使ってください。
(Streaming APIに独自対応(超乱暴)させてます)

## ビルド方法
$ git clone https://github.com/taka-tuos/nanotter.git  
$ cd nanotter/twitcurl  
$ make && sudo make install  
$ cd ..  
$ make  


## 今のところは
ツイートとTL閲覧だけです。  
ふぁぼふぁぼしたりリツイートしたりツイ消ししたりはできません。  
