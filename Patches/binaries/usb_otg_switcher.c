/*
 * Copyright (C) 2025 ravindu644. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#define POWER_ROLE_PATH "/sys/class/typec/port0/power_role"
#define USB_ROLE_PATH "/sys/devices/platform/soc/11201000.usb0/usb_role/11201000.usb0-role-switch/role"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main() {
    int fd, wd;
    char buffer[BUF_LEN];

    printf("Initializing USB OTG switcher...\n");

    // Initialize inotify
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return 1;
    }
    printf("Inotify initialized successfully.\n");

    // Add watch to the power_role file
    wd = inotify_add_watch(fd, POWER_ROLE_PATH, IN_MODIFY);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return 1;
    }
    printf("Watching %s for changes.\n", POWER_ROLE_PATH);

    // Read initial state
    int role_fd = open(POWER_ROLE_PATH, O_RDONLY);
    if (role_fd < 0) {
        perror("open power_role");
        close(fd);
        return 1;
    }
    char initial_role[256];
    ssize_t len = read(role_fd, initial_role, sizeof(initial_role) - 1);
    if (len > 0) {
        initial_role[len] = '\0';
        printf("Initial power_role: %s", initial_role);
        if (strstr(initial_role, "[source] sink") != NULL) {
            // Switch to host
            int usb_fd = open(USB_ROLE_PATH, O_WRONLY);
            if (usb_fd >= 0) {
                write(usb_fd, "host", 4);
                close(usb_fd);
                printf("Switched to host mode.\n");
            } else {
                perror("open usb_role for host");
            }
        } else if (strstr(initial_role, "source [sink]") != NULL) {
            // Switch to device
            int usb_fd = open(USB_ROLE_PATH, O_WRONLY);
            if (usb_fd >= 0) {
                write(usb_fd, "device", 6);
                close(usb_fd);
                printf("Switched to device mode.\n");
            } else {
                perror("open usb_role for device");
            }
        } else {
            printf("Initial role does not match expected patterns.\n");
        }
    } else {
        perror("read initial role");
    }
    close(role_fd);

    printf("Monitoring for changes...\n");

    // Monitor for changes
    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->mask & IN_MODIFY) {
                printf("Power role file modified.\n");
                // Read the current role
                int role_fd = open(POWER_ROLE_PATH, O_RDONLY);
                if (role_fd >= 0) {
                    char role[256];
                    ssize_t len = read(role_fd, role, sizeof(role) - 1);
                    if (len > 0) {
                        role[len] = '\0';
                        printf("Current power_role: %s", role);
                        if (strstr(role, "[source] sink") != NULL) {
                            // Switch to host
                            int usb_fd = open(USB_ROLE_PATH, O_WRONLY);
                            if (usb_fd >= 0) {
                                write(usb_fd, "host", 4);
                                close(usb_fd);
                                printf("Switched to host mode.\n");
                            } else {
                                perror("open usb_role for host");
                            }
                        } else if (strstr(role, "source [sink]") != NULL) {
                            // Switch to device
                            int usb_fd = open(USB_ROLE_PATH, O_WRONLY);
                            if (usb_fd >= 0) {
                                write(usb_fd, "device", 6);
                                close(usb_fd);
                                printf("Switched to device mode.\n");
                            } else {
                                perror("open usb_role for device");
                            }
                        } else {
                            printf("Role does not match expected patterns.\n");
                        }
                    } else {
                        perror("read role");
                    }
                    close(role_fd);
                } else {
                    perror("open power_role");
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }

    // Clean up
    inotify_rm_watch(fd, wd);
    close(fd);
    printf("Exiting USB OTG switcher.\n");

    return 0;
}
