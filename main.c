#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_FRAMES 1024
#define MAX_POINTS 100000

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct
{
    int x, y;
    int w, h;
} Rect;

typedef struct
{
    unsigned char r, g, b, a;
} Pixel;

typedef struct
{
    int x, y;
	int distFromEdge;
} Point;

static bool PixelEqual(const Pixel* a, const Pixel* b)
{
    return a->r == b->r &&
           a->g == b->g &&
           a->b == b->b &&
           a->a == b->a;
}

int main(int argc, char** argv)
{
    if(argc < 6) {
        fprintf(stderr, "Usage: %s path/to/image path/to/output_image frame_width frame_height max_dist_from_edge\n", argv[0]);
        return 1;
    }

    int fw = atoi(argv[3]);
    int fh = atoi(argv[4]);
	int maxDistFromEdge = atoi(argv[5]);

    if(fw <= 0 || fh <= 0) {
        fprintf(stderr, "Invalid frame width or frame height.\n");
        return 1;
    }

	if (maxDistFromEdge < 0) {
		fprintf(stderr, "Invalid max dist from edge. It must be non-negative.\n");
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

	bool* checked = calloc(sizeof(bool), w * h);

    for(int y = 0; y < h; ++y) {
        for(int x = 0; x < w; ++x) {
            const Pixel* p = (Pixel*)(&src[x * 4 + y * (w * 4)]);
  
            if(PixelEqual(p, bg)) {
                continue;
            } 

			if (checked[x + y * w]) continue;
			
            // Flood fill to find extents
            int minX = INT_MAX;
            int minY = INT_MAX;
            int maxX = INT_MIN;
            int maxY = INT_MIN;

            int numPoints = 0;
            static Point points[MAX_POINTS]; 

            points[numPoints++] = (Point){ x, y, 0 };
            
            while(numPoints > 0) {
                Point pt = points[numPoints - 1];
                numPoints -= 1;

				if (pt.x < 0 || pt.y < 0 || pt.x >= w || pt.y >= h) continue;
				if (checked[pt.x + pt.y * w]) continue;

                p = (Pixel*)(&src[(pt.x * 4) + pt.y * (w * 4)]);

				bool isBg = PixelEqual(p, bg);
                if(isBg && pt.distFromEdge >= maxDistFromEdge) {
                    continue;
                }

				int newDistFromEdge = 0;
				if (isBg) {
					newDistFromEdge = pt.distFromEdge + 1;
				}

				checked[pt.x + pt.y * w] = true;

                if(pt.x < minX) {
                    minX = pt.x;
                }

                if(pt.y < minY) {
                    minY = pt.y;
                }
                
                if(pt.x > maxX) {
                    maxX = pt.x;
                }
                
                if(pt.y > maxY) {
                    maxY = pt.y;
                }

                assert(numPoints < MAX_POINTS);

                points[numPoints++] = (Point){pt.x - 1, pt.y, newDistFromEdge};
                points[numPoints++] = (Point){pt.x, pt.y - 1, newDistFromEdge};
                points[numPoints++] = (Point){pt.x + 1, pt.y, newDistFromEdge};
                points[numPoints++] = (Point){pt.x, pt.y + 1, newDistFromEdge};
            }

            Rect r = { minX, minY, maxX - minX + 1, maxY - minY + 1 };

			if (r.w < fw / 4 && r.h < fh / 4) {
				fprintf(stderr, "Found rect (%d,%d,%d,%d) but it's too small so I'm skipping it.\n", r.x, r.y, r.w, r.h);
			} else if(r.w > fw) {
                fprintf(stderr, "Found rect (%d,%d,%d,%d) but it's too large to fit in a single frame so I'm skipping it.\n", r.x, r.y, r.w, r.h);
            } else {
                assert(numFrames < MAX_FRAMES);
                frames[numFrames++] = r;
            }
        }
    }

	free(checked);

    int reqArea = numFrames * fw * fh;
    
    int dw = fw;
    int dh = fh;
 
    while(dw * dh < reqArea) {
        dw *= 2;
        dh *= 2;
    }
    
    int columns = dw / fw;

    unsigned char* dest = calloc(4, dw * dh);

    for(int i = 0; i < numFrames; ++i) {
        Rect r = frames[i];

        int dx = (i % columns) * fw + fw / 2 - r.w / 2;
        int dy = (i / columns) * fh + fh / 2 - r.h / 2;

        for(int y = 0; y < r.h; ++y) {
            for(int x = 0; x < r.w; ++x) {
                Pixel sp = *(Pixel*)(&src[((x + r.x) * 4) + (y + r.y) * (w * 4)]);

                if(PixelEqual(&sp, bg)) continue;
                
                *(Pixel*)(&dest[(dx + x) * 4 + (y + dy) * (dw * 4)]) = sp; 
            }
        }
    }

    if(stbi_write_png(argv[2], dw, dh, 4, dest, dw * 4) == 0) {
        fprintf(stderr, "Failed to write file.\n");
    } else {
        printf("Succeeded.\n");
    }

    return 0;
}
