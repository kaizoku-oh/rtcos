## Running the linux example app on PC

```bash
$ gcc -Wall examples/linux/app.c src/rtcos.c -Ios -Iinclude -Iexamples/linux -o examples/linux/app
$ examples/linux/app
Task one received PING event!
Task two received PONG event!
Task two received a message: Hello
Task one received PING event!
Task two received PONG event!
^C
$ 
```

## Note about running the Linux example:

Since we don't have direct access to a timer tick handler under linux
this app will only test sending immediate events and messages rather than future events.
