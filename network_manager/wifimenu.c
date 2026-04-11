#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_NETWORKS 64
#define MAX_SSID_LEN 128
#define MAX_CMD_LEN 512
#define READ_END 0
#define WRITE_END 1


typedef struct {
    char ssid[MAX_SSID_LEN];
    int signal;
} Network;


int get_networks(Network * nets) {
    FILE *fp = popen("nmcli -f SSID,SIGNAL -t dev wifi list", "r");

    if(!fp) return 0;

    int count = 0;
    char line[MAX_SSID_LEN+16];

    while(fgets(line,sizeof(line), fp) && count < MAX_NETWORKS) {
        char * colon = strrchr(line, ':');
        if(!colon) continue;
        *colon = '\0';
        strncpy(nets[count].ssid, line, MAX_SSID_LEN -1);
        nets[count].signal = atoi(colon +1);
        count++;
    }

    pclose(fp);
    return count;
}

char *dmenu_select(Network *nets, int count) {
    int stdin_pipe[2];
    int stdout_pipe[2];

    pipe(stdin_pipe); /* parent writes SSIDs to dmenu stdin */
    pipe(stdout_pipe); /* parent reads selection from dmenu stdout */

    pid_t pid = fork();
    if (pid == 0) {
        dup2(stdin_pipe[READ_END], STDIN_FILENO);
        dup2(stdout_pipe[WRITE_END], STDOUT_FILENO);
        close(stdin_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[READ_END]);
        close(stdout_pipe[WRITE_END]);
        execlp("dmenu", "dmenu", "-p", "WiFi:", NULL);
        exit(1);
    }

    close(stdin_pipe[READ_END]);
    close(stdout_pipe[WRITE_END]);

    for (int i = 0; i < count; i++) {
        write(stdin_pipe[WRITE_END], nets[i].ssid, strlen(nets[i].ssid));
        write(stdin_pipe[WRITE_END], "\n", 1);
    }
    close(stdin_pipe[1]);

    static char selection[MAX_SSID_LEN];
    int n = read(stdout_pipe[READ_END], selection, sizeof(selection) - 1);
    close(stdout_pipe[READ_END]);
    if (n <= 0) return NULL;
    selection[n] = '\0';
    selection[strcspn(selection, "\n")] = '\0';
    return selection;
}

void connect_network(const char *ssid) {
    char password[256] = {0};
    int stdin_pipe[2];
    int stdout_pipe[2];

    pipe(stdin_pipe);
    pipe(stdout_pipe);

    pid_t pid = fork();
    if(pid == 0) {
        dup2(stdin_pipe[READ_END], STDIN_FILENO);
        dup2(stdout_pipe[WRITE_END], STDOUT_FILENO);
        close(stdin_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[READ_END]);
        close(stdout_pipe[WRITE_END]);
        execlp("dmenu", "dmenu", "-p", "Password:", "-P", NULL);
        exit(1);
    }

    close(stdin_pipe[READ_END]);
    close(stdin_pipe[WRITE_END]);
    close(stdout_pipe[WRITE_END]);

    int n = read(stdout_pipe[READ_END], password, sizeof(password) -1);
    close(stdout_pipe[READ_END]);
    waitpid(pid,NULL,0);

    if(n <= 0 ) return;
    password[n] = '\0';
    password[strcspn(password, "\n")] = '\0';


    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "nmcli dev wifi connect \"%s\" >> /tmp/wifi.log 2>&1", ssid);
    system(cmd);
}

int main(void) {
    Network nets[MAX_NETWORKS];
    
    int count = get_networks(nets);
    if(count == 0 )return 1;

    char * selected = dmenu_select(nets, count);
    if(!selected) return 1;
    connect_network(selected);
    printf("Selected: %s\n", selected);
    return 0;
}