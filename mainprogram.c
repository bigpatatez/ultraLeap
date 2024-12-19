
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



//VALUES THAT IS NEEDED TO BE SET
#define PAUSE 1
#define NO_MODE 0
#define VOLUME_MODE 1
#define FILTER_MODE 2
#define SLOW_MODE 3

#define VOL_LOW 0
#define VOL_MED 50
#define VOL_HIGH 100

#define SLOWDOWN_EN 1

#define FILTER1 1
#define FILTER2 2
#define FILTER3 3
#define FILTER4 4



typedef enum {
    IDLE,
    COOLDOWN
  } SWIPE_STATE;

typedef enum {
    START,
    GRAB,
  } GRAB_STATE;

typedef enum {
	NOMODE,
	VOLUME,
	REVERB,
	FILTER
}MODE;

typedef enum {
	NOFILTER,
	FILTER_1,
	FILTER_2,
	FILTER_3,
	FILTER_4
}FILTER_TYPE;

BRAMReader reader;

static LEAP_CONNECTION* connectionHandle;
SWIPE_STATE swipeState = IDLE;
GRAB_STATE grabState = START;
MODE mode = NOMODE;
FILTER_TYPE filter_t = NOFILTER; 
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
                    printf("Switching songs\n\n");
           			// here something needs to be written down to memory to change the currently playing song
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


void volumeLevel(LEAP_HAND* hand) {

    double pos_y = (hand->palm.position.y)/10.0;
    if(pos_y >= 12 && pos_y<22){
    	pthread_mutex_lock(&buffer_mutex);
    	uint32_t volume = (uint32_t)((0 * 0xFFFF) / 100);
    	writeBRAMData(&reader,0,volume);
    	pthread_mutex_unlock(&buffer_mutex);
    	printf("volume low: wrote 0 to bram\n");
    }
    else if(pos_y>=22&&pos_y<32){
    	pthread_mutex_lock(&buffer_mutex);
  		uint32_t volume = (uint32_t)((50 * 0xFFFF) / 100);
    	writeBRAMData(&reader,0,volume);
    	pthread_mutex_unlock(&buffer_mutex);
    	printf("volume medium: wrote 50 to bram\n");
    }
    else if(pos_y>=32&&pos_y<42){
    	pthread_mutex_lock(&buffer_mutex);
    	uint32_t volume = (uint32_t)((100 * 0xFFFF) / 100);
    	writeBRAMData(&reader,0,volume);
    	pthread_mutex_unlock(&buffer_mutex);
    	printf("volume high: wrote 100 to bram\n");
    }
    else{
    	printf("out of range\n");
    }
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

void getMode(uint32_t data){
	
	if(data == 1){
		mode = VOLUME;	
	}
	else if(data == 2){
		mode = REVERB;
	}
	else if(data == 3){
		mode = FILTER;
	}
	else if(data == 0){
		mode = NOMODE;
	}
	
}


uint32_t data;
/** Callback for when a frame of tracking data is available. */
static void OnFrame(const LEAP_TRACKING_EVENT *frame)
{
    if (frame->info.frame_id %100==0)
        {printf("Frame %lli with %i hands.\n", (long long int)frame->info.frame_id, frame->nHands);

		pthread_mutex_lock(&buffer_mutex);
        readBRAMData(&reader,1,&data);
        pthread_mutex_unlock(&buffer_mutex);
		printf("buffer value %#x\n",data);
		getMode(data);

	}
	

    for(uint32_t h = 0; h < frame->nHands; h++){
        LEAP_HAND* hand = &frame->pHands[h];
        swiped(hand);
        switch(mode){
        	case NOMODE:
        		printf("idle mode\n");
        	break;
        	case VOLUME:
        		volumeLevel(hand);
        		printf("volume mode\n");
        		
        	break;
        	case REVERB:
        		printf("reverb mode\n");
        	break;
        	case FILTER:
        		printf("Filter mode!\n");
        	break;

        	default:
        		break;
 		
        }
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
