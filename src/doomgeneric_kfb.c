#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "doomgeneric.h"
#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include "doomkeys.h"

#define KINDLE_FRAME_BUFFER_PATH "/dev/fb0"
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 800
#define KEYQUEUE_SIZE 16


int frameBuffer;
static int frameCounter = 0;
int input_fd = -1;

unsigned short s_KeyQueue[KEYQUEUE_SIZE];
int s_KeyQueueReadIndex = 0;
int s_KeyQueueWriteIndex = 0;

int display_mode = 1; // default 1 for dithered, 2 for black and white, 3 for greyscale 

void log_message(const char* message) {
    time_t now;
    time(&now);
    struct tm* local = localtime(&now);

    // Print timestamp with message
    printf("[%02d:%02d:%02d] %s\n",
           local->tm_hour, local->tm_min, local->tm_sec,
           message);
}

static struct option long_options[] = {
    {"mode", required_argument, NULL, 'm'},
    {0, 0, 0, 0}
};

void parse_custom_options(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt_long(argc, argv, "m:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm': // --mode
                if (strcmp(optarg, "greyscale") == 0) {
                    display_mode = 3;
                    printf("greyscale mode ON\n");
                } else if (strcmp(optarg, "blackwhite") == 0) {
                    display_mode = 2;
                    printf("Black and white mode ON\n");
                } 
                break;
            default:
                // Do nothing if an unrecognized option is encountered
                break;
        }
    }
}

void apply_dithering(unsigned char *buffer, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Get the current pixel's grayscale value
            int oldPixel = buffer[y * width + x];
            // Quantize the pixel to 0 or 255 (black or white)
            int newPixel = oldPixel < 128 ? 0 : 255;
            // Set the pixel to its new quantized value
            buffer[y * width + x] = (unsigned char)newPixel;
            // Calculate the quantization error
            int error = oldPixel - newPixel;

            // Distribute the error to neighboring pixels
            if (x + 1 < width) buffer[y * width + (x + 1)] += error * 7 / 16;
            if (x - 1 >= 0 && y + 1 < height) buffer[(y + 1) * width + (x - 1)] += error * 3 / 16;
            if (y + 1 < height) buffer[(y + 1) * width + x] += error * 5 / 16;
            if (x + 1 < width && y + 1 < height) buffer[(y + 1) * width + (x + 1)] += error * 1 / 16;
        }
    }
}


void update_display() {
    FILE *file = fopen("/proc/eink_fb/update_display", "w");
    if (file == NULL) {
        perror("Failed to open /proc/eink_fb/update_display");
        exit(1);
    }
    fprintf(file, "1");
    fclose(file);
}


void pushKeyToDoomArray(int pressed, unsigned char doomKey) {
    unsigned short keyData = (pressed << 8) | doomKey;
    s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
    s_KeyQueueWriteIndex++;
    s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}


void pollInputs() {
    if (input_fd < 0) return;
    struct input_event ev;
    while (read(input_fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY) {
            int pressed = ev.value == 1 ? 1 : 0; // 1 for press, 0 for release
            unsigned char doomKey;

            // Map Kindle keys to DoomGeneric key codes
            switch (ev.code) {
                case KEY_LEFT:
                     printf("Key pressed: KEY_LEFT\n");
                    doomKey = KEY_LEFTARROW;
                    break;
                case KEY_RIGHT:
                    printf("Key pressed: KEY_RIGHT\n");
                    doomKey = KEY_RIGHTARROW;
                    break;
                case KEY_UP:
                    printf("Key pressed: KEY_UP\n");
                    doomKey = KEY_UPARROW;
                    break;
                case KEY_DOWN:
                    printf("Key pressed: KEY_DOWN\n");
                    doomKey = KEY_DOWNARROW;
                    break;
                case KEY_F24:
                    printf("Key pressed: KEY_ENTER\n");
                    doomKey = KEY_FIRE; // Map ENTER to firing
                    break;
                default:
                    continue; // Ignore unrecognized keys
            }

            pushKeyToDoomArray(pressed, doomKey);
        }
    }
}

void DG_Init() {
    log_message("DG_Init");

    // Open framebuffer
    frameBuffer = open(KINDLE_FRAME_BUFFER_PATH, O_RDWR);
    if (frameBuffer < 0) {
        perror("Failed to open framebuffer device");
        exit(1);
    }

    // open and hold main keyboard inputs
    input_fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        perror("Failed to open /dev/input/event1");
    }

    if (ioctl(input_fd, EVIOCGRAB, 1) < 0) {
        perror("Failed to grab input device");
    }

    // Clear the framebuffer
    // add a logo ?
    uint8_t clear_screen[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};
    write(frameBuffer, clear_screen, sizeof(clear_screen));
    log_message("Kindle Framebuffer cleared ? ");
}

void DG_DrawFrame() {
    log_message("DG_DrawFrame");
    // DoomGeneric's framebuffer is DOOMGENERIC_RESX x DOOMGENERIC_RESY
    // Resize if necessary to fit Kindle's resolution
    // uint8_t resized_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t backBuffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};
    uint8_t resized_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};

    int x_offset = (SCREEN_WIDTH - DOOMGENERIC_RESX) / 2;
    int y_offset = (SCREEN_HEIGHT - DOOMGENERIC_RESY) / 2;


    memset(resized_framebuffer, 0, sizeof(resized_framebuffer));

    // Copy or scale DoomGeneric framebuffer to Kindle's framebuffer
    for (int y = 0; y < DOOMGENERIC_RESY && y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < DOOMGENERIC_RESX && x < SCREEN_WIDTH; x++) {
          	pixel_t pixel = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];

            // RGBA bits
            uint8_t red = (pixel >> 16) & 0xFF;
            uint8_t green = (pixel >> 8) & 0xFF;
            uint8_t blue = pixel & 0xFF;

            // intensity= 0.299×R + 0.587×G + 0.114×B
            uint8_t intensity = (0.299 * red) + (0.587 * green) + (0.114 * blue);
            // brightness

            if(display_mode == 2) {
                intensity =  intensity > 60 ? 255 : 0;
            }
			//intensity = (intensity + 30 > 255) ? 255 : intensity + 30;


            backBuffer[(y + y_offset) * SCREEN_WIDTH + (x + x_offset)] = 255 - intensity;
        }
    }

    if(display_mode == 1) {
        apply_dithering(backBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

    }
	// backbuffer to main buffer to avoid flickering
    memcpy(resized_framebuffer, backBuffer, sizeof(backBuffer));

    // Write the resized buffer to /dev/fb0
    lseek(frameBuffer, 0, SEEK_SET);
    write(frameBuffer, resized_framebuffer, sizeof(resized_framebuffer));
    pollInputs();
    update_display();
}

void DG_Shutdown() {
    log_message("DG_Shutdown");

    // Close the framebuffer device
    close(frameBuffer);
}

void DG_SleepMs(uint32_t ms)
{
    usleep (ms * 1000);
}


uint32_t DG_GetTicksMs() {
    // log_message("DG_GetTicksMs: Tick...");
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint32_t)((time.tv_sec * 1000) + (time.tv_usec / 1000));
}


void DG_SetWindowTitle(const char * title) {
    log_message("DG_SetWindowTitle");
    uint8_t clear_screen[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};
    write(frameBuffer, clear_screen, sizeof(clear_screen));
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
        return 0; // Key queue is empty
    } else {
        unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
        s_KeyQueueReadIndex++;
        s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

        *pressed = keyData >> 8;    // Extract press/release state
        *doomKey = keyData & 0xFF;  // Extract Doom key code

        return 1;
    }
}


int main(int argc, char **argv)
{
    parse_custom_options(argc, argv);
    doomgeneric_Create(argc, argv);

    while (1)
    {
        doomgeneric_Tick();
    }

    return 0;
}

