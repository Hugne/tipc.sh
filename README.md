#Enable the module
```[root@kdev ~]# enable -f tipc_subscribe.so tipc_subscribe```
#Subscribe to something (network node events in this case)
```[root@kdev ~]# tipc_subscribe 0:0:-1 netevent```
#Generated events are written directly to the shell stdout by default
#can be changed with the -u flag
```
+ {0:774974976:774974976} <774974976:0> [netevent]
+ {0:4014942704:4014942704} <4014942704:0> [netevent]
[root@kdev ~]#
```