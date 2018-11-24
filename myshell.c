#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

#define MAX_CMD_LEN 1000
#define WRITE 1
#define READ 0

const char ex[] = "exit";
const char pip[] = "|";
const char separators[] = " \n";

int parse_commands(char ****cmds, int *num_cmds, int num_leks, char **leks);

int free_commands(char ***cmd, int num_cmds);

int execute(char ***cmd, int num_cmd);


int main() {
    char ***cmd = NULL;
    int num_cmd;
    char buffer[MAX_CMD_LEN];
    char *tokens[MAX_CMD_LEN];
    char *correct_tokens[MAX_CMD_LEN];
    char *ptr = NULL;
    int num_tokens, num_rightleks;


    while (1) {
        write(1, "@ ", 2);
        cmd = NULL;
        num_cmd = 1;
        ptr = NULL;
        num_tokens = 0, num_rightleks = 0;

        if (fgets(buffer, MAX_CMD_LEN, stdin) <= 0)
            return 0;

        if ((ptr = strtok(buffer, separators)) == NULL)
            continue;

        if (strcmp(ptr, ex) == 0)
            return 0;

        while (ptr != NULL) {
            tokens[num_tokens++] = ptr;
            ptr = strtok(NULL, separators);
        }
        for (int i = 0; i < num_tokens; i++) {
            char *ptr_ = strchr(tokens[i], '|');
            if (!ptr_ || (strcmp(tokens[i], pip) == 0)) {
                correct_tokens[num_rightleks++] = tokens[i];
                continue;
            }

            ptr_ = tokens[i];
            int first = 0, second = 0;
            for (int k = 0; k < strlen(tokens[i]); k++) {
                if (tokens[i][k] == '"')
                    second = (second + 1) % 2;
                else if (tokens[i][k] == '\'')
                    first = (first + 1) % 2;
                else if (tokens[i][k] == '|') {
                    if ((first && strchr(tokens[i] + k, '\'')) || (second && strchr(tokens[i] + k, '"')))
                        continue;
                    else {
                        correct_tokens[num_rightleks++] = ptr_;
                        correct_tokens[num_rightleks++] = pip;
                        ptr_ = tokens[i] + k + 1;
                        tokens[i][k] = '\0';
                    }

                }
            }
            correct_tokens[num_rightleks++] = ptr_;
        }

        if (parse_commands(&cmd, &num_cmd, num_rightleks, correct_tokens)) {
            printf("Sintax error\n");
            continue;
        }

        execute(cmd, num_cmd);
        free_commands(cmd, num_cmd);
    }
}

int parse_commands(char ****cmds, int *num_cmds, int num_leks, char **leks) {

    int num_cmd = 1;

    for (int k = 0; k < num_leks; k++) {
        if (strcmp(leks[k], pip) == 0)
            num_cmd++;
    }
    *num_cmds = num_cmd;

    char ***cmd = (char ***) malloc(sizeof(char **) * num_cmd);
    for (int k = 0; k < num_cmd; k++)
        cmd[k] = (char **) malloc(sizeof(char *) * num_leks);


    int flag = -1;
    int i = 0;
    num_cmd = 1;

    for (int k = 0; k < num_leks; k++) {
        if (strcmp(leks[k], pip) == 0) {
            if (flag >= 0) {
                cmd[num_cmd - 1][i] = NULL;
                num_cmd++;
                i = 0;
                flag = -1;
                continue;
            } else
                return 1;
        }

        cmd[num_cmd - 1][i++] = leks[k];
        flag++;
    }

    if (flag < 0)
        return 1;

    *cmds = cmd;
    return 0;
}

int free_commands(char ***cmd, int num_cmds) {
    for (int k = 0; k < num_cmds; k++) {
        free(cmd[k]);
    }
    free(cmd);
}

int execute(char ***cmd, int num_cmd) {
    int k = 0;
    int fb[2][2];
    pipe(fb[1]);

    while (k < num_cmd) {
        pipe(fb[k % 2]);

        pid_t pid = fork();
        if (pid == 0) {
            if (k + 1 != num_cmd) {
                close(fb[k % 2][READ]);
                close(STDOUT_FILENO);
                dup(fb[k % 2][WRITE]);
                close(fb[k % 2][WRITE]);
            }

            if (k > 0) {
                close(STDIN_FILENO);
                dup(fb[(k + 1) % 2][READ]);
                close(fb[(k + 1) % 2][READ]);
                close(fb[(k + 1) % 2][WRITE]);
            }

            int er = execvp(cmd[k][0], cmd[k]);

            if (er == -1) {
                perror("Exec error\n");
                for (int i = 0; i < 2; i++) {
                    close(fb[i][0]);
                    close(fb[i][1]);
                }
                exit(0);
            }
        } else {
            close(fb[(k + 1) % 2][READ]);
            close(fb[(k + 1) % 2][WRITE]);
            k++;
        }
    }
    close(fb[(k + 1) % 2][READ]);
    close(fb[(k + 1) % 2][WRITE]);
    for (int i = 0; i < k; i++)
        wait(NULL);
    return 0;
}