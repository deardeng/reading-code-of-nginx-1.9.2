
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/*
一．正则表达式匹配，其中：
* ~ 为区分大小写匹配
* ~* 为不区分大小写匹配
* !~和!~*分别为区分大小写不匹配及不区分大小写不匹配
二．文件及目录匹配，其中：
* -f和!-f用来判断是否存在文件
* -d和!-d用来判断是否存在目录
* -e和!-e用来判断是否存在文件或目录
* -x和!-x用来判断文件是否可执行
三．rewrite指令的最后一项参数为flag标记，flag标记有：
1.last    相当于apache里面的[L]标记，表示rewrite。
2.break本条规则匹配完成后，终止匹配，不再匹配后面的规则。
3.redirect  返回302临时重定向，浏览器地址会显示跳转后的URL地址。
4.permanent  返回301永久重定向，浏览器地址会显示跳转后的URL地址。


使用last和break实现URI重写，浏览器地址栏不变。而且两者有细微差别，使用alias指令必须用last标记;使用proxy_pass指令时，需要使用break标记。Last标记在本条rewrite规则执行完毕后，会对其所在server{......}标签重新发起请求，而break标记则在本条规则匹配完成后，终止匹配。
例如：如果我们将类似URL/photo/123456 重定向到/path/to/photo/12/1234/123456.png
rewrite "/photo/([0-9]{2})([0-9]{2})([0-9]{2})"/path/to/photo/$1/$1$2/$1$2$3.png ;


四．NginxRewrite 规则相关指令


1.break指令
使用环境：server,location,if;
该指令的作用是完成当前的规则集，不再处理rewrite指令。


2.if指令
使用环境：server,location
该指令用于检查一个条件是否符合，如果条件符合，则执行大括号内的语句。If指令不支持嵌套，不支持多个条件&&和||处理。


3.return指令
语法：returncode ;
使用环境：server,location,if;
该指令用于结束规则的执行并返回状态码给客户端。
示例：如果访问的URL以".sh"或".bash"结尾，则返回403状态码
location ~ .*\.(sh|bash)?$
{
return 403;
}


4.rewrite 指令
语法：rewriteregex replacement flag
使用环境：server,location,if
该指令根据表达式来重定向URI，或者修改字符串。指令根据配置文件中的顺序来执行。注意重写表达式只对相对路径有效。如果你想配对主机名，你应该使用if语句，示例如下：
if( $host ~* www\.(.*) )
{
set $host_without_www $1;
rewrite ^(.*)$  http://$host_without_www$1permanent;
}


5.Set指令
语法：setvariable value ; 默认值:none; 使用环境：server,location,if;
该指令用于定义一个变量，并给变量赋值。变量的值可以为文本、变量以及文本变量的联合。
示例：set$varname "hello world";


6.Uninitialized_variable_warn指令
语法：uninitialized_variable_warnon|off
使用环境：http,server,location,if
该指令用于开启和关闭未初始化变量的警告信息，默认值为开启。

 


五．Nginx的Rewrite规则编写实例
1.当访问的文件和目录不存在时，重定向到某个php文件
if( !-e $request_filename )
{
rewrite ^/(.*)$ index.php last;
}


2.目录对换 /123456/xxxx  ====>  /xxxx?id=123456
rewrite ^/(\d+)/(.+)/  /$2?id=$1 last;


3.如果客户端使用的是IE浏览器，则重定向到/ie目录下
if( $http_user_agent  ~ MSIE)
{
rewrite ^(.*)$ /ie/$1 break;
}


4.禁止访问多个目录
location ~ ^/(cron|templates)/
{
deny all;
break;
}


5.禁止访问以/data开头的文件
location ~ ^/data
{
deny all;
}


6.禁止访问以.sh,.flv,.mp3为文件后缀名的文件
location ~ .*\.(sh|flv|mp3)$
{
return 403;
}


7.设置某些类型文件的浏览器缓存时间
location ~ .*\.(gif|jpg|jpeg|png|bmp|swf)$
{
expires 30d;
}
location ~ .*\.(js|css)$
{
expires 1h;
}


8.给favicon.ico和robots.txt设置过期时间;
这里为favicon.ico为99天,robots.txt为7天并不记录404错误日志
location ~(favicon.ico) {
log_not_found off;
expires 99d;
break;
}
location ~(robots.txt) {
log_not_found off;
expires 7d;
break;
}


9.设定某个文件的过期时间;这里为600秒，并不记录访问日志
location ^~ /html/scripts/loadhead_1.js {
access_log  off;
root /opt/lampp/htdocs/web;
expires 600;
break;
}


10.文件反盗链并设置过期时间
这里的return412 为自定义的http状态码，默认为403，方便找出正确的盗链的请求
“rewrite ^/ http://img.linuxidc.net/leech.gif;”显示一张防盗链图片
“access_log off;”不记录访问日志，减轻压力
“expires 3d”所有文件3天的浏览器缓存


location ~*^.+\.(jpg|jpeg|gif|png|swf|rar|zip|css|js)$ {
valid_referers none blocked *.linuxidc.com*.linuxidc.net localhost 208.97.167.194;
if ($invalid_referer) {
rewrite ^/ http://img.linuxidc.net/leech.gif;
return 412;
break;
}
access_log  off;
root /opt/lampp/htdocs/web;
expires 3d;
break;
}


11.只允许固定ip访问网站，并加上密码


root /opt/htdocs/www;
allow  208.97.167.194; 
allow  222.33.1.2; 
allow  231.152.49.4;
deny  all;
auth_basic “C1G_ADMIN”;
auth_basic_user_file htpasswd;


12将多级目录下的文件转成一个文件，增强seo效果
/job-123-456-789.html 指向/job/123/456/789.html


rewrite^/job-([0-9]+)-([0-9]+)-([0-9]+)\.html$ /job/$1/$2/jobshow_$3.html last;


13.文件和目录不存在的时候重定向：


if (!-e $request_filename) {
proxy_pass http://127.0.0.1;
}


14.将根目录下某个文件夹指向2级目录
如/shanghaijob/ 指向 /area/shanghai/
如果你将last改成permanent，那么浏览器地址栏显是/location/shanghai/
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;
上面例子有个问题是访问/shanghai时将不会匹配
rewrite ^/([0-9a-z]+)job$ /area/$1/ last;
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;
这样/shanghai 也可以访问了，但页面中的相对链接无法使用，
如./list_1.html真实地址是/area/shanghia/list_1.html会变成/list_1.html,导至无法访问。
那我加上自动跳转也是不行咯
(-d $request_filename)它有个条件是必需为真实目录，而我的rewrite不是的，所以没有效果
if (-d $request_filename){
rewrite ^/(.*)([^/])$ http://$host/$1$2/permanent;
}
知道原因后就好办了，让我手动跳转吧
rewrite ^/([0-9a-z]+)job$ /$1job/permanent;
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;


15.域名跳转
server
{
listen      80;
server_name  jump.linuxidc.com;
index index.html index.htm index.php;
root  /opt/lampp/htdocs/www;
rewrite ^/ http://www.linuxidc.com/;
access_log  off;
}


16.多域名转向
server_name  www.linuxidc.comwww.linuxidc.net;
index index.html index.htm index.php;
root  /opt/lampp/htdocs;
if ($host ~ "linuxidc\.net") {
rewrite ^(.*) http://www.linuxidc.com$1permanent;
}


六．nginx全局变量
arg_PARAMETER    #这个变量包含GET请求中，如果有变量PARAMETER时的值。
args                    #这个变量等于请求行中(GET请求)的参数，如：foo=123&bar=blahblah;
binary_remote_addr #二进制的客户地址。
body_bytes_sent    #响应时送出的body字节数数量。即使连接中断，这个数据也是精确的。
content_length    #请求头中的Content-length字段。
content_type      #请求头中的Content-Type字段。
cookie_COOKIE    #cookie COOKIE变量的值
document_root    #当前请求在root指令中指定的值。
document_uri      #与uri相同。
host                #请求主机头字段，否则为服务器名称。
hostname          #Set to themachine’s hostname as returned by gethostname
http_HEADER
is_args              #如果有args参数，这个变量等于”?”，否则等于”"，空值。
http_user_agent    #客户端agent信息
http_cookie          #客户端cookie信息
limit_rate            #这个变量可以限制连接速率。
query_string          #与args相同。
request_body_file  #客户端请求主体信息的临时文件名。
request_method    #客户端请求的动作，通常为GET或POST。
remote_addr          #客户端的IP地址。
remote_port          #客户端的端口。
remote_user          #已经经过Auth Basic Module验证的用户名。
request_completion #如果请求结束，设置为OK. 当请求未结束或如果该请求不是请求链串的最后一个时，为空(Empty)。
request_method    #GET或POST
request_filename  #当前请求的文件路径，由root或alias指令与URI请求生成。
request_uri          #包含请求参数的原始URI，不包含主机名，如：”/foo/bar.php?arg=baz”。不能修改。
scheme                #HTTP方法（如http，https）。
server_protocol      #请求使用的协议，通常是HTTP/1.0或HTTP/1.1。
server_addr          #服务器地址，在完成一次系统调用后可以确定这个值。
server_name        #服务器名称。
server_port          #请求到达服务器的端口号。


七．Apache和Nginx规则的对应关系
Apache的RewriteCond对应Nginx的if
Apache的RewriteRule对应Nginx的rewrite
Apache的[R]对应Nginx的redirect
Apache的[P]对应Nginx的last
Apache的[R,L]对应Nginx的redirect
Apache的[P,L]对应Nginx的last
Apache的[PT,L]对应Nginx的last


例如：允许指定的域名访问本站，其他的域名一律转向www.linuxidc.net
  Apache:
RewriteCond %{HTTP_HOST} !^(.*?)\.aaa\.com$[NC]
RewriteCond %{HTTP_HOST} !^localhost$ 
RewriteCond %{HTTP_HOST}!^192\.168\.0\.(.*?)$
RewriteRule ^/(.*)$ http://www.linuxidc.net[R,L]


  Nginx:
if( $host ~* ^(.*)\.aaa\.com$ )
{
set $allowHost ‘1’;
}
if( $host ~* ^localhost )
{
set $allowHost ‘1’;
}
if( $host ~* ^192\.168\.1\.(.*?)$ )
{
set $allowHost ‘1’;
}
if( $allowHost !~ ‘1’ )
{
rewrite ^/(.*)$ http://www.linuxidc.netredirect ;
}

本篇文章来源于 Linux公社网站(www.linuxidc.com)  原文链接：http://www.linuxidc.com/Linux/2014-01/95493.htm



















Nginx Rewrite详解 
Nginx Rewrite详解

引用链接：http://blog.cafeneko.info/2010/10/nginx_rewrite_note/

 

原文如下：

 

在新主机的迁移过程中,最大的困难就是WP permalink rewrite的设置.

因为旧主机是用的Apache, 使用的是WP本身就可以更改的.htaccess,没有太大的难度.而这次在VPS上跑的是Nginx,主要是因为Nginx的速度比Apache要快很多.

但是另一方面就不是那么舒服了,因为Nginx的rewrite跟Apache不同,而且是在服务器上面才能更改.

下面是其间的一些研究笔记.(以下用例如无特别说明均摘自nginx wiki)

 

/1 Nginx rewrite基本语法
 

Nginx的rewrite语法其实很简单.用到的指令无非是这几个

?set
?if
?return
?break
?rewrite
麻雀虽小,可御可萝五脏俱全.只是简单的几个指令却可以做出绝对不输apache的简单灵活的配置.

1.set

set主要是用来设置变量用的,没什么特别的

2.if

if主要用来判断一些在rewrite语句中无法直接匹配的条件,比如检测文件存在与否,http header,cookie等,

用法: if(条件) {…}

- 当if表达式中的条件为true,则执行if块中的语句

- 当表达式只是一个变量时,如果值为空或者任何以0开头的字符串都会当作false

- 直接比较内容时,使用 = 和 !=

- 使用正则表达式匹配时,使用

~ 大小写敏感匹配   YANG ADD 应该算完全匹配
~* 大小写不敏感匹配  yang add 应该是不区分大小写匹配
!~ 大小写敏感不匹配   yang add，如果完全匹配，则为假
!~* 大小写不敏感不匹配 yang add 如果不区分大小写匹配成功，返回假

这几句话看起来有点绕,总之记住: ~为正则匹配, 后置*为大小写不敏感, 前置!为”非”操作

随便一提,因为nginx使用花括号{}判断区块,所以当正则中包含花括号时,则必须用双引号将正则包起来.对下面讲到的rewrite语句中的正则亦是如此. 
比如 “\d{4}\d{2}\.+”

- 使用-f,-d,-e,-x检测文件和目录

-f 检测文件存在
-d 检测目录存在
-e 检测文件,目录或者符号链接存在
-x 检测文件可执行

跟~类似,前置!则为”非”操作

举例

if ($http_user_agent ~ MSIE) {
  rewrite  ^(.*)$  /msie/$1  break;
}//如果UA包含”MSIE”,rewrite 请求到/msie目录下

if ($http_cookie ~* "id=([^;] +)(?:;|$)" ) {
  set  $id  $1;
}//如果cookie匹配正则,设置变量$id等于正则引用部分

if ($request_method = POST ) {
  return 405;
}//如果提交方法为POST,则返回状态405 (Method not allowed)

if (!-f $request_filename) {
  break;
  proxy_pass  http://127.0.0.1;
}//如果请求文件名不存在,则反向代理localhost

if ($args ~ post=140){
  rewrite ^ http://example.com/ permanent;
}//如果query string中包含”post=140″,永久重定向到example.com

3.return

return可用来直接设置HTTP返回状态,比如403,404等(301,302不可用return返回,这个下面会在rewrite提到)

4.break

立即停止rewrite检测,跟下面讲到的rewrite的break flag功能是一样的,区别在于前者是一个语句,后者是rewrite语句的flag

5.rewrite

最核心的功能(废话)

用法: rewrite 正则 替换 标志位

其中标志位有四种

break – 停止rewrite检测,也就是说当含有break flag的rewrite语句被执行时,该语句就是rewrite的最终结果 
last – 停止rewrite检测,但是跟break有本质的不同,last的语句不一定是最终结果,这点后面会跟nginx的location匹配一起提到 
redirect – 返回302临时重定向,一般用于重定向到完整的URL(包含http:部分) 
permanent – 返回301永久重定向,一般用于重定向到完整的URL(包含http:部分)

因为301和302不能简单的只单纯返回状态码,还必须有重定向的URL,这就是return指令无法返回301,302的原因了. 作为替换,rewrite可以更灵活的使用redirect和permanent标志实现301和302. 比如上一篇日志中提到的Blog搬家要做的域名重定向,在nginx中就会这么写

rewrite ^(.*)$ http://newdomain.com/ permanent;举例来说一下rewrite的实际应用

rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  last;如果请求为 /download/eva/media/op1.mp3 则请求被rewrite到 /download/eva/mp3/op1.mp3

使用起来就是这样,很简单不是么? 不过要注意的是rewrite有很多潜规则需要注意

- rewrite的生效区块为sever, location, if

- rewrite只对相对路径进行匹配,不包含hostname 比如说以上面301重定向的例子说明

rewrite ~* cafeneko\.info http://newdomain.com/ permanent;这句是永远无法执行的,以这个URL为例

http://blog.cafeneko.info/2010/10/neokoseseiki_in_new_home/?utm_source=rss&utm_medium=rss&utm_campaign=neokoseseiki_in_new_home

其中cafeneko.info叫做hostname,再往后到?为止叫做相对路径,?后面的一串叫做query string

对于rewrite来说,其正则表达式仅对”/2010/10/neokoseseiki_in_new_home”这一部分进行匹配,即不包含hostname,也不包含query string .所以除非相对路径中包含跟域名一样的string,否则是不会匹配的. 如果非要做域名匹配的话就要使用if语句了,比如进行去www跳转

if ($host ~* ^www\.(cafeneko\.info)) {
  set $host_without_www $1;
  rewrite ^(.*)$ http://$host_without_www$1 permanent;
}- 使用相对路径rewrite时,会根据HTTP header中的HOST跟nginx的server_name匹配后进行rewrite,如果HOST不匹配或者没有HOST信息的话则rewrite到server_name设置的第一个域名,如果没有设置server_name的话,会使用本机的localhost进行rewrite

- 前面提到过,rewrite的正则是不匹配query string的,所以默认情况下,query string是自动追加到rewrite后的地址上的,如果不想自动追加query string,则在rewrite地址的末尾添加?

rewrite  ^/users/(.*)$  /show?user=$1?  last;rewrite的基本知识就是这么多..但还没有完..还有最头疼的部分没有说…

 

/2 Nginx location 和 rewrite retry
 

nginx的rewrite有个很奇特的特性 — rewrite后的url会再次进行rewrite检查,最多重试10次,10次后还没有终止的话就会返回HTTP 500

用过nginx的朋友都知道location区块,location区块有点像Apache中的RewriteBase,但对于nginx来说location是控制的级别而已,里面的内容不仅仅是rewrite.

这里必须稍微先讲一点location的知识.location是nginx用来处理对同一个server不同的请求地址使用独立的配置的方式

举例:

location  = / {
  ....配置A
}
 
location  / {
  ....配置B
}
 
location ^~ /images/ {
  ....配置C
}
 
location ~* \.(gif|jpg|jpeg)$ {
  ....配置D
}访问 / 会使用配置A 
访问 /documents/document.html 会使用配置B 
访问 /images/1.gif 会使用配置C 
访问 /documents/1.jpg 会使用配置D

如何判断命中哪个location暂且按下不婊, 我们在实战篇再回头来看这个问题.

现在我们只需要明白一个情况: nginx可以有多个location并使用不同的配.

sever区块中如果有包含rewrite规则,则会最先执行,而且只会执行一次, 然后再判断命中哪个location的配置,再去执行该location中的rewrite, 当该location中的rewrite执行完毕时,rewrite并不会停止,而是根据rewrite过的URL再次判断location并执行其中的配置. 那么,这里就存在一个问题,如果rewrite写的不正确的话,是会在location区块间造成无限循环的.所以nginx才会加一个最多重试10次的上限. 比如这个例子

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  last;
}如果请求为 /download/eva/media/op1.mp3 则请求被rewrite到 /download/eva/mp3/op1.mp3

结果rewrite的结果重新命中了location /download/ 虽然这次并没有命中rewrite规则的正则表达式,但因为缺少终止rewrite的标志,其仍会不停重试download中rewrite规则直到达到10次上限返回HTTP 500

认真的朋友这时就会问了,上面的rewrite规则不是有标志位last么? last不是终止rewrite的意思么?

说到这里我就要抱怨下了,网上能找到关于nginx rewrite的文章中80%对last标志的解释都是

last – 基本上都用这个Flag

……这他妈坑爹呢!!! 什么叫基本上都用? 什么是不基本的情况?  =皿=

有兴趣的可以放狗”基本上都用这个Flag”…

我最终还是在stack overflow找到了答案:

last和break最大的不同在于

- break是终止当前location的rewrite检测,而且不再进行location匹配 
– last是终止当前location的rewrite检测,但会继续重试location匹配并处理区块中的rewrite规则

还是这个该死的例子

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  ;
  rewrite  ^(/download/.*)/movie/(.*)\..*$  $1/avi/$2.mp3  ;
  rewrite  ^(/download/.*)/avvvv/(.*)\..*$  $1/rmvb/$2.mp3 ;
}上面没有写标志位,请各位自行脑补…

如果请求为 /download/acg/moive/UBW.avi

last的情况是: 在第2行rewrite处终止,并重试location /download..死循环 
break的情况是: 在第2行rewrite处终止,其结果为最终的rewrite地址.

也就是说,上面的某位试图下载eva op不但没下到反而被HTTP 500射了一脸的例子正是因为用了last标志所以才会造成死循环,如果用break就没事了.

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  break;
}对于这个问题,我个人的建议是,如果是全局性质的rewrite,最好放在server区块中并减少不必要的location区块.location区块中的rewrite要想清楚是用last还是break.

有人可能会问,用break不就万无一失了么?

不对.有些情况是要用last的. 典型的例子就是wordpress的permalink rewrite

常见的情况下, wordpress的rewrite是放在location /下面,并将请求rewrite到/index.php

这时如果这里使用break乃就挂了,不信试试. ｂ（￣▽￣）ｄ…因为nginx返回的是没有解释的index.php的源码…

这里一定要使用last才可以在结束location / 的rewrite, 并再次命中location ~ \.php$,将其交给fastcgi进行解释.其后返回给浏览器的才是解释过的html代码.

关于nginx rewrite的简介到这里就全部讲完了,水平及其有限,请大家指出错漏…

 

/3 实战! WordPress的Permalink+Supercache rewrite实现
 

这个rewrite写法其实是来自supercache作者本家的某个评论中,网上很容易查到,做了一些修改. 先给出该配置文件的全部内容..部分内容码掉了..绝对路径什么的你知道也没啥用对吧?

server {
	listen   80;
	server_name  cafeneko.info www.cafeneko.info;
 
	access_log  ***;
	error_log   *** ;
 
	root   ***;
	index  index.php;
 
	gzip_static on;
 
	if (-f $request_filename) {
		break;
	}
 
	set $supercache_file '';
	set $supercache_uri $request_uri;
 
	if ($request_method = POST) {
		set $supercache_uri '';
	}
 
	if ($query_string) {
		set $supercache_uri '';
	}
 
	if ($http_cookie ~* "comment_author_|wordpress_logged_|wp-postpass_" ) {
		set $supercache_uri '';
	}
 
	if ($supercache_uri ~ ^(.+)$) {
		set $supercache_file /wp-content/cache/supercache/$http_host/$1index.html;
	}
 
	if (-f $document_root$supercache_file) {
		rewrite ^(.*)$ $supercache_file break;
	}
 
	if (!-e $request_filename) {
		rewrite . /index.php last;
	}
 
	location ~ \.php$ {
 
		fastcgi_pass   127.0.0.1:9000;
		fastcgi_index  index.php;
		fastcgi_param  SCRIPT_FILENAME  ***$fastcgi_script_name;
		include        fastcgi_params;
	}
 
	location ~ /\.ht {
		deny  all;
	}
}下面是解释:

gzip_static on;如果浏览器支持gzip,则在压缩前先寻找是否存在压缩好的同名gz文件避免再次压缩浪费资源,配合supercache的压缩功能一起使用效果最好,相比supercache原生的Apache mod_rewrite实现,nginx的实现简单的多. Apache mod_rewrite足足用了两套看起来一模一样的条件判断来分别rewrite支持gzip压缩和不支持的情况.

if (-f $request_filename) {
	break;
}//如果是直接请求某个真实存在的文件,则用break语句停止rewrite检查

set $supercache_file '';
set $supercache_uri $request_uri;//用$request_uri初始化变量 $supercache_uri.

if ($request_method = POST) {
	set $supercache_uri '';
}//如果请求方式为POST,则不使用supercache.这里用清空$supercache_uri的方法来跳过检测,下面会看到

if ($query_string) {
	set $supercache_uri '';
}//因为使用了rewrite的原因,正常情况下不应该有query_string(一般只有后台才会出现query string),有的话则不使用supercache

if ($http_cookie ~* "comment_author_|wordpress_logged_|wp-postpass_" ) {
	set $supercache_uri '';
}//默认情况下,supercache是仅对unknown user使用的.其他诸如登录用户或者评论过的用户则不使用.

comment_author是测试评论用户的cookie, wordpress_logged是测试登录用户的cookie. wp-postpass不大清楚,字面上来看可能是曾经发表过文章的?只要cookie中含有这些字符串则条件成立.

原来的写法中检测登录用户cookie用的是wordpress_,但是我在测试中发现登入/登出以后还会有一个叫wordpress_test_cookie存在,不知道是什么作用,我也不清楚一般用户是否会产生这个cookie.由于考虑到登出以后这个cookie依然存在可能会影响到cache的判断,于是把这里改成了匹配wordpress_logged_

if ($supercache_uri ~ ^(.+)$) {
	set $supercache_file /wp-content/cache/supercache/$http_host$1index.html;
}//如果变量$supercache_uri不为空,则设置cache file的路径

这里稍微留意下$http_host$1index.html这串东西,其实写成 $http_host/$1/index.html 就好懂很多

以这个rewrite形式的url为例

cafeneko.info/2010/09/tsukihime-doujin_part01/

其中 
$http_host = ‘cafeneko.info’ , $1 = $request_uri = ‘/2010/09/tsukihime-doujin_part01/’

则 $http_host$1index.html = ‘cafeneko.info/2010/09/tsukihime-doujin_part01/index.html’

而 $http_host/$1/index.html = ‘cafeneko.info//2010/09/tsukihime-doujin_part01//index.html’

虽然在调试过程中两者并没有不同,不过为了保持正确的路径,还是省略了中间的/符号.

最后上例rewrite后的url = ‘cafeneko.info/wp-content/cache/supercache/cafeneko.info/2010/09/tsukihime-doujin_part01/index.html’

if (-f $document_root$supercache_file) {
	rewrite ^(.*)$ $supercache_file break;
}//检查cache文件是否存在,存在的话则执行rewrite,留意这里因为是rewrite到html静态文件,所以可以直接用break终止掉.

if (!-e $request_filename) {
	rewrite . /index.php last;
}//执行到此则说明不使用suercache,进行wordpress的permalink rewrite

检查请求的文件/目录是否存在,如果不存在则条件成立, rewrite到index.php

顺便说一句,当时这里这句rewrite看的我百思不得其解. .

只能匹配一个字符啊?这是什么意思?

一般情况下,想调试nginx rewrite最简单的方法就是把flag写成redirect,这样就能在浏览器地址栏里看到真实的rewrite地址.

然而对于permalink rewrite却不能用这种方法,因为一旦写成redirect以后,不管点什么链接,只要没有supercache,都是跳转回首页了.

后来看了一些文章才明白了rewrite的本质,其实是在保持请求地址不变的情况下,在服务器端将请求转到特定的页面.

乍一看supercache的性质有点像302到静态文件,所以可以用redirect调试.

但是permalink却是性质完全不同的rewrite,这跟wordpress的处理方式有关. 我研究不深就不多说了,简单说就是保持URL不变将请求rewrite到index.php,WP将分析其URL结构再对其并进行匹配(文章,页面,tag等),然后再构建页面. 所以其实这条rewrite

rewrite . /index.php last;说的是,任何请求都会被rewrite到index.php.因为”.”匹配任意字符,所以这条rewrite其实可以写成任何形式的能任意命中的正则.比如说

rewrite . /index.php last;
rewrite ^ /index.php last;
rewrite .* /index.php last;效果都是一样的,都能做到permalink rewrite.

最后要提的就是有人可能注意到我的rewrite规则是放在server块中的.网上能找到的大多数关于wordpress的nginx rewrite规则都是放在location /下面的,但是上面我却放在了server块中,为何?

原因是WP或某个插件会在当前页面做一个POST的XHR请求,本来没什么特别,但问题就出在其XHR请求的URL结构上.

正常的permalink一般为: domain.com/year/month/postname/ 或者 domain.com/tags/tagname/ 之类.

但这个XHR请求的URL却是 domain.com/year/month/postname/index.php 或者 domain.com/tags/tagname/index.php

这样一来就命中了location ~ \.php$而交给fastcgi,但因为根本没有做过rewrite其页面不可能存在,结果就是这个XHR返回一个404

鉴于location之间匹配优先级的原因,我将主要的rewrite功能全部放进了server区块中,这样就得以保证在进行location匹配之前是一定做过rewrite的.

这时有朋友又要问了,为什么命中的是location ~ \.php$而不是location / ?

…望天…长叹…这就要扯到天杀的location匹配问题了….

locatoin并非像rewrite那样逐条执行,而是有着匹配优先级的,当一条请求同时满足几个location的匹配时,其只会选择其一的配置执行.

其寻找的方法为:

1. 首先寻找所有的常量匹配,如location /, location /av/, 以相对路径自左向右匹配,匹配长度最高的会被使用, 
2. 然后按照配置文件中出现的顺序依次测试正则表达式,如 location ~ download\/$, location ~* \.wtf, 第一个匹配会被使用 
3. 如果没有匹配的正则,则使用之前的常量匹配

而下面几种方法当匹配时会立即终止其他location的尝试

1. = 完全匹配,location = /download/ 
2. ^~ 终止正则测试,如location ^~ /download/ 如果这条是最长匹配,则终止正则测试,这个符号只能匹配常量 
3. 在没有=或者^~的情况下,如果常量完全匹配,也会立即终止测试,比如请求为 /download/ 会完全命中location /download/而不继续其他的正则测试

总结:

1. 如果完全匹配(不管有没有=),尝试会立即终止
2. 以最长匹配测试各个常量,如果常量匹配并有 ^~, 尝试会终止 
3. 按在配置文件中出现的顺序测试各个正则表达式 
4. 如果第3步有命中,则使用其匹配location,否则使用第2步的location

另外还可以定义一种特殊的named location,以@开头,如location @thisissparta 不过这种location定义不用于一般的处理,而是专门用于try_file, error_page的处理,这里不再深入.

晕了没? 用前文的例子来看看

location  = / {
  ....配置A
}
 
location  / {
  ....配置B
}
 
location ^~ /images/ {
  ....配置C
}
 
location ~* \.(gif|jpg|jpeg)$ {
  ....配置D
}访问 / 会使用配置A -> 完全命中
访问 /documents/document.html 会使用配置B -> 匹配常量B,不匹配正则C和D,所以用B 
访问 /images/1.gif 会使用配置C -> 匹配常量B,匹配正则C,使用首个命中的正则,所以用C 
访问 /documents/1.jpg 会使用配置D -> 匹配常量B,不匹配正则C,匹配正则D,使用首个命中的正则,所以用D

那么再回头看我们刚才说的问题.为什么那个URL结果奇怪的XHR请求会命中location ~ \.php$而不是location / ? 我相信你应该已经知道答案了.

所以要解决这个问题最简单的方法就是把rewrite规则放在比location先执行的server块里面就可以了哟.

这次的研究笔记就到此为止了.

最后留一个思考题,如果不将rewrite规则放入server块,还有什么方法可以解决这个XHR 404的问题?

原来的location /块包含从location ~ \.php$到root为止的部分.

答案是存在的.在用使用目前的方法前我死脑筋的在保留location /的前提下尝试了很多种方法…请不要尝试为各种permalink构建独立的location.因为wp的permalink种类很多,包括单篇文章,页面,分类,tag,作者,存档等等..欢迎在回复中讨论 /

参考:
Nginx wiki

-EOF-

 

更新  @2010.10.23

之前的supercache rewrite规则适用于大部分的WP.但是并不适用于mobile press插件的移动设备支持.

因为其中并没有检测移动设备的user agent,从而导致移动设备也会被rewrite到cache上.这样的结果是在移动设备上也是看到的跟PC一样的完全版blog. 对于性能比较好的手机比如iphone安卓什么的大概没什么问题,但比较一般的比如nokia上用opera mini等看就会比较辛苦了,这次把supercache原本在htaccess中的移动设备检测的代码块也移植了过来.

在前文的配置文件中cookie检测后面加入以下代码段

	# Bypass special user agent
	if ($http_user_agent ~* "2.0 MMP|240x320|400X240|AvantGo|BlackBerry|Blazer|Cellphone|Danger|DoCoMo|Elaine/3.0|EudoraWeb|Googlebot-Mobile|hiptop|IEMobile|KYOCERA/WX310K|LG/U990|MIDP-2.|MMEF20|MOT-V|NetFront|Newt|Nintendo Wii|Nitro|Nokia|Opera Mini|Palm|PlayStation Portable|portalmmm|Proxinet|ProxiNet|SHARP-TQ-GX10|SHG-i900|Small|SonyEricsson|Symbian OS|SymbianOS|TS21i-10|UP.Browser|UP.Link|webOS|Windows CE|WinWAP|YahooSeeker/M1A1-R2D2|iPhone|iPod|Android|BlackBerry9530|LG-TU915 Obigo|LGE VX|webOS|Nokia5800") {
		set $supercache_uri '';
	}
 
	if ($http_user_agent ~* "w3c |w3c-|acs-|alav|alca|amoi|audi|avan|benq|bird|blac|blaz|brew|cell|cldc|cmd-|dang|doco|eric|hipt|htc_|inno|ipaq|ipod|jigs|kddi|keji|leno|lg-c|lg-d|lg-g|lge-|lg/u|maui|maxo|midp|mits|mmef|mobi|mot-|moto|mwbp|nec-|newt|noki|palm|pana|pant|phil|play|port|prox|qwap|sage|sams|sany|sch-|sec-|send|seri|sgh-|shar|sie-|siem|smal|smar|sony|sph-|symb|t-mo|teli|tim-|tosh|tsm-|upg1|upsi|vk-v|voda|wap-|wapa|wapi|wapp|wapr|webc|winw|winw|xda\ |xda-") {
		set $supercache_uri '';
	}这样就可以对移动设备绕开cache规则,而直接使用mobile press产生的移动版的效果了.


*/

typedef struct {
    ngx_array_t  *codes;        /* uintptr_t */

    ngx_uint_t    stack_size;

    ngx_flag_t    log;
    ngx_flag_t    uninitialized_variable_warn;
} ngx_http_rewrite_loc_conf_t;


static void *ngx_http_rewrite_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_rewrite_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_rewrite_init(ngx_conf_t *cf);
static char *ngx_http_rewrite(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_rewrite_return(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_rewrite_break(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_rewrite_if(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char * ngx_http_rewrite_if_condition(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf);
static char *ngx_http_rewrite_variable(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf, ngx_str_t *value);
static char *ngx_http_rewrite_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char * ngx_http_rewrite_value(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf, ngx_str_t *value);


static ngx_command_t  ngx_http_rewrite_commands[] = {

    { ngx_string("rewrite"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE23,
      ngx_http_rewrite,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("return"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE12,
      ngx_http_rewrite_return,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("break"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_NOARGS,
      ngx_http_rewrite_break,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("if"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_BLOCK|NGX_CONF_1MORE,
      ngx_http_rewrite_if,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("set"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE2,
      ngx_http_rewrite_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("rewrite_log"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rewrite_loc_conf_t, log),
      NULL },

    { ngx_string("uninitialized_variable_warn"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rewrite_loc_conf_t, uninitialized_variable_warn),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_rewrite_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_rewrite_init,                 /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_rewrite_create_loc_conf,      /* create location configuration */
    ngx_http_rewrite_merge_loc_conf        /* merge location configuration */
};


ngx_module_t  ngx_http_rewrite_module = {
    NGX_MODULE_V1,
    &ngx_http_rewrite_module_ctx,          /* module context */
    ngx_http_rewrite_commands,             /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_rewrite_handler(ngx_http_request_t *r)
{
    ngx_int_t                     index;
    ngx_http_script_code_pt       code;
    ngx_http_script_engine_t     *e;
    ngx_http_core_srv_conf_t     *cscf;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_rewrite_loc_conf_t  *rlcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    index = cmcf->phase_engine.location_rewrite_index;

    if (r->phase_handler == index && r->loc_conf == cscf->ctx->loc_conf) {
        /* skipping location rewrite phase for server null location */
        return NGX_DECLINED;
    }

    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_rewrite_module);

    if (rlcf->codes == NULL) {
        return NGX_DECLINED;
    }

    e = ngx_pcalloc(r->pool, sizeof(ngx_http_script_engine_t));
    if (e == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    e->sp = ngx_pcalloc(r->pool,
                        rlcf->stack_size * sizeof(ngx_http_variable_value_t));
    if (e->sp == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    e->ip = rlcf->codes->elts;
    e->request = r;
    e->quote = 1;
    e->log = rlcf->log;
    e->status = NGX_DECLINED;

    while (*(uintptr_t *) e->ip) {
        code = *(ngx_http_script_code_pt *) e->ip;
        code(e);
    }

    if (e->status < NGX_HTTP_BAD_REQUEST) {
        return e->status;
    }

    if (r->err_status == 0) {
        return e->status;
    }

    return r->err_status;
}


static ngx_int_t
ngx_http_rewrite_var(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_http_variable_t          *var;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_rewrite_loc_conf_t  *rlcf;

    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_rewrite_module);

    if (rlcf->uninitialized_variable_warn == 0) {
        *v = ngx_http_variable_null_value;
        return NGX_OK;
    }

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    var = cmcf->variables.elts;

    /*
     * the ngx_http_rewrite_module sets variables directly in r->variables,
     * and they should be handled by ngx_http_get_indexed_variable(),
     * so the handler is called only if the variable is not initialized
     */

    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                  "using uninitialized \"%V\" variable", &var[data].name);

    *v = ngx_http_variable_null_value;

    return NGX_OK;
}


static void *
ngx_http_rewrite_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_rewrite_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_rewrite_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->stack_size = NGX_CONF_UNSET_UINT;
    conf->log = NGX_CONF_UNSET;
    conf->uninitialized_variable_warn = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_rewrite_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_rewrite_loc_conf_t *prev = parent;
    ngx_http_rewrite_loc_conf_t *conf = child;

    uintptr_t  *code;

    ngx_conf_merge_value(conf->log, prev->log, 0);
    ngx_conf_merge_value(conf->uninitialized_variable_warn,
                         prev->uninitialized_variable_warn, 1);
    ngx_conf_merge_uint_value(conf->stack_size, prev->stack_size, 10);

    if (conf->codes == NULL) {
        return NGX_CONF_OK;
    }

    if (conf->codes == prev->codes) {
        return NGX_CONF_OK;
    }

    code = ngx_array_push_n(conf->codes, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_CONF_ERROR;
    }

    *code = (uintptr_t) NULL;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_rewrite_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rewrite_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rewrite_handler;

    return NGX_OK;
}


static char *
ngx_http_rewrite(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    ngx_str_t                         *value;
    ngx_uint_t                         last;
    ngx_regex_compile_t                rc;
    ngx_http_script_code_pt           *code;
    ngx_http_script_compile_t          sc;
    ngx_http_script_regex_code_t      *regex;
    ngx_http_script_regex_end_code_t  *regex_end;
    u_char                             errstr[NGX_MAX_CONF_ERRSTR];

    regex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                       sizeof(ngx_http_script_regex_code_t));
    if (regex == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(regex, sizeof(ngx_http_script_regex_code_t));

    value = cf->args->elts;

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = value[1];
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    /* TODO: NGX_REGEX_CASELESS */

    regex->regex = ngx_http_regex_compile(cf, &rc);
    if (regex->regex == NULL) {
        return NGX_CONF_ERROR;
    }

    regex->code = ngx_http_script_regex_start_code;
    regex->uri = 1;
    regex->name = value[1];

    if (value[2].data[value[2].len - 1] == '?') {

        /* the last "?" drops the original arguments */
        value[2].len--;

    } else {
        regex->add_args = 1;
    }

    last = 0;

    if (ngx_strncmp(value[2].data, "http://", sizeof("http://") - 1) == 0
        || ngx_strncmp(value[2].data, "https://", sizeof("https://") - 1) == 0
        || ngx_strncmp(value[2].data, "$scheme", sizeof("$scheme") - 1) == 0)
    {
        regex->status = NGX_HTTP_MOVED_TEMPORARILY;
        regex->redirect = 1;
        last = 1;
    }

    if (cf->args->nelts == 4) {
        if (ngx_strcmp(value[3].data, "last") == 0) {
            last = 1;

        } else if (ngx_strcmp(value[3].data, "break") == 0) {
            regex->break_cycle = 1;
            last = 1;

        } else if (ngx_strcmp(value[3].data, "redirect") == 0) {
            regex->status = NGX_HTTP_MOVED_TEMPORARILY;
            regex->redirect = 1;
            last = 1;

        } else if (ngx_strcmp(value[3].data, "permanent") == 0) {
            regex->status = NGX_HTTP_MOVED_PERMANENTLY;
            regex->redirect = 1;
            last = 1;

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }
    }

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = &value[2];
    sc.lengths = &regex->lengths;
    sc.values = &lcf->codes;
    sc.variables = ngx_http_script_variables_count(&value[2]);
    sc.main = regex;
    sc.complete_lengths = 1;
    sc.compile_args = !regex->redirect;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    regex = sc.main;

    regex->size = sc.size;
    regex->args = sc.args;

    if (sc.variables == 0 && !sc.dup_capture) {
        regex->lengths = NULL;
    }

    regex_end = ngx_http_script_add_code(lcf->codes,
                                      sizeof(ngx_http_script_regex_end_code_t),
                                      &regex);
    if (regex_end == NULL) {
        return NGX_CONF_ERROR;
    }

    regex_end->code = ngx_http_script_regex_end_code;
    regex_end->uri = regex->uri;
    regex_end->args = regex->args;
    regex_end->add_args = regex->add_args;
    regex_end->redirect = regex->redirect;

    if (last) {
        code = ngx_http_script_add_code(lcf->codes, sizeof(uintptr_t), &regex);
        if (code == NULL) {
            return NGX_CONF_ERROR;
        }

        *code = NULL;
    }

    regex->next = (u_char *) lcf->codes->elts + lcf->codes->nelts
                                              - (u_char *) regex;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_return(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    u_char                            *p;
    ngx_str_t                         *value, *v;
    ngx_http_script_return_code_t     *ret;
    ngx_http_compile_complex_value_t   ccv;

    ret = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                     sizeof(ngx_http_script_return_code_t));
    if (ret == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    ngx_memzero(ret, sizeof(ngx_http_script_return_code_t));

    ret->code = ngx_http_script_return_code;

    p = value[1].data;

    ret->status = ngx_atoi(p, value[1].len);

    if (ret->status == (uintptr_t) NGX_ERROR) {

        if (cf->args->nelts == 2
            && (ngx_strncmp(p, "http://", sizeof("http://") - 1) == 0
                || ngx_strncmp(p, "https://", sizeof("https://") - 1) == 0
                || ngx_strncmp(p, "$scheme", sizeof("$scheme") - 1) == 0))
        {
            ret->status = NGX_HTTP_MOVED_TEMPORARILY;
            v = &value[1];

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid return code \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

    } else {

        if (ret->status > 999) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid return code \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        v = &value[2];
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = v;
    ccv.complex_value = &ret->text;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_break(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t *lcf = conf;

    ngx_http_script_code_pt  *code;

    code = ngx_http_script_start_code(cf->pool, &lcf->codes, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_CONF_ERROR;
    }

    *code = ngx_http_script_break_code;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_if(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    void                         *mconf;
    char                         *rv;
    u_char                       *elts;
    ngx_uint_t                    i;
    ngx_conf_t                    save;
    ngx_http_module_t            *module;
    ngx_http_conf_ctx_t          *ctx, *pctx;
    ngx_http_core_loc_conf_t     *clcf, *pclcf;
    ngx_http_script_if_code_t    *if_code;
    ngx_http_rewrite_loc_conf_t  *nlcf;

    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    pctx = cf->ctx;
    ctx->main_conf = pctx->main_conf;
    ctx->srv_conf = pctx->srv_conf;

    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_loc_conf) {

            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                 return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[i]->ctx_index] = mconf;
        }
    }

    pclcf = pctx->loc_conf[ngx_http_core_module.ctx_index];

    clcf = ctx->loc_conf[ngx_http_core_module.ctx_index];
    clcf->loc_conf = ctx->loc_conf;
    clcf->name = pclcf->name;
    clcf->noname = 1;

    if (ngx_http_add_location(cf, &pclcf->locations, clcf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (ngx_http_rewrite_if_condition(cf, lcf) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    if_code = ngx_array_push_n(lcf->codes, sizeof(ngx_http_script_if_code_t));
    if (if_code == NULL) {
        return NGX_CONF_ERROR;
    }

    if_code->code = ngx_http_script_if_code;

    elts = lcf->codes->elts;


    /* the inner directives must be compiled to the same code array */

    nlcf = ctx->loc_conf[ngx_http_rewrite_module.ctx_index];
    nlcf->codes = lcf->codes;


    save = *cf;
    cf->ctx = ctx;

    if (pclcf->name.len == 0) {
        if_code->loc_conf = NULL;
        cf->cmd_type = NGX_HTTP_SIF_CONF;

    } else {
        if_code->loc_conf = ctx->loc_conf;
        cf->cmd_type = NGX_HTTP_LIF_CONF;
    }

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    if (rv != NGX_CONF_OK) {
        return rv;
    }


    if (elts != lcf->codes->elts) {
        if_code = (ngx_http_script_if_code_t *)
                   ((u_char *) if_code + ((u_char *) lcf->codes->elts - elts));
    }

    if_code->next = (u_char *) lcf->codes->elts + lcf->codes->nelts
                                                - (u_char *) if_code;

    /* the code array belong to parent block */

    nlcf->codes = NULL;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_if_condition(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf)
{
    u_char                        *p;
    size_t                         len;
    ngx_str_t                     *value;
    ngx_uint_t                     cur, last;
    ngx_regex_compile_t            rc;
    ngx_http_script_code_pt       *code;
    ngx_http_script_file_code_t   *fop;
    ngx_http_script_regex_code_t  *regex;
    u_char                         errstr[NGX_MAX_CONF_ERRSTR];

    value = cf->args->elts;
    last = cf->args->nelts - 1;

    if (value[1].len < 1 || value[1].data[0] != '(') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    if (value[1].len == 1) {
        cur = 2;

    } else {
        cur = 1;
        value[1].len--;
        value[1].data++;
    }

    if (value[last].len < 1 || value[last].data[value[last].len - 1] != ')') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[last]);
        return NGX_CONF_ERROR;
    }

    if (value[last].len == 1) {
        last--;

    } else {
        value[last].len--;
        value[last].data[value[last].len] = '\0';
    }

    len = value[cur].len;
    p = value[cur].data;

    if (len > 1 && p[0] == '$') {

        if (cur != last && cur + 2 != last) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid condition \"%V\"", &value[cur]);
            return NGX_CONF_ERROR;
        }

        if (ngx_http_rewrite_variable(cf, lcf, &value[cur]) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        if (cur == last) {
            return NGX_CONF_OK;
        }

        cur++;

        len = value[cur].len;
        p = value[cur].data;

        if (len == 1 && p[0] == '=') {

            if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
                return NGX_CONF_ERROR;
            }

            code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                              sizeof(uintptr_t));
            if (code == NULL) {
                return NGX_CONF_ERROR;
            }

            *code = ngx_http_script_equal_code;

            return NGX_CONF_OK;
        }

        if (len == 2 && p[0] == '!' && p[1] == '=') {

            if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
                return NGX_CONF_ERROR;
            }

            code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                              sizeof(uintptr_t));
            if (code == NULL) {
                return NGX_CONF_ERROR;
            }

            *code = ngx_http_script_not_equal_code;
            return NGX_CONF_OK;
        }

        if ((len == 1 && p[0] == '~')
            || (len == 2 && p[0] == '~' && p[1] == '*')
            || (len == 2 && p[0] == '!' && p[1] == '~')
            || (len == 3 && p[0] == '!' && p[1] == '~' && p[2] == '*'))
        {
            regex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                         sizeof(ngx_http_script_regex_code_t));
            if (regex == NULL) {
                return NGX_CONF_ERROR;
            }

            ngx_memzero(regex, sizeof(ngx_http_script_regex_code_t));

            ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

            rc.pattern = value[last];
            rc.options = (p[len - 1] == '*') ? NGX_REGEX_CASELESS : 0;
            rc.err.len = NGX_MAX_CONF_ERRSTR;
            rc.err.data = errstr;

            regex->regex = ngx_http_regex_compile(cf, &rc);
            if (regex->regex == NULL) {
                return NGX_CONF_ERROR;
            }

            regex->code = ngx_http_script_regex_start_code;
            regex->next = sizeof(ngx_http_script_regex_code_t);
            regex->test = 1;
            if (p[0] == '!') {
                regex->negative_test = 1;
            }
            regex->name = value[last];

            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "unexpected \"%V\" in condition", &value[cur]);
        return NGX_CONF_ERROR;

    } else if ((len == 2 && p[0] == '-')
               || (len == 3 && p[0] == '!' && p[1] == '-'))
    {
        if (cur + 1 != last) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid condition \"%V\"", &value[cur]);
            return NGX_CONF_ERROR;
        }

        value[last].data[value[last].len] = '\0';
        value[last].len++;

        if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        fop = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                          sizeof(ngx_http_script_file_code_t));
        if (fop == NULL) {
            return NGX_CONF_ERROR;
        }

        fop->code = ngx_http_script_file_code;

        if (p[1] == 'f') {
            fop->op = ngx_http_script_file_plain;
            return NGX_CONF_OK;
        }

        if (p[1] == 'd') {
            fop->op = ngx_http_script_file_dir;
            return NGX_CONF_OK;
        }

        if (p[1] == 'e') {
            fop->op = ngx_http_script_file_exists;
            return NGX_CONF_OK;
        }

        if (p[1] == 'x') {
            fop->op = ngx_http_script_file_exec;
            return NGX_CONF_OK;
        }

        if (p[0] == '!') {
            if (p[2] == 'f') {
                fop->op = ngx_http_script_file_not_plain;
                return NGX_CONF_OK;
            }

            if (p[2] == 'd') {
                fop->op = ngx_http_script_file_not_dir;
                return NGX_CONF_OK;
            }

            if (p[2] == 'e') {
                fop->op = ngx_http_script_file_not_exists;
                return NGX_CONF_OK;
            }

            if (p[2] == 'x') {
                fop->op = ngx_http_script_file_not_exec;
                return NGX_CONF_OK;
            }
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[cur]);
        return NGX_CONF_ERROR;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid condition \"%V\"", &value[cur]);

    return NGX_CONF_ERROR;
}


static char *
ngx_http_rewrite_variable(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf,
    ngx_str_t *value)
{
    ngx_int_t                    index;
    ngx_http_script_var_code_t  *var_code;

    value->len--;
    value->data++;

    index = ngx_http_get_variable_index(cf, value);

    if (index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    var_code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                          sizeof(ngx_http_script_var_code_t));
    if (var_code == NULL) {
        return NGX_CONF_ERROR;
    }

    var_code->code = ngx_http_script_var_code;
    var_code->index = index;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    ngx_int_t                            index;
    ngx_str_t                           *value;
    ngx_http_variable_t                 *v;
    ngx_http_script_var_code_t          *vcode;
    ngx_http_script_var_handler_code_t  *vhcode;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }

    index = ngx_http_get_variable_index(cf, &value[1]);
    if (index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (v->get_handler == NULL
        && ngx_strncasecmp(value[1].data, (u_char *) "http_", 5) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "sent_http_", 10) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "upstream_http_", 14) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "cookie_", 7) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "upstream_cookie_", 16)
           != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "arg_", 4) != 0)
    {
        v->get_handler = ngx_http_rewrite_var;
        v->data = index;
    }

    if (ngx_http_rewrite_value(cf, lcf, &value[2]) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    if (v->set_handler) {
        vhcode = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                   sizeof(ngx_http_script_var_handler_code_t));
        if (vhcode == NULL) {
            return NGX_CONF_ERROR;
        }

        vhcode->code = ngx_http_script_var_set_handler_code;
        vhcode->handler = v->set_handler;
        vhcode->data = v->data;

        return NGX_CONF_OK;
    }

    vcode = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                       sizeof(ngx_http_script_var_code_t));
    if (vcode == NULL) {
        return NGX_CONF_ERROR;
    }

    vcode->code = ngx_http_script_set_var_code;
    vcode->index = (uintptr_t) index;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_value(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf,
    ngx_str_t *value)
{
    ngx_int_t                              n;
    ngx_http_script_compile_t              sc;
    ngx_http_script_value_code_t          *val;
    ngx_http_script_complex_value_code_t  *complex;

    n = ngx_http_script_variables_count(value);

    if (n == 0) {
        val = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                         sizeof(ngx_http_script_value_code_t));
        if (val == NULL) {
            return NGX_CONF_ERROR;
        }

        n = ngx_atoi(value->data, value->len);

        if (n == NGX_ERROR) {
            n = 0;
        }

        val->code = ngx_http_script_value_code;
        val->value = (uintptr_t) n;
        val->text_len = (uintptr_t) value->len;
        val->text_data = (uintptr_t) value->data;

        return NGX_CONF_OK;
    }

    complex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                 sizeof(ngx_http_script_complex_value_code_t));
    if (complex == NULL) {
        return NGX_CONF_ERROR;
    }

    complex->code = ngx_http_script_complex_value_code;
    complex->lengths = NULL;

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = value;
    sc.lengths = &complex->lengths;
    sc.values = &lcf->codes;
    sc.variables = n;
    sc.complete_lengths = 1;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
