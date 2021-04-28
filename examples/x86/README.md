## Running the x86 example app on PC

```bash
$ gcc -Wall examples/x86/main.c src/rtcos.c -Iinclude -Iexamples/x86 -o examples/x86/main
$ examples/x86/main
Task one argument is: TaskOne
Task one received PING event!
Task one argument is: TaskOne
Task one received boadcasted event: EVENT_COMMON
Task two argument is: TaskTwo
Task two received PONG event!
Task two received a message: Hello
Task one argument is: TaskOne
Task one received PING event!
Task two argument is: TaskTwo
^C
$ 
```

### Note:

Since we don't have direct access to a timer tick handler under x86
this app will only test sending immediate events and messages rather than future events.
