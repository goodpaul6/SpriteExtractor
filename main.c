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

typedef struct
{
    const char* inputImage;
    const char* outputImage;
    int fw, fh;
    int maxDistFromEdge;
    bool pot;
    int dw;
} Args;

static int CompareFramesRowThresh;

static void PrintUsage(const char* app)
{
    fprintf(stderr, "Usage: %s path/to/input/image path/to/output/image OPTIONS\n", app);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t--frame-width DESIRED_FRAME_WIDTH\n\t\tDesired width of the frames\n");
    fprintf(stderr, "\t--frame-height DESIRED_FRAME_HEIGHT\n\t\tDesired height of the frames\n");
    fprintf(stderr, "\t-e EDGE_DISTANCE_THRESHOLD\n\t\tThe edge distance threshold is used to determine whether disconnected pixels still belong to a frame.\n\t\tIf the distance from these pixels to the nearest edge is less than or equal to the\n\t\tthreshold, then they're incorporated.\n");
    fprintf(stderr, "\t--pot\n\t\tThis is optional. It makes the app generate a power of two image.\n");
    fprintf(stderr, "\t--dest-width DESIRED_OUTPUT_IMAGE_WIDTH\n\t\tThis is optional and mutually exclusive with the --pot option.\n\t\tThis is the width you want the output image to be.\n\t\tThe height will be determined from the number of frames detected.\n");
	fprintf(stderr, "\t--row-thresh DESIRED_ROW_THRESHOLD\n\t\tThis is equal to half the frame height by default.\n\t\tIt is used to order the resulting frames. If two frames are within the threshold on the y axis\n\t\tthen they are ordered from left-to-right next to each other in the final image.\n");
}

static bool ParseArgs(Args* args, int argc, char** argv)
{
    if(argc < 2) {
        fprintf(stderr, "Not enough arguments.\n");
        PrintUsage(argv[0]);
        return false;
    }
    
    args->inputImage = NULL;
    args->outputImage = NULL;
    args->fw = 0;
    args->fh = 0;
    args->maxDistFromEdge = 0;
    args->pot = false;
    args->dw = 0;

    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "-h") == 0) {
            PrintUsage(argv[0]);
            return false;
        } else if(strcmp(argv[i], "--frame-width") == 0) {
            args->fw = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "--frame-height") == 0) {
            args->fh = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "--dest-width") == 0) {
            args->dw = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "-e") == 0) {
            args->maxDistFromEdge = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "--pot") == 0) {
            args->pot = true;
		} else if (strcmp(argv[i], "--row-thresh") == 0) {
			CompareFramesRowThresh = atoi(argv[i + 1]);
			i += 1;
		} else {
    		if(!args->inputImage) args->inputImage = argv[i];
    		else if (!args->outputImage) args->outputImage = argv[i];
    		else {
    			fprintf(stderr, "Not sure what '%s' is specified for.\n", argv[i]);
    			return false;
    		}
    	}
    }

    if (!args->inputImage) {
    	fprintf(stderr, "Please specify an input image.\n");
    	return false;
    }

    if(!args->outputImage) {
        fprintf(stderr, "Please specify an output image.\n");
        return false;
    }

    if(args->fw == 0) {
        fprintf(stderr, "Specify a valid frame width.\n");
        return false;
    }

    if(args->fh == 0) {
        fprintf(stderr, "Specify a valid frame height.");
        return false;
    }

    if(args->maxDistFromEdge < 0) {
        fprintf(stderr, "Please specify a non-negative edge distance.");
        return false;
    }

    if(args->pot && args->dw > 0) {
        fprintf(stderr, "Cannot specify both power of two and destination width.\n");
        return false;
    }
    
    if(!args->pot && args->dw <= 0) {
        fprintf(stderr, "Please specify either --pot or --dest-width.\n");
        return false;
    }

    if (args->dw % args->fw != 0) {
    	fprintf(stderr, "Dest width must be a multiple of frame width.\n");
    	return false;
    }

	if (CompareFramesRowThresh == 0) {
		CompareFramesRowThresh = args->fh / 2;
	}

    return true;
}

static bool PixelEqual(const Pixel* a, const Pixel* b)
{
    return a->r == b->r &&
           a->g == b->g &&
           a->b == b->b &&
           a->a == b->a;
}

static int CompareFrames(const void* va, const void* vb)
{
    const Rect* a = va;
    const Rect* b = vb;

    if(abs(a->y - b->y) < CompareFramesRowThresh) {
        // They're in the same row, roughly
        return a->x - b->x;
    } 

    return a->y - b->y;
}

int main(int argc, char** argv)
{
    Args args;

    if(!ParseArgs(&args, argc, argv)) {
        return 1;
    }

    int fw = args.fw;
    int fh = args.fh;
    int maxDistFromEdge = args.maxDistFromEdge;

    int w, h, n;
    unsigned char* src = stbi_load(args.inputImage, &w, &h, &n, 4);

    if(!src) {
        fprintf(stderr, "Failed to load image '%s': %s\n", args.inputImage, stbi_failure_reason());
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
 
    qsort(frames, numFrames, sizeof(Rect), CompareFrames);

    int dw;
    int dh;

    if(args.pot) {
        int reqArea = numFrames * fw * fh;

        dw = fw;
        dh = fh;
     
        while(dw * dh < reqArea) {
            dw *= 2;
            dh *= 2;
        }
    } else {
        dw = args.dw;
        dh = ((numFrames * fw) / dw + 1) * fh;
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
