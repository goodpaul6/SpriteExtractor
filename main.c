#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_FRAMES 4096
#define MAX_POINTS 100000

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define TINYFILES_IMPLEMENTATION
#include "tinyfiles.h"

typedef struct
{
    unsigned char r, g, b, a;
} Pixel;

typedef struct
{
    unsigned char* src;
	int sw, sh;
	Pixel bg;

    int x, y;
    int w, h;
} Rect;

typedef struct
{
    int x, y;
    int distFromEdge;
} Point;

typedef struct
{
    bool isDir;
    const char* inputImage;
    const char* outputImage;
    int fw, fh;
    int maxDistFromEdge;
	int minW, minH;
    bool pot;
    int dw;
    int packW, packH;
	bool label;
    bool metadata;
} Args;

static Pixel NumFont[10][3 * 5];

static void GenerateNumFont(Pixel col)
{
	static const unsigned char bits[10][3 * 5] =
	{
		{
			1, 1, 1,
			1, 0, 1,
			1, 0, 1,
			1, 0, 1,
			1, 1, 1
		},
		{
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,	
		},
		{
			1, 1, 1,
			0, 0, 1,
			1, 1, 1,
			1, 0, 0,
			1, 1, 1
		},
		{
			1, 1, 1,
			0, 0, 1,
			1, 1, 1,
			0, 0, 1,
			1, 1, 1
		},
		{
			1, 0, 1,
			1, 0, 1,
			1, 1, 1,
			0, 0, 1,
			0, 0, 1
		},
		{
			1, 1, 1,
			1, 0, 0,
			1, 1, 1,
			0, 0, 1,
			1, 1, 1
		},
		{
			1, 1, 1,
			1, 0, 0,
			1, 1, 1,
			1, 0, 1,
			1, 1, 1
		},
		{
			1, 1, 1,
			0, 0, 1,
			0, 0, 1,
			0, 0, 1,
			0, 0, 1
		},
		{
			1, 1, 1,
			1, 0, 1,
			1, 1, 1,
			1, 0, 1,
			1, 1, 1
		},
		{
			1, 1, 1,
			1, 0, 1,
			1, 1, 1,
			0, 0, 1,
			1, 1, 1
		},
	};

	for (int i = 0; i < 10; ++i) {
		for (int y = 0; y < 5; ++y) {
			for (int x = 0; x < 3; ++x) {
				if (bits[i][x + y * 3]) {
					NumFont[i][x + y * 3] = col;
				} else {
					NumFont[i][x + y * 3] = (Pixel) { 0 };
				}
			}
		}
	}
}

static int CompareFramesRowThresh;

static void PrintUsage(const char* app)
{
    fprintf(stderr, "Usage: %s (path/to/input/image or directory of images) path/to/output/image OPTIONS\n", app);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t--dir\n\t\tThis must be specified if you supplied a directory of images instead of a single file.\n");
    fprintf(stderr, "\t--frame-width DESIRED_FRAME_WIDTH\n\t\tDesired width of the frames.\n");
    fprintf(stderr, "\t--frame-height DESIRED_FRAME_HEIGHT\n\t\tDesired height of the frames.\n");
    fprintf(stderr, "\t-e EDGE_DISTANCE_THRESHOLD\n\t\tThe edge distance threshold is used to determine whether disconnected pixels still belongs to a frame.\n\t\tIf the distance from these pixels to the nearest edge is less than or equal to the\n\t\tthreshold, then they're incorporated.\n");
    fprintf(stderr, "\t--min-width MIN_FRAME_WIDTH\n\t--min-height MIN_FRAME_HEIGHT\n\t\tAll frames smaller than these in both dimensions will be discarded.\n\t\tBy default these are frame width / 4 and frame height / 4.\n");
    fprintf(stderr, "\t--pot\n\t\tThis is optional. It makes the app generate a power of two image.\n");
    fprintf(stderr, "\t--dest-width DESIRED_OUTPUT_IMAGE_WIDTH\n\t\tThis is optional and mutually exclusive with the --pot option.\n\t\tThis is the width you want the output image to be.\n\t\tThe height will be determined from the number of frames detected.\n");
	fprintf(stderr, "\t--row-thresh DESIRED_ROW_THRESHOLD\n\t\tThis is equal to half the frame height by default.\n\t\tIt is used to order the resulting frames. If two frames are within the threshold on the y axis\n\t\tthen they are ordered from left-to-right next to each other in the final image.\n");
	fprintf(stderr, "\t--label\n\t\tPrints the rectangle indices into the top-left corner of the frames.\n");
	fprintf(stderr, "\t--metadata\n\t\tIf specified, the rectangles are output to a text file in the format mentioned below.\n");
    fprintf(stderr, "\t--pack PACKED_IMAGE_WIDTH PACKED_IMAGE_HEIGHT\n\t\tIf this is supplied, then the frames are tightly packed and metadata is generated for each frame.\n\t\tThe metadata is simply a text file with the number of frames followed by 4 integers\n\t\tfor each frame: x y w h\n");
}

static bool ParseArgs(Args* args, int argc, char** argv)
{
    if(argc < 2) {
        fprintf(stderr, "Not enough arguments.\n");
        PrintUsage(argv[0]);
        return false;
    }
    
	memset(args, 0, sizeof(Args));

    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "-h") == 0) {
            PrintUsage(argv[0]);
            return false;
        } else if(strcmp(argv[i], "--dir") == 0) {
            args->isDir = true;
        } else if(strcmp(argv[i], "--frame-width") == 0) {
            args->fw = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "--frame-height") == 0) {
            args->fh = atoi(argv[i + 1]);
            i += 1;
		} else if (strcmp(argv[i], "--min-width") == 0) {
			args->minW = atoi(argv[i + 1]);
			i += 1;
		} else if (strcmp(argv[i], "--min-height") == 0) {
			args->minH = atoi(argv[i + 1]);
			i += 1;
		} else if (strcmp(argv[i], "--dest-width") == 0) {
            args->dw = atoi(argv[i + 1]);
            i += 1;
		} else if (strcmp(argv[i], "--metadata") == 0) {
			args->metadata = true;
		} else if (strcmp(argv[i], "-e") == 0) {
            args->maxDistFromEdge = atoi(argv[i + 1]);
            i += 1;
        } else if(strcmp(argv[i], "--pack") == 0) {
            args->packW = atoi(argv[i + 1]);
            args->packH = atoi(argv[i + 2]);
            i += 2;
        } else if(strcmp(argv[i], "--pot") == 0) {
            args->pot = true;
		} else if (strcmp(argv[i], "--label") == 0) {
			args->label = true;
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
    
    if(args->packW > 0 && args->packH == 0) {
        fprintf(stderr, "Invalid pack size.\n");
        return false;
    }

    if(args->packH > 0 && args->packW == 0) {
        fprintf(stderr, "Invalid pack size.\n");
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

    if(args->packW > 0 && args->packH > 0 && args->pot) {
        fprintf(stderr, "Cannot specify pack width and height and also power of two.\n");
        return false;
    }

    if(args->packW > 0 && args->packH > 0 && args->dw > 0) {
        fprintf(stderr, "Cannot specify pack width and height and also dest width.\n");
        return false;
    }

    if(args->pot && args->dw > 0) {
        fprintf(stderr, "Cannot specify both power of two and destination width.\n");
        return false;
    }
    
    if(args->packW == 0 && args->packH == 0 && !args->pot && args->dw <= 0) {
        fprintf(stderr, "Please specify either --pot or --dest-width.\n");
        return false;
    }

    if (args->fw > 0 && (args->dw > 0 && (args->dw % args->fw) != 0)) {
    	fprintf(stderr, "Dest width must be a multiple of frame width.\n");
    	return false;
    }

	if (args->minW == 0) {
		args->minW = args->fw / 4;
	}

	if (args->minH == 0) {
		args->minH = args->fh / 4;
	}

	if (args->label) {
		GenerateNumFont((Pixel) { 255, 255, 255, 255 });
	}

    if(args->packW > 0 && args->packH > 0) {
        args->metadata = true;
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

static int NumFrames = 0;
static Rect Frames[MAX_FRAMES];

static void ExtractFrames(const char* filename, const Args* args)
{
    int w, h, n;
    unsigned char* src = stbi_load(filename, &w, &h, &n, 4);

    if(!src) {
        fprintf(stderr, "Failed to load image '%s': %s\n", filename, stbi_failure_reason());
		if (args->isDir) {
			fprintf(stderr, "Skipping...\n");
		}
		return;
    }

    int fw = args->fw;
    int fh = args->fh;
    int maxDistFromEdge = args->maxDistFromEdge;

    // Top left pixel is bg color
    const Pixel* bg = (Pixel*)src;

    printf("Processing image '%s'...\n", filename);
    printf("image size: %d, %d\n", w, h);
    printf("bg: %d %d %d %d\n", bg->r, bg->g, bg->b, bg->a);

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
        
                if(!isBg) {
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
                }

                assert(numPoints < MAX_POINTS);

                points[numPoints++] = (Point){pt.x - 1, pt.y, newDistFromEdge};
                points[numPoints++] = (Point){pt.x, pt.y - 1, newDistFromEdge};
                points[numPoints++] = (Point){pt.x + 1, pt.y, newDistFromEdge};
                points[numPoints++] = (Point){pt.x, pt.y + 1, newDistFromEdge};
            }

            Rect r = { src, w, h, *bg, minX, minY, maxX - minX + 1, maxY - minY + 1 };

    		if (r.w < args->minW && r.h < args->minH) {
    			fprintf(stderr, "Found rect (%d,%d,%d,%d) but it's too small so I'm skipping it.\n", r.x, r.y, r.w, r.h);
    		} else if(r.w > fw) {
                fprintf(stderr, "Found rect (%d,%d,%d,%d) but it's too large to fit in a single frame so I'm skipping it.\n", r.x, r.y, r.w, r.h);
            } else {
                assert(NumFrames < MAX_FRAMES);
                Frames[NumFrames++] = r;
            }
        }
    }

    free(checked);
}

static void TraverseImages(tfFILE* file, void* data)
{
	ExtractFrames(file->path, data);
}

int main(int argc, char** argv)
{
    Args args;

    if(!ParseArgs(&args, argc, argv)) {
        return 1;
    }

    if(args.isDir) {
		tfTraverse(args.inputImage, TraverseImages, &args);
    } else {
        ExtractFrames(args.inputImage, &args);
    }

    if(NumFrames == 0) {
        fprintf(stderr, "I found no frames. Are you sure you supplied the correct input?\n");
        return 1;
    }
 
    qsort(Frames, NumFrames, sizeof(Rect), CompareFrames);

    int dw;
    int dh;

    static stbrp_rect rects[MAX_FRAMES];

    if(args.packW == 0 && args.packH == 0) {
        if(args.pot) {
            int reqArea = NumFrames * args.fw * args.fh;

            dw = args.fw;
            dh = args.fh;
         
            while(dw * dh < reqArea) {
                dw *= 2;
                dh *= 2;
            }
        } else {
            dw = args.dw;
            dh = ((NumFrames * args.fw) / dw + 1) * args.fh;
        }
    } else {
        dw = args.packW;
        dh = args.packH;

        stbrp_context ctx;

        stbrp_node* nodes = malloc(sizeof(stbrp_node) * dw);

        stbrp_init_target(&ctx, dw, dh, nodes, dw);

        for(int i = 0; i < NumFrames; ++i) {
            rects[i].w = Frames[i].w;
            rects[i].h = Frames[i].h;
        }

        if(stbrp_pack_rects(&ctx, rects, NumFrames) != 1) {
            fprintf(stderr, "Failed to pack (some) rectangles. Try again with a different size or don't pack at all.\n");
            return 1;
        }

        free(nodes);
    }
    if(args.metadata) {
        // Output rectangle metadata in top-left to bottom-right order
		char path[512];

		if (strlen(args.outputImage) >= 512) {
			fprintf(stderr, "Failed; output image path is too long to generate metadata.\n");
			return 1;
		}

		strcpy(path, args.outputImage);

		char* ext = strrchr(path, '.');

		if (!ext) {
			fprintf(stderr, "Failed; missing extension on output image path.\n");
			return 1;
		}

		ext[1] = 't';
		ext[2] = 'x';
		ext[3] = 't';
		ext[4] = '\0';

		FILE* file = fopen(path, "w");

		if (!file) {
			fprintf(stderr, "Failed to open metadata file '%s' for writing.\n", path);
			return 1;
		}

		fprintf(file, "%d\n", NumFrames);

		for (int i = 0; i < NumFrames; ++i) {
            if(args.packW > 0 && args.packH > 0) {
                fprintf(file, "%d %d %d %d\n", rects[i].x, rects[i].y, rects[i].w, rects[i].h);
            } else {
				int columns = dw / args.fw;

				int dx = (i % columns) * args.fw;
				int dy = (i / columns) * args.fh;

                fprintf(file, "%d %d %d %d\n", dx, dy, args.fw, args.fh);
            }
		}

		fclose(file);
		printf("Successfully wrote metadata to '%s'.\n", path);
    }
    
    unsigned char* dest = calloc(4, dw * dh);

    for(int i = 0; i < NumFrames; ++i) {
        Rect r = Frames[i];

        int dx, dy;

		int fw = args.fw;
		int fh = args.fh;

        if(args.packW == 0 && args.packH == 0) {
			int columns = dw / fw;

            dx = (i % columns) * fw + fw / 2 - r.w / 2;
            dy = (i / columns) * fh + fh / 2 - r.h / 2;
        } else {
            dx = rects[i].x;
            dy = rects[i].y;
        }

        for(int y = 0; y < r.h; ++y) {
            for(int x = 0; x < r.w; ++x) {
                Pixel sp = *(Pixel*)(&r.src[((x + r.x) * 4) + (y + r.y) * (r.sw * 4)]);

                if(PixelEqual(&sp, &r.bg)) continue;
                
                *(Pixel*)(&dest[(dx + x) * 4 + (y + dy) * (dw * 4)]) = sp; 
            }
        }

		if (args.label) {
			char num[32];
			sprintf(num, "%d", i);

			int len = strlen(num);

			for (int i = 0; i < len; ++i) {
				int digit = num[i] - '0';

				for (int y = 0; y < 5; ++y) {
					for (int x = 0; x < 3; ++x) {
						Pixel sp = NumFont[digit][x + y * 3];

						*(Pixel*)(&dest[(dx + x + i * 4) * 4 + (y + dy) * (dw * 4)]) = sp;
					}
				}
			}
		}
    }

    if(stbi_write_png(args.outputImage, dw, dh, 4, dest, dw * 4) == 0) {
        fprintf(stderr, "Failed to write file.\n");
    } else {
        printf("Succeeded.\n");
    }

    return 0;
}
