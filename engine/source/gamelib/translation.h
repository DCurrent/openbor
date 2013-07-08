#ifndef TRANSLATION_H
#define TRANSLATION_H
char* ob_gettrans(char* id);
void ob_inittrans();
void ob_termtrans();
void ob_addtrans(char* id, char* str);
#endif

