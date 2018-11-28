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

const char exit_flag[] = "exit";
const char prefix[] = "@ ";
const char pipe_sep[] = "|";
const char separators[] = " \n";

int parse_commands(char ****cmds, int *num_cmds, int num_leks, char **leks);

int free_commands(char ***cmd, int num_cmds);

int execute(char ***cmd, int num_cmd);


int main() {
    char ***cmd = NULL;
    int num_cmd = 0;
    char buffer[MAX_CMD_LEN];
    char *tokens[MAX_CMD_LEN];
    char *correct_tokens[MAX_CMD_LEN];
    char *ptr = NULL;
    int num_tokens, num_correct_tokens;


    while (1) {
        cmd = NULL;
        num_cmd = 1;
        num_tokens = 0, num_correct_tokens = 0;

        fputs(prefix, stdout);

        // Read command
        if (fgets(buffer, MAX_CMD_LEN, stdin) == NULL)
            return 0;

        // Check empty line
        if ((ptr = strtok(buffer, separators)) == NULL)
            continue;

        //Finish program
        if (strcmp(ptr, exit_flag) == 0)
            return 0;

        // Split buffer into tokens
        while (ptr != NULL) {
            tokens[num_tokens++] = ptr;
            ptr = strtok(NULL, separators);
        }
        for (int i = 0; i < num_tokens; i++) {
            char *ptr_ = strchr(tokens[i], '|');
            if (!ptr_ || (strcmp(tokens[i], pipe_sep) == 0)) {
                correct_tokens[num_correct_tokens++] = tokens[i];
                continue;
            }

            ptr_ = tokens[i];
            int quote_first_flag = 0, quote_second_flag = 0;
            for (int k = 0; k < strlen(tokens[i]); k++) {
                if (tokens[i][k] == '"')
                    quote_second_flag = !quote_second_flag;
                else if (tokens[i][k] == '\'')
                    quote_first_flag = !quote_first_flag;
                else if (tokens[i][k] == '|') {
                    if ((quote_first_flag && strchr(tokens[i] + k, '\'')) || (quote_second_flag && strchr(tokens[i] + k, '"')))
                        continue;
                    else {
                        correct_tokens[num_correct_tokens++] = ptr_;
                        correct_tokens[num_correct_tokens++] = pipe_sep;
                        ptr_ = tokens[i] + k + 1;
                        tokens[i][k] = '\0';
                    }

                }
            }
            correct_tokens[num_correct_tokens++] = ptr_;
        }
        printf("correct tokens\n");
        for(int i = 0; i < num_correct_tokens; i++)
            printf("%s\n", correct_tokens[i]);

        if (parse_commands(&cmd, &num_cmd, num_correct_tokens, correct_tokens)) {
            printf("Syntax error\n");
            continue;
        }

        execute(cmd, num_cmd);
        free_commands(cmd, num_cmd);
    }
}

int parse_commands(char ****cmds, int *num_cmds, int num_leks, char **leks) {

    int num_cmd = 1;

    for (int k = 0; k < num_leks; k++) {
        if (strcmp(leks[k], pipe_sep) == 0)
            num_cmd++;
    }
    *num_cmds = num_cmd;

    char ***cmd = (char ***) calloc(sizeof(char **), num_cmd);
    for (int k = 0; k < num_cmd; k++)
        cmd[k] = (char **) calloc(sizeof(char *), num_leks);


    int flag = -1;
    int i = 0;
    num_cmd = 1;

    for (int k = 0; k < num_leks; k++) {
        if (strcmp(leks[k], pipe_sep) == 0) {
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
    printf("cmd:\n");
    for (int i = 0; i < num_cmd; i++)
        printf("%s, %s\n", cmd[i][0], cmd[i][1]);
    int k = 0;
    int fb[2][2];
    pipe(fb[1]);
    int pipe_index = 0;

    while (k < num_cmd) {
        pipe_index = k % 2;
        pipe(fb[pipe_index]);

        pid_t pid = fork();
        if (pid == 0) {
            if (k + 1 != num_cmd) {
                close(fb[pipe_index][READ]);
                close(STDOUT_FILENO);
                dup(fb[pipe_index][WRITE]);
                close(fb[pipe_index][WRITE]);
            }

            if (k > 0) {
                close(STDIN_FILENO);
                dup(fb[!pipe_index][READ]);
                close(fb[!pipe_index][READ]);
                close(fb[!pipe_index][WRITE]);
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
            close(fb[!pipe_index][READ]);
            close(fb[!pipe_index][WRITE]);
            k++;
        }
    }
    close(fb[!pipe_index][READ]);
    close(fb[!pipe_index][WRITE]);
    for (int i = 0; i < k; i++)
        wait(NULL);
    return 0;
}

