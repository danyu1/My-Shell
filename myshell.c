#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

//print an error message as specified
void print_error() {
    char error_message[] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}
//trim the leading and trailing whitespace
char *trim_whitespace(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t')
        str++;
    if (*str == '\0')
        return str;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t'))
        end--;
    *(end + 1) = '\0';
    return str;
}

//returns 1 if a string contiains any non-whitespace
int has_non_whitespace(char *str) {
    while (*str) {
        if (*str != ' ' && *str != '\t' && *str != '\n')
            return 1;
        str++;
    }
    return 0;
}

//returns 1 if a string contains any whitespace
int contains_whitespace(char *str) {
    while (*str) {
        if (*str == ' ' || *str == '\t')
            return 1;
        str++;
    }
    return 0;
}

//execute a command with passed in arg
void execute_command(char **args) {
  int status;
    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    } else if (pid == 0) {
        execvp(args[0], args);
        print_error();
        exit(1);
    } else {
        waitpid(pid, &status, 0);
    }
}

//basic redirection implementation
void basic_redirection(char **args, char *outfile) {
    int status;
    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    } else if (pid == 0) {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            print_error();
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execvp(args[0], args);
        print_error();
        exit(1);
    } else {
        waitpid(pid, &status, 0);
    }
}

//seperate implementation for an advanced redirection command
void advanced_redirection(char **args, char *outfile) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        print_error();
        return;
    }
    int status;
    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(args[0], args);
        print_error();
        exit(1);
    } else {
        close(pipefd[1]);
        char buffer[1024];
        ssize_t bytes_read;
        char *new_output = NULL;
        size_t new_output_size = 0;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            char *temp = realloc(new_output, new_output_size + bytes_read);
            if (temp == NULL) {
                free(new_output);
                new_output = NULL;
                break;
            }
            new_output = temp;
            memcpy(new_output + new_output_size, buffer, bytes_read);
            new_output_size += bytes_read;
        }
        close(pipefd[0]);
        waitpid(pid, &status, 0);

        char *old_content = NULL;
        size_t old_size = 0;
        if (access(outfile, F_OK) == 0) {
            //old fd is read only
            int fd_old = open(outfile, O_RDONLY);
            if (fd_old >= 0) {
                char temp_buf[1024];
                ssize_t n;
                while ((n = read(fd_old, temp_buf, sizeof(temp_buf))) > 0) {
                    char *temp2 = realloc(old_content, old_size + n);
                    if (temp2 == NULL) {
                        free(old_content);
                        old_content = NULL;
                        break;
                    }
                    old_content = temp2;
                    memcpy(old_content + old_size, temp_buf, n);
                    old_size += n;
                }
                close(fd_old);
}
        }
        //write only
        int fd_new = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_new < 0) {
            print_error();
            free(new_output);
            free(old_content);
            return;
        }
        if (new_output && new_output_size > 0) {
            if (write(fd_new, new_output, new_output_size) != new_output_size)
                print_error();
        }
        if (old_content && old_size > 0)
            write(fd_new, old_content, old_size);
        close(fd_new);
        free(new_output);
        free(old_content);
    }
}

//process only a single command, redirection?
void process_command(char *command) {
    char *cmd_copy = strdup(command);
    if (cmd_copy == NULL) {
        print_error();
        return;
    }

    char *redir_ptr = strstr(cmd_copy, ">");
    //0 = none, 1 = basic, 2 = advanced
    int redir_type = 0;
    char *cmd_part = NULL;
    char *file_part = NULL;

    if (redir_ptr != NULL) {
        if (*(redir_ptr + 1) == '+') {
            redir_type = 2;
            if (strstr(redir_ptr + 2, ">") != NULL) {
                print_error();
                free(cmd_copy);
                return;
            }
        } else {
            redir_type = 1;
            if (strstr(redir_ptr + 1, ">") != NULL) {
                print_error();
                free(cmd_copy);
                return;
            }
        }
        *redir_ptr = '\0';
        cmd_part = trim_whitespace(cmd_copy);
        if (redir_type == 2)
            file_part = trim_whitespace(redir_ptr + 2);
        else
            file_part = trim_whitespace(redir_ptr + 1);
        if (strlen(file_part) == 0 || contains_whitespace(file_part)) {
            print_error();
            free(cmd_copy);
            return;
        }
    } else {
        cmd_part = trim_whitespace(cmd_copy);
    }

    char *args[128];
    int arg_count = 0;
    char *token = strtok(cmd_part, " \t");
    while (token != NULL && arg_count < 127) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        free(cmd_copy);
        return;
    }

    //implement built in commands
    if (strcmp(args[0], "exit") == 0) {
        if (arg_count != 1 || redir_type != 0) {
            print_error();
            free(cmd_copy);
            return;
        }
        free(cmd_copy);
        exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (redir_type != 0 || arg_count > 2) {
            print_error();
            free(cmd_copy);
            return;
    }
        char *dir = (arg_count == 1) ? getenv("HOME") : args[1];
        if (chdir(dir) != 0) {
            print_error();
        }
        free(cmd_copy);
        return;
    }
    if (strcmp(args[0], "pwd") == 0) {
        if (redir_type != 0 || arg_count != 1) {
            print_error();
            free(cmd_copy);
            return;
        }
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            print_error();
        } else {
            write(STDOUT_FILENO, cwd, strlen(cwd));
            write(STDOUT_FILENO, "\n", 1);
        }
        free(cmd_copy);
        return;
    }
    //check if there needs to be a redirection and call the respective function
    if (redir_type == 1) {
        if (access(file_part, F_OK) == 0) {
            print_error();
            free(cmd_copy);
            return;
        }
        basic_redirection(args, file_part);
    } else if (redir_type == 2) {
        advanced_redirection(args, file_part);
    } else {
        execute_command(args);
    }

    free(cmd_copy);
}

//reads from stdin or batchmode
int main(int argc, char *argv[]) {
    FILE *input_stream = NULL;
    int interactive = 0;

    if (argc > 2) {
        print_error();
        exit(1);
} else if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            print_error();
            exit(1);
        }
    } else {
        input_stream = stdin;
        interactive = 1;
    }

    char line[514];

    while (1) {
        if (interactive)
            write(STDOUT_FILENO, "myshell> ", 9);

        if (fgets(line, sizeof(line), input_stream) == NULL)
            break;

       //if in batch mode echo the line exactly as read
        if (!interactive && has_non_whitespace(line)) {
            write(STDOUT_FILENO, line, strlen(line));
        }

        if (strchr(line, '\n') == NULL) {
            write(STDOUT_FILENO, line, strlen(line));
            print_error();
            int ch;
            //EOF used to deonote end of a file
            while ((ch = fgetc(input_stream)) != '\n' && ch != EOF);
            continue;
        }

        //get rid of the trailing new line
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n')
            line[len-1] = '\0';

        //make sure that the command does not exceed 512 chars
        if (strlen(line) > 512) {
            write(STDOUT_FILENO, line, strlen(line));
            print_error();
            continue;
        }

        //process the line of commands
        char *line_copy = strdup(line);
        if (!line_copy) {
            print_error();
            exit(1);
        }

        char *tokens[128];
        int token_count = 0;
        char *temp_token = strtok(line_copy, ";");
        while (temp_token != NULL && token_count < 128) {
            tokens[token_count] = strdup(temp_token);
            if (!tokens[token_count]) {
                print_error();
                exit(1);
            }
            token_count++;
            temp_token = strtok(NULL, ";");
        }
        free(line_copy);

        for (int i = 0; i < token_count; i++) {
            char *trimmed_token = trim_whitespace(tokens[i]);
            if (strlen(trimmed_token) > 0) {
                process_command(trimmed_token);
            }
            free(tokens[i]);
        }
    }
    exit(0);
}
