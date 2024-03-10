#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>

#include "dblogo.h"

// LED display constants
#define PIN 22
#define WIDTH 71
#define HEIGHT 16

// Colour constants (Our LED strips use 16bit GRB)
#define BRIGHTNESS 32
#define BLACK    matrix->Color(0, 0, 0)
#define BLUE     matrix->Color(0, 0, 255)
#define RED      matrix->Color(255, 0, 0)
#define GREEN    matrix->Color(0, 255, 0)
#define CYAN     matrix->Color(0, 255, 255)
#define MAGENTA  matrix->Color(255, 0, 255)
#define YELLOW   matrix->Color(255, 255, 0)
#define WHITE    matrix->Color(255, 255, 255)

// Animation constants
#define EYE_L_X 20
#define EYE_L_Y 7
#define EYE_R_X 50
#define EYE_R_Y 7
#define EYE_RADIUS 7
#define PUPIL_RADIUS 1
#define NOTE_X 12
#define NOTE_SIZE_X 50
#define NOTE_SIZE_Y 25
#define NOTE_THICKNESS 8
#define O_MOUTH_Y 12
#define O_MOUTH_R 3

// Status bar constants
#define STATUS_WIDTH 8
#define FLASH_SPEED 4  // ticks

// Instantiating the LED display
Adafruit_NeoMatrix *matrix = new Adafruit_NeoMatrix(
    WIDTH, HEIGHT, PIN,
    NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
    NEO_RGB + NEO_KHZ800);

enum StatusPattern {
  EMPTY,
  INTAKE,
  IN_RANGE,
  NOT_IN_RANGE,
  CLIMB_EXTENDING,
  CLIMB_EXTENDED,
  CLIMB_RETRACTED,
  OFF
};

// Struct definitions
typedef void (*Frame_Ptr)(int);

typedef struct{
    Frame_Ptr render;
    int duration;
}Frame;

typedef struct {
    int ticks;
    int frame_index;
    int frames_len;
    Frame *frames;
}Animation;


// Initialisation functions
Frame create_frame(Frame_Ptr render, int duration) {
    Frame new_frame;

    new_frame.render = render;
    new_frame.duration = duration;

    return new_frame;
}

Animation create_animation(int frames_len, Frame *frames) {
    Animation new_animation;

    new_animation.ticks = 0;
    new_animation.frame_index = 0;
    new_animation.frames_len = frames_len;
    new_animation.frames = frames;

    return new_animation;
}

///////////////////////////////////////////////////////////////////////////////

void eyes_static(int ticks) {
    matrix->fillCircle(EYE_L_X, EYE_L_Y, EYE_RADIUS, WHITE);
    matrix->fillCircle(EYE_R_X, EYE_R_Y, EYE_RADIUS, WHITE);
    matrix->fillCircle(EYE_L_X, EYE_L_Y, PUPIL_RADIUS, BLACK);
    matrix->fillCircle(EYE_R_X, EYE_R_Y, PUPIL_RADIUS, BLACK);
}

// Duration=28
void eyes_look_leftright(int ticks) {
    if (ticks < EYE_RADIUS) {
        matrix->fillCircle(EYE_L_X, EYE_L_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_R_X, EYE_R_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_L_X - ticks, EYE_L_Y, PUPIL_RADIUS, BLACK);
        matrix->fillCircle(EYE_R_X - ticks, EYE_R_Y, PUPIL_RADIUS, BLACK);
    } else if (ticks < EYE_RADIUS * 3) {
        matrix->fillCircle(EYE_L_X, EYE_L_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_R_X, EYE_R_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_L_X - (EYE_RADIUS) + (ticks-EYE_RADIUS), EYE_L_Y, PUPIL_RADIUS, BLACK);
        matrix->fillCircle(EYE_R_X - (EYE_RADIUS) + (ticks-EYE_RADIUS), EYE_R_Y, PUPIL_RADIUS, BLACK);
    } else {
        matrix->fillCircle(EYE_L_X, EYE_L_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_R_X, EYE_R_Y, EYE_RADIUS, WHITE);
        matrix->fillCircle(EYE_L_X + (EYE_RADIUS * 4) - ticks, EYE_L_Y, PUPIL_RADIUS, BLACK);
        matrix->fillCircle(EYE_R_X + (EYE_RADIUS * 4) - ticks, EYE_R_Y, PUPIL_RADIUS, BLACK);
    }
}

// Duration=16
void eyes_blink(int ticks) {
    matrix->fillCircle(EYE_L_X, EYE_L_Y, EYE_RADIUS, WHITE);
    matrix->fillCircle(EYE_R_X, EYE_R_Y, EYE_RADIUS, WHITE);
    matrix->fillCircle(EYE_R_X, EYE_R_Y, PUPIL_RADIUS, BLACK);
    matrix->fillCircle(EYE_L_X, EYE_L_Y, PUPIL_RADIUS, BLACK);
    if (ticks <= HEIGHT / 2) {
        matrix->fillRect(0, 0, WIDTH, ticks, BLACK);
        matrix->fillRect(0, HEIGHT - ticks, WIDTH, HEIGHT, BLACK);
    } else {
        matrix->fillRect(0, -1, WIDTH, HEIGHT - ticks, BLACK);
        matrix->fillRect(0, ticks, WIDTH, HEIGHT, BLACK);
    }
}

///////////////////////////////////////////////////////////////////////////////

int read_serial_port(u_int8_t packet) {
    if(Serial.available() > 0) {
        Serial.readBytes(&packet, 1);  // Read the packet
        Serial.flush();
        return 1;
    }
    return 0;
}

void render_status(int colour) {
    matrix->fillRect(0, 0, STATUS_WIDTH, HEIGHT, colour);
    matrix->fillRect(WIDTH-STATUS_WIDTH, 0, STATUS_WIDTH, HEIGHT, colour);
}

void render_status_flashing(int colour, int ticks) {
    if ((ticks % (FLASH_SPEED*2)) >= FLASH_SPEED) {
        matrix->fillRect(0, 0, STATUS_WIDTH, HEIGHT, colour);
        matrix->fillRect(WIDTH-STATUS_WIDTH, 0, STATUS_WIDTH, HEIGHT, colour);
    }
    else {
        matrix->fillRect(0, 0, STATUS_WIDTH, HEIGHT, BLACK);
        matrix->fillRect(WIDTH-STATUS_WIDTH, 0, STATUS_WIDTH, HEIGHT, BLACK);
    }
}


// Create animation frames
const int eye_frames_len = 3;
Frame eye_frames[eye_frames_len] = {
    create_frame(&eyes_static, 30),
    create_frame(&eyes_look_leftright, 28),
    create_frame(&eyes_blink, 16),
};

// Create animations
int animation_index = 0;
const int animations_len = 1;
Animation animations[animations_len] = {
    create_animation(eye_frames_len, eye_frames)
};

uint8_t packet = 0;
uint8_t global_ticks = 0;

uint8_t pattern = INTAKE;  // Will only render this pattern if the highpriority pattern == CLIMB_RETRACTED
uint8_t high_priority_pattern = CLIMB_RETRACTED;


void setup() {
    matrix->begin();
    matrix->setTextWrap(false);
    matrix->setBrightness(BRIGHTNESS);

    Serial.begin(9600);
}


void loop() {
    // INPUT
    if (read_serial_port(packet)) {
        // Set everything
        uint8_t status_lights = (packet >> 4) & 0b111;  // Bit shift to status section of packet
        switch (status_lights) {
        case 4:
            high_priority_pattern = CLIMB_EXTENDING;
            break;
        case 5:
            high_priority_pattern = CLIMB_EXTENDED;
            break;
        case 6:
            high_priority_pattern = CLIMB_RETRACTED;
            break;
        default:
            pattern = status_lights;
        }
    }

    // UPDATE
    global_ticks++;

    // RENDER
    matrix->clear();

    // Draw current animation's current frame
    Animation* animation = &animations[animation_index];
    Frame frame = animation->frames[animation->frame_index];
    frame.render(animation->ticks);
    animation->ticks++;

    // Go to next frame
    if (animation->ticks >= frame.duration) {
        animation->frame_index++;
        animation->frame_index %= animation->frames_len;
        animation->ticks = 0;
    }

    // Draw current status as bars over animation
        if (high_priority_pattern == CLIMB_RETRACTED) {
        switch (pattern){
            case EMPTY:
                render_status(WHITE);
                break;
            case INTAKE:
                render_status(MAGENTA);
                break;
            case IN_RANGE:
                render_status(GREEN);
                break;
            case NOT_IN_RANGE:
                render_status(RED);
                break;
            case OFF:
                render_status(BLACK);
                break;
        }
    }
    else {
        switch (high_priority_pattern){
            case CLIMB_EXTENDING:
                render_status_flashing(YELLOW, global_ticks);
                break;
            case CLIMB_EXTENDED:
                render_status(YELLOW);
                break;
        }
    }

    delay(1);
    matrix->show();
}
