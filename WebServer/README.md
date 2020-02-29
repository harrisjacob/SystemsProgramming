# Project - README

This is the [Final Project] for [CSE 20289 Systems Programming (Spring 2019)].

## Members

- Ben Merrick (bmerrick@nd.edu)
- Jake Harris (jharri18@nd.edu)
- Noah Viner  (nviner@nd.edu)

## Demonstration

- [Link to Google Slides]()
https://docs.google.com/presentation/d/1iMlhRGIV3h6-vPe-ZEw3VqgDqKxJncdW2YYpWGyKLB0/edit?usp=sharing

## Errata

Summary of things that don't work (quite right).

Overall, it appears that our project works as required. We were able to pass all tests including
the Valgrind tests. However, if we had more time, our program encounters an error when the port 
was just previously bound to. This error was discussed in class and was inevitable to avoid. Yet, 
depending upon the mode, our code runs into a segfault which is not supposed to happen. This is a 
result of some error checking (we think), but we would try to fix that if we had more time. For now,
we are assuming that we will not run into this issue as we bind to new ports each time. Everything
else appears to work great!

## Contributions

Enumeration of the contributions of each group member.

Ben Merrick:
-thor.py
-spidey.c
-single.c
-requests.c
-handler.c
-Resolving Memory leaks


Jake Harris:
-requests.c
-forking.c
-utils.c
-Resolving memory leaks

Noah Viner:
-Test scripts /test.sh
-socket.c
-Utils.c





[Final Project]: https://www3.nd.edu/~pbui/teaching/cse.20289.sp19/project.html
[CSE 20289 Systems Programming (Spring 2019)]: https://www3.nd.edu/~pbui/teaching/cse.20289.sp19/
