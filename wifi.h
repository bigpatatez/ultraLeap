//
// Created by Belgin Ayvat on 12/12/2024.
//
#ifndef WIFI_H
#define WIFI_H
#include "pthread.h"

typedef struct {
    char *buffer;// Pointer to the shared buffer
    int size;
    pthread_mutex_t *mutex; // Pointer to the mutex
} ThreadArgs;

char* get_local_ip();
void* wifiRoutine(void *arg);
#endif //WIFI_H
