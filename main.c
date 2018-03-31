#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_FRAMES 1024

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct
{
    int x, y;
    int w, h;
} Rect;

typedef struct
{
    unsigned char r, g, b, a;
} Pixel;

static bool PixelEqual(const Pixel* a, const Pixel* b)
{
    return a->r == b->r &&
           a->g == b->g &&
           a->b == b->b &&
           a->a == b->a;
}

int main(int argc, char** argv)
{
    if(argc < 5) {
        fprintf(stderr, "Usage: %s path/to/image path/to/output_image frame_width frame_height\n", argv[0]);
        return 1;
    }

    int fw = atoi(argv[3]);
    int fh = atoi(argv[4]);

    if(fw <= 0 || fh <= 0) {
        fprintf(stderr, "Invalid frame width or frame height.\n");
        return 1;
    }

    int w, h, n;
    unsigned char* src = stbi_load(argv[1], &w, &h, &n, 4);

    if(!src) {
        fprintf(stderr, "Failed to load image '%s': %s\n", argv[1], stbi_failure_reason());
        return 1;
    }
    
    // Top left pixel is bg color
    const Pixel* bg = (Pixel*)src;

    printf("image size: %d, %d\n", w, h);
    printf("bg: %d %d %d %d\n", bg->r, bg->g, bg->b, bg->a);

    int numFrames = 0;
    static Rect frames[MAX_FRAMES];

    for(int i = 0; i < MAX_FRAMES; ++i) {
        frames[i].x = -1;
        frames[i].y = -1;
        frames[i].w = -1;
        frames[i].h = -1;
    }

    for(int y = 0; y < h; ++y) {
        int min = -1;
        int max = -1;

        int curFrame = 0;

        for(int x = 0; x < w; ++x) {
            const Pixel* p = (Pixel*)(&src[x * 4 + y * (w * 4)]);
  
            if(PixelEqual(p, bg)) {
                continue;
            }

            if(min < 0) {
                min = x;
            } else {
                if(x - min < fw) {
                    max = x;
                } else {
                    // We found a pixel outside of the frame boundaries, so build the frame
                    Rect* frame = &frames[curFrame];

                    if(frame->x < 0 || min < frame->x) {
                        frame->x = min;
                    }

                    if(frame->w < 0 || ((max - frame->x + 1) <= fw && (max - frame->x + 1 > frame->w))) {
                        frame->w = max - frame->x + 1;
                    }

                    if(frame->y < 0) {
                        frame->y = y;
                    }

                    if(y - frame->y + 1 <= fh) {
                        frame->h = y - frame->y + 1;
                    }

                    curFrame += 1;
                    
                    if(curFrame > numFrames) {
                        numFrames = curFrame;
                    }

                    min = x;
                    max = -1;
                }
            }
        }
    }

    stbi_image_free(src);
    
    printf("num frames: %d\n", numFrames);

    for(int i = 0; i < numFrames; ++i) {
        printf("%d %d %d %d\n", frames[i].x, frames[i].y, frames[i].w, frames[i].h);
    }
    
    return 0;
}
