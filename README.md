# RTCOS (Run To Completion Operating System)

<p align="center">
  <img src="https://github.com/kaizoku-oh/rtcos/blob/main/docs/image/logo.png">
</p>

<!-- ![RTCOS logo](https://github.com/kaizoku-oh/rtcos/blob/main/docs/image/logo.png) -->
<!-- ![](https://github.com/<OWNER>/<REPOSITORY>/workflows/<WORKFLOW_NAME>/badge.svg) -->
![GitHub Build workflow status](https://github.com/kaizoku-oh/rtcos/workflows/Build/badge.svg)
![GitHub release](https://img.shields.io/github/v/release/kaizoku-oh/rtcos)
[![dependencies Status](https://status.david-dm.org/gh/dwyl/esta.svg)](https://david-dm.org/dwyl/esta)
![GitHub issues](https://img.shields.io/github/issues/kaizoku-oh/rtcos)
![GitHub top language](https://img.shields.io/github/languages/top/kaizoku-oh/rtcos)
![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Twitter follow](https://img.shields.io/twitter/follow/kaizoku_ouh?style=social)

is an event-driven RTC (Run To Completion) scheduler dedicated to ultra constrained IoT devices.

RTCOS is written in pure C and doesn't need any thirdparty library or dependency to work.

Being hardware agnostic, RTCOS can work on any hardware platform that can provide a hardware timer tick and optionally an interface to enter and exit critical section.

## Acknowledgments
- This project is inspired from the work done by [Jon Norenberg](https://github.com/norenberg99) in [rtcOS](https://github.com/norenberg99/rtcOS) project.
