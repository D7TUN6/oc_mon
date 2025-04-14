#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void getAvailableDisplays(char displays[][50], int *count) {
    FILE *fp;
    char buffer[256];

    fp = popen("xrandr", "r");
    if (fp == NULL) {
        printf("Failed to run xrandr command.\n");
        return;
    }

    printf("Available displays:\n");
    *count = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, " connected") != NULL) {
            sscanf(buffer, "%s", displays[*count]);
            printf("[%d] %s\n", *count + 1, displays[*count]);
            (*count)++;
        }
    }

    pclose(fp);
}

int main() {
    int num1, num2, num3;
    printf("Enter resolution width:\n");
    scanf("%d", &num1);
    printf("Enter resolution height:\n");
    scanf("%d", &num2);
    printf("Enter refresh rate in hz:\n");
    scanf("%d", &num3);

    char command[256];
    sprintf(command, "cvt %d %d %d", num1, num2, num3);

    FILE *fp;
    char buffer[256];
    char modeline[256];
    char displayName[50];

    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run cvt command\n");
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "Modeline", 8) == 0) {
            strcpy(modeline, buffer);
            break;
        }
    }
    pclose(fp);

    modeline[strcspn(modeline, "\n")] = 0;

    char displays[10][50]; 
    int count;
    getAvailableDisplays(displays, &count);

    int choice;
    printf("Select a display by number: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > count) {
        printf("Invalid choice.\n");
        return 1;
    }

    strcpy(displayName, displays[choice - 1]);

    char modeName[50];
    sscanf(modeline, "Modeline \"%[^\"]\"", modeName);
    char addModeCommand[256];
    sprintf(addModeCommand, "xrandr --newmode %s", modeline + 8);

    system(addModeCommand);

    char addModeDisplayCommand[256];
    sprintf(addModeDisplayCommand, "xrandr --addmode %s %s", displayName, modeName);
    system(addModeDisplayCommand);

    char apply;
    printf("Do you want to apply the new resolution now? (y/n): ");
    scanf(" %c", &apply);
    
    if (apply == 'y' || apply == 'Y') {
        char outputCommand[256];
        sprintf(outputCommand, "xrandr --output %s --mode \"%s\"", displayName, modeName);
        system(outputCommand);
        printf("Resolution applied to %s.\n", displayName);
    } else {
        printf("Changes have not been applied.\n");
    }

    return 0;
}
