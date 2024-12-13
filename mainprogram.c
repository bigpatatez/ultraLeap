
//
// Created by Belgin Ayvat on 12/12/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include "LeapC.h"
#include "ExampleConnection.h"
#include "bram.h"
#include "wifi.h"
#include "pthread.h"



#define MAX_FRAME 1
#define COOLDOWN_FRAMES 20
#define SWIPE_VELOCITY_THRESHOLD 30.0 // cm/s
#define BUFFER_SIZE 8

typedef enum {
    IDLE,
    COOLDOWN
  } SWIPE_STATE;

typedef enum {
    START,
    GRAB,
  } GRAB_STATE;

BRAMReader reader;

static LEAP_CONNECTION* connectionHandle;
SWIPE_STATE swipeState = IDLE;
GRAB_STATE grabState = START;
int swipe_frame_count = 0;
int grab_frame_count = 0;
int release_frame_count = 0;
int cooldown_frame_count = 0;

char* buffer;
pthread_t wifi_thread;
pthread_mutex_t buffer_mutex;


bool swiped(LEAP_HAND* hand) {
    //printf("palm velocity in x: %f cm/s\n",hand->palm.velocity.x/10.0);
    double velocity_x = hand->palm.velocity.x/10.0;
    switch (swipeState) {
        case IDLE:
            if(velocity_x>SWIPE_VELOCITY_THRESHOLD && (hand->type == eLeapHandType_Right)) {
                swipe_frame_count++;
                //printf("passed the threshold, frame_count = %d\n",swipe_frame_count);
                if(swipe_frame_count>=MAX_FRAME) {
                    printf("Right swipe detected\n\n");
                    //writeBRAMData(&reader,0,0x1);
                    //printf("Wrote to memory\n");
                    swipeState = COOLDOWN;
                    cooldown_frame_count = 0;
                    swipe_frame_count = 0;
                    return true;
                }
            } else {
                swipe_frame_count = 0;
            }
        break;
        case COOLDOWN:
            cooldown_frame_count++;
        if (cooldown_frame_count >= COOLDOWN_FRAMES) {
            swipeState = IDLE; // Reset to IDLE after cooldown
        }
        break;

        default:
            break;
    }

    return false;
}


void detectCutOff(LEAP_HAND* hand) {

    double pos_y = (hand->palm.position.y)/10.0;
    printf("%s hand with %s level\n",(hand->type == eLeapHandType_Left ? "left" : "right"),(pos_y >= 12 && pos_y<22)? "low":
    (pos_y>=22&&pos_y<32)?"medium":
    (pos_y>=32&&pos_y<42)?"high":"out of range");
}

void reverb_detect(LEAP_HAND* hand) {
    float squeeze = hand->grab_strength;
    //printf("hand squeezed with strength: %f\n",squeeze);
    switch(grabState) {
        case START:
            if(squeeze == 1.0) {
                printf("Time based effect start\n");
                grabState = GRAB;
            }
        break;
        case GRAB:
            if(squeeze==0) {
                printf("End signal detected\n");
                grabState = START;
            }
        break;

        default:
            grabState = START;
        break;
    }
}


//Callback functions start here//

/** Callback for when the connection opens. */
static void OnConnect(void){
    printf("Connected.\n");
    int success = initBRAMReader(&reader);
    if(success == -1){
        printf("error when initializing the bram reader\n");
    }
    pthread_mutex_init(&buffer_mutex, NULL);
    buffer = malloc(sizeof(char)*BUFFER_SIZE);
    ThreadArgs args;
    args.buffer = buffer;
    args.size = BUFFER_SIZE;
    args.mutex = &buffer_mutex;
    args.reader = &reader;
    pthread_create(&wifi_thread, NULL,wifiRoutine, &args);
}

/** Callback for when a device is found. */
static void OnDevice(const LEAP_DEVICE_INFO *props){
    printf("Found device %s.\n", props->serial);
}

static void OnLogMessage(const eLeapLogSeverity severity, const int64_t timestamp,
                         const char* message) {
    const char* severity_str;
    switch(severity) {
        case eLeapLogSeverity_Critical:
            severity_str = "Critical";
        break;
        case eLeapLogSeverity_Warning:
            severity_str = "Warning";
        break;
        case eLeapLogSeverity_Information:
            severity_str = "Info";
        break;
        default:
            severity_str = "";
        break;
    }
    printf("[%s][%lli] %s\n", severity_str, (long long int)timestamp, message);
}

static void* allocate(uint32_t size, eLeapAllocatorType typeHint, void* state) {
    void* ptr = malloc(size);
    return ptr;
}

static void deallocate(void* ptr, void* state) {
    if (!ptr)
        return;
    free(ptr);
}
uint32_t data;
/** Callback for when a frame of tracking data is available. */
static void OnFrame(const LEAP_TRACKING_EVENT *frame)
{
    if (frame->info.frame_id %100==0)
        {printf("Frame %lli with %i hands.\n", (long long int)frame->info.frame_id, frame->nHands);
        readBRAMData(&reader,0,&data);
	printf("buffer value %d\n",data);
	}

    for(uint32_t h = 0; h < frame->nHands; h++){
        LEAP_HAND* hand = &frame->pHands[h];
        swiped(hand);
        //pthread_mutex_lock(&buffer_mutex);
        //printf("current buffer value %s\n", buffer);
        //pthread_mutex_unlock(&buffer_mutex);
        //reverb_detect(hand);
        // detectCutOff( hand);

    }
}

int main(int argc, char** argv) {
    //Set callback function pointers
    ConnectionCallbacks.on_connection          = &OnConnect;
    ConnectionCallbacks.on_device_found        = &OnDevice;
    ConnectionCallbacks.on_frame               = &OnFrame;
    //ConnectionCallbacks.on_image               = &OnImage;
    //ConnectionCallbacks.on_point_mapping_change = &OnPointMappingChange;
    ConnectionCallbacks.on_log_message         = &OnLogMessage;
    //ConnectionCallbacks.on_head_pose           = &OnHeadPose;

    connectionHandle = OpenConnection();
    {
        LEAP_ALLOCATOR allocator = { allocate, deallocate, NULL };
        LeapSetAllocator(*connectionHandle, &allocator);
    }
    LeapSetPolicyFlags(*connectionHandle, eLeapPolicyFlag_Images | eLeapPolicyFlag_MapPoints, 0);

    printf("Press Enter to exit program.\n");




    getchar();

    CloseConnection();
    DestroyConnection();

    return 0;
}
