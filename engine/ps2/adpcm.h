#ifndef ADPCM_H
#define ADPCM_H

void adpcm_reset(void);
void adpcm_coder(short *, char *, int);
void adpcm_decoder(char *, short *, int);

#endif
