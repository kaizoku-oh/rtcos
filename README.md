# RTCOS (Run To Completion Operating System)

<p align="center">
  <img src="https://github.com/kaizoku-oh/rtcos/blob/main/docs/image/logo.png">
</p>

<!-- ![RTCOS logo](https://github.com/kaizoku-oh/rtcos/blob/main/docs/image/logo.png) -->
<!-- ![](https://github.com/<OWNER>/<REPOSITORY>/workflows/<WORKFLOW_NAME>/badge.svg) -->
[![GitHub Build workflow status](https://github.com/kaizoku-oh/rtcos/workflows/Build/badge.svg)](https://github.com/kaizoku-oh/rtcos/actions/workflows/main.yaml)
[![GitHub release](https://img.shields.io/github/v/release/kaizoku-oh/rtcos)](https://github.com/kaizoku-oh/rtcos/releases)
![dependencies Status](https://status.david-dm.org/gh/dwyl/esta.svg)
[![GitHub issues](https://img.shields.io/github/issues/kaizoku-oh/rtcos)](https://github.com/kaizoku-oh/rtcos/issues)
![GitHub top language](https://img.shields.io/github/languages/top/kaizoku-oh/rtcos)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/kaizoku-oh/rtcos/blob/main/LICENSE)
[![Twitter follow](https://img.shields.io/twitter/follow/kaizoku_ouh?style=social)](https://twitter.com/kaizoku_ouh)

is an event-driven RTC (Run To Completion) scheduler dedicated to ultra constrained IoT devices.

RTCOS is written in pure C and doesn't need any thirdparty library or dependency to work.

Being hardware agnostic, RTCOS can work on any hardware platform that can provide a hardware timer tick and optionally an interface to enter and exit critical section.

## TODO ✅

- [x] Add timer callback void * argument

- [x] Change the argument type passed to the task handler from _u32 to void *

- [ ] Add broadcast event function

- [ ] Add broadcast message function

- [ ] Add more comments explaining code

- [ ] Add wiki documentation

- [ ] Add Arduino wrapper

## Acknowledgments
- This project is inspired from the work done by [Jon Norenberg](https://github.com/norenberg99) in [rtcOS](https://github.com/norenberg99/rtcOS) project.
