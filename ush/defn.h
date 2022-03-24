#pragma once
int expand (char *orig, char *new, int newsize);
int check_bi(char** LINE, int argsc, int outfd);
void strmode_bi(mode_t mode, char *p);
int processline (char *line, int infd, int outfd, int flags);
