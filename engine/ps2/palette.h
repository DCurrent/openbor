#ifndef PALETTE_H
#define PALETTE_H


// Set gamma/brightness corrected palette.
// Valid values range between -255 and 255, where 0 is normal.
void palette_set_corrected(char *pal, int gr, int gg, int gb, int br, int bg, int bb);

// Find colour in palette
int palette_find(char *pal, int r, int g, int b);

// Create lookup tables
char * palette_table_multiply(char *pal);
char * palette_table_screen(char *pal);
char * palette_table_dodge(char *pal);
char * palette_table_half(char *pal);
char * palette_table_overlay(char *pal);
char * palette_table_hardlight(char *pal);


#endif


