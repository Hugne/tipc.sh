## Enable the module
Ensure that your LD_LIBRARY_PATH points to where tipc_subscribe.so lives, or
use a full path to the .so file in the enable command.

```
[root@kdev ~]# enable -f tipc_subscribe.so tipc_subscribe
FD 3 connected to TIPC topology server
```
The subscribe module will spawn a thread in your current shell, establish a connection to
the TIPC topology server and start listening for events.
Generated events are written directly to the shell stdout by default, this can be changed by
passing another FD with the -u option.

## Examples
### Monitor network events
This will give you a notification whenever a node joins/leaves a TIPC network plane.
```
[root@kdev ~]# tipc_subscribe 0:0:-1 netevent
+ {0:774974976:774974976} <774974976:0> [netevent]
+ {0:4014942704:4014942704} <4014942704:0> [netevent]
[root@kdev ~]#
```

### Monitor service availability
```
[root@kdev ~]# tipc_subscribe 1000:123 foosvc
[root@kdev ~]# exec 10<>/dev/tipc/1000/123
+ {1000:123:123} <0:3057385781> [foosvc]
[root@kdev ~]#
```
Service availability is the default subscription method.
You can specify service ranges aswell, for example
```
tipc_subscribe 1000:123:456
```

### Monitor port availability
```
[root@kdev ~]# tipc_subscribe 1000:123 foosvc
[root@kdev ~]# exec 10<>/dev/tipc/1000/123
+ {1000:123:123} <0:1972276250> [foosvc]
[root@kdev ~]# exec 11<>/dev/tipc/1000/123
+ {1000:123:123} <0:3592640485> [foosvc]
[root@kdev ~]# exec 10<&-
- {1000:123:123} <0:1972276250> [foosvc]
[root@kdev ~]#
```

### Cancelling a subscription
Adding the -c flag will cancel a subscription. It is important that
the full subscription string is passed in the cancel request, including any
optional handle or timeout setting.
```[root@kdev ~]# tipc_subscribe -c port 1000:123 foosvc```
