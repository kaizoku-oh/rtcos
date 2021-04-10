# RTC OS

![GitHub release](https://img.shields.io/github/v/release/kaizoku-oh/rtcos)
![GitHub issues](https://img.shields.io/github/issues/kaizoku-oh/rtcos)
![GitHub top language](https://img.shields.io/github/languages/top/kaizoku-oh/rtcos)
![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)

is an event-driven RTC (Run To Completion) scheduler dedicated to ultra constrained devices.

RTC OS is written in pure C and doesn't need any thirdparty library or dependency to work.

Being hardware agnostic, RTC OS can work on any hardware platform that can provide a hardware timer tick and optionally an interface to enter and exit critical section.

### Running the example app on PC
```bash
$ gcc -Wall examples/app.c os/rtcos.c -Ios -Iports -Iconfig -o examples/app
$ examples/app
Task one received PING event!
Task two received PONG event!
Task two received a message: Hello
Task one received PING event!
Task two received PONG event!
^C
$ 
```

![Twitter follow](https://img.shields.io/twitter/follow/kaizoku_ouh?style=social)