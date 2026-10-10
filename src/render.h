/* render.h — интерфейс экранов. */
#ifndef RENDER_H
#define RENDER_H

void render_init(void);
void render_frame(float dt);
void render_center_on(float wx, float wy);
int  render_screen_id(void);   /* 0 меню, 1 карта, 2 город, 3 отряд */
void render_set_screen(int s);

#endif
