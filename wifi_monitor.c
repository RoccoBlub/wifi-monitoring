#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define INFLUX_URL "http://localhost:8086/api/v2/write?org=wifi_monitoring_org&bucket=wifi_monitoring&precision=s"
#define TOKEN ""  // Replace with your actual token
#define PING_CMD "ping -c 1 8.8.8.8 -W 2 -c 1 > /dev/null 2>&1"
#define SIGNAL_CMD "iwconfig wlan0 | grep 'Signal level' | awk '{print $4}' | cut -d'=' -f2"

char location[50];

void log_to_influx(const char *status, int signal_strength) {
    char command[512];

    char location_escaped[100];
    snprintf(location_escaped, sizeof(location_escaped), "%s", location);
    for (char *p = location_escaped; *p; p++) {
        if (*p == ' ') *p = '\\';
    }

    snprintf(command, sizeof(command),
             "curl -i -XPOST '%s' --header 'Authorization: Token %s' --header 'Content-Type: text/plain' --data-binary 'wifi_status,location=%s,status=%s strength=%di %ld'",
             INFLUX_URL, TOKEN, location_escaped, status, signal_strength, time(NULL));

    system(command);
}

void check_wifi() {
    int result = system(PING_CMD);
    if (result != 0) {
        log_to_influx("Disconnected", -100);
        return;
    }

    FILE *fp = popen(SIGNAL_CMD, "r");
    if (fp == NULL) {
        perror("Error getting WiFi signal strength");
        return;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        int signal_strength = atoi(buffer);
        log_to_influx("Connected", signal_strength);
    } else {
        log_to_influx("Disconnected", -100);
    }

    pclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <location>\n", argv[0]);
        return 1;
    }
    strncpy(location, argv[1], sizeof(location) - 1);
    location[sizeof(location) - 1] = '\0';

    while (1) {
        check_wifi();
        sleep(30);  // Check every 30 seconds
    }
    return 0;
}