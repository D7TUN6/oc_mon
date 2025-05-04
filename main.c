#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define MAX_DISPLAYS 10
#define DISPLAY_NAME_LEN 50
#define CMD_BUFFER_LEN 256
#define MODELINE_LEN 256

typedef enum {
    SUCCESS = 0,
    ERR_SYSTEM_CALL,
    ERR_INVALID_INPUT,
    ERR_NO_DISPLAYS
} ErrorCode;

typedef struct {
    char name[DISPLAY_NAME_LEN];
} Display;

ErrorCode getAvailableDisplays(Display displays[], size_t *count) {
    FILE *fp = popen("xrandr", "r");
    if (fp == NULL) {
        perror("Failed to run xrandr command");
        return ERR_SYSTEM_CALL;
    }

    printf("Available displays:\n");
    *count = 0;
    char buffer[CMD_BUFFER_LEN];

    while (fgets(buffer, sizeof(buffer), fp) != NULL && *count < MAX_DISPLAYS) {
        if (strstr(buffer, " connected") != NULL) {
            if (sscanf(buffer, "%49s", displays[*count].name) == 1) {
                printf("[%zu] %s\n", *count + 1, displays[*count].name);
                (*count)++;
            }
        }
    }

    pclose(fp);
    return (*count == 0) ? ERR_NO_DISPLAYS : SUCCESS;
}

ErrorCode safeInputInt(const char *prompt, int *value) {
    char input[32];
    char *endptr;
    long val;

    printf("%s", prompt);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return ERR_INVALID_INPUT;
    }

    errno = 0;
    val = strtol(input, &endptr, 10);
    if (errno != 0 || *endptr != '\n' || val < 0 || val > INT_MAX) {
        return ERR_INVALID_INPUT;
    }

    *value = (int)val;
    return SUCCESS;
}

ErrorCode executeCommand(const char *command) {
    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "Command failed: %s\n", command);
        return ERR_SYSTEM_CALL;
    }
    return SUCCESS;
}

int main(void) {
    int width, height, refresh_rate;
    Display displays[MAX_DISPLAYS];
    size_t display_count = 0;
    ErrorCode ret = SUCCESS;

    if (safeInputInt("Enter resolution width: ", &width) != SUCCESS ||
        safeInputInt("Enter resolution height: ", &height) != SUCCESS ||
        safeInputInt("Enter refresh rate in Hz: ", &refresh_rate) != SUCCESS) {
        fprintf(stderr, "Invalid input parameters\n");
        return ERR_INVALID_INPUT;
    }

    char modeline[MODELINE_LEN] = {0};
    char command[CMD_BUFFER_LEN];
    snprintf(command, sizeof(command), "cvt %d %d %d", width, height, refresh_rate);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run cvt command");
        return ERR_SYSTEM_CALL;
    }

    char buffer[CMD_BUFFER_LEN];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "Modeline", 8) == 0) {
            strncpy(modeline, buffer, sizeof(modeline) - 1);
            break;
        }
    }
    pclose(fp);

    if (modeline[0] == '\0') {
        fprintf(stderr, "No modeline found\n");
        return ERR_SYSTEM_CALL;
    }

    modeline[strcspn(modeline, "\n")] = '\0';

    if ((ret = getAvailableDisplays(displays, &display_count)) != SUCCESS) {
        fprintf(stderr, "No available displays found\n");
        return ret;
    }

    int choice;
    printf("Select a display by number (1-%zu): ", display_count);
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > (int)display_count) {
        fprintf(stderr, "Invalid choice\n");
        return ERR_INVALID_INPUT;
    }

    char mode_name[DISPLAY_NAME_LEN] = {0};
    if (sscanf(modeline, "Modeline \"%49[^\"]\"", mode_name) != 1) {
        fprintf(stderr, "Failed to parse modeline\n");
        return ERR_SYSTEM_CALL;
    }

    snprintf(command, sizeof(command), "xrandr --newmode %s", modeline + 8);
    if ((ret = executeCommand(command)) != SUCCESS) return ret;

    snprintf(command, sizeof(command), "xrandr --addmode %s %s", 
             displays[choice - 1].name, mode_name);
    if ((ret = executeCommand(command)) != SUCCESS) return ret;

    char apply;
    printf("Apply the new resolution now? (y/n): ");
    scanf(" %c", &apply);

    if (apply == 'y' || apply == 'Y') {
        snprintf(command, sizeof(command), "xrandr --output %s --mode \"%s\"", 
                 displays[choice - 1].name, mode_name);
        if ((ret = executeCommand(command)) != SUCCESS) return ret;
        printf("Resolution applied to %s.\n", displays[choice - 1].name);
    } else {
        printf("Changes have not been applied.\n");
    }

    return SUCCESS;
}
