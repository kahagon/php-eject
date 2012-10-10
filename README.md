php-eject
=====================

使い方
---------------------

    $ php -a
    Interactive shell
    
    php > eject_toggle_tray("cdrom");
    php > eject_close_tray("cdrom");


能書き
---------------------
[Ejectコマンドユーザー会](http://eject.kokuda.org/)を見つけて徹夜明けのどうしようもないテンションで作りました。  
Ejectコマンドユーザー会の [GitHub リポジトリ](https://github.com/Akkiesoft/Eject-Command-Users-Group/) を眺めていると eject は以下のように行われていました。  
[Eject-Command-Users-Group/minimum-sample.php](https://github.com/Akkiesoft/Eject-Command-Users-Group/blob/14fd8b020812182a3156dcade6dcdb84b872ddab/minimum-sample.php#L4)

    exec('/usr/sbin/eject -T /dev/sr0');

実装方法としてはとってリ早い exec のコール。しかし、 PHP の exec の実装は popen(3) を用いています。  
[php-src/TSRM/tsrm_virtual_cwd.h](https://github.com/php/php-src/blob/8775a37559caa67b2b8d5ede02cde2bac2f974e0/TSRM/tsrm_virtual_cwd.h#L311)  
このため、内部的に eject を実装するのに比べ、いくらかオーバーヘッドが生じます。  
man popen(3) によると、

> FILE *popen(const char *command, const char *type);  
> （中略）  
> popen() 関数は、プロセスをオープンする。具体的には、パイプを生成し、フォークを行い、 シェルを起動する。

とあります。つまり、「パイプを生成し、フォークを行い、 シェルを起動」というあたりがオーバーヘッドになり、
 **大規模な eject サービス** ではこれがボトルネックになりうると考えられます。
そこで eject を PHP に組み込もうと考えるに至りました。
php-eject の実装はシンプルで、ほとんどが eject(1) の実装をパクリ、元い、コピペしたものです。

Unix-likeなOSでのビルド・インストール
---------------------

    $ ./phpize
    $ ./configure [--enable--eject] 
    $ make
    $ make test
    $ [sudo] make install


Windows
---------------------
現在対応していません。
時間見つけて対応できたらいいなとは思っています。
