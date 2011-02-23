#include "ps2port.h"
#include "types.h"
#include "gsvga.h"

int video_set_mode(int width, int height){

  // success yay yay
  return 1;

}

// we are going to assume the screen is 320x240
int video_copy_screen(s_screen * src){

  gsvga_draw(src->data);

  return 1;
}

// unimplemented for now
void video_clearscreen(){
    gsvga_clear();
}


