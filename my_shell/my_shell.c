#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define ctrl(x) ((x)&0x1f)

char *username = NULL;
int status;
char cwd[PATH_MAX];
char *args = NULL;
char *bin_path = NULL;
int bufsize = LSH_TOK_BUFSIZE, position;
int c_flag = 1;
int r_flag = 0;
char *input = NULL;
char *output = NULL;

void type_prompt()
{
    username = (char *)malloc(10 * sizeof(char));
    username = getlogin();
    printf("[cs345sh][%s]", username);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("[%s]~$", cwd);
    }
    else
    {
        perror("getcwd() error");
    }
}

void read_command(char *command, char *tokens[])
{
    char *command_copy = NULL;
    command_copy = (char *)malloc(100 * sizeof(char));

    char *token;
    token = (char *)malloc(100 * sizeof(char));

    position = 0;
    getline(&command, &bufsize, stdin);
    strcpy(command_copy, command);
    int com_size = strlen(command);

    token = strtok(command_copy, LSH_TOK_DELIM);

    while (token != NULL)
    {
        tokens[position] = strdup(token);
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            //tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    char *bin = "/bin/";
    char *temp;
    temp = (char *)malloc(1 + sizeof(tokens[0]));
    temp = tokens[0];
    bin_path = (char *)malloc(256 * sizeof(char));

    if (tokens[0] != NULL)
    {
        strcpy(bin_path, bin);
        strcat(bin_path, temp);
        if (!strcmp(tokens[0], "cd"))
        {
            if (tokens[1] != NULL)
            {
                chdir(strdup(tokens[1]));
            }
            else
            {
                printf("Please enter a directory path.\n");
            }
        }
        else if (!strcmp(tokens[0], "exit"))
        {
            exit(0);
        }
    }
    else
    {
        strcpy(bin_path, bin);
    }
    //check if input has pipeline
    int i;
    for (i = 0; i < position; i++)
    {
        if (!strcmp(tokens[i], ">"))
        {
            c_flag = 2;
        }
        else if (!strcmp(tokens[i], "|"))
        {
            c_flag = 3;
            r_flag = 1;
            if (tokens[i + 1] != NULL)
            {
                input = tokens[i + 1];
            }
        }
        else if (!strcmp(tokens[i], "||"))
        {
            c_flag = 3;
            r_flag = 2;
            if (tokens[i + 1] != NULL)
            {
                output = tokens[i + 1];
            }
        }
        else if (!strcmp(tokens[i], "|||"))
        {
            c_flag = 3;
            r_flag = 3;
            if (tokens[i + 1] != NULL)
            {
                output = tokens[i + 1];
            }
        }
    }
}

void exec_args(char *tokens[])
{
    pid_t cpid;
    cpid = fork();
    if (cpid == 0)
    {
        //child
        execvp(bin_path, tokens); /* execute command "/bin/ls", {"ls","-l", NULL}*/
        free(*tokens);
        exit(EXIT_SUCCESS);
    }
    else if (cpid < 0)
    {
        //error
    }
    else
    {
        //parent
        waitpid(cpid, &status, 0); /* wait for child to exit */
    }
}

void exec_pipe(char *tokens[])
{
    int status;
    int pipefd[2];
    pid_t cpid;
    char *cmd[3] = {"ls", NULL, NULL};
    char *cmd2[3] = {"grep", ".c", NULL};
    char *l_c[256];
    char *r_c[256];
    int i, j = 0;
    for (i = 0; i < position; i++)
    {
        if (strcmp(tokens[i], ">"))
        {
            l_c[i] = tokens[i];
        }
        else
        {
            i++;
            for (i; i < position; i++)
            {
                r_c[j] = tokens[i];
                j++;
            }
        }
    }

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }
    cpid = fork();

    if (cpid == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (cpid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execvp(l_c[0], l_c);
    }
    else
    {
        cpid = fork();
        if (cpid == 0)
        {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(r_c[0], r_c);
        }
        else
        {
            int status;
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(cpid, &status, 0);
        }
    }

    /*if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    cpid = fork();
    if (cpid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (cpid == 0)
    {                     //Child reads from pipe 
        close(pipefd[0]); // Close unused write end 
        dup2(pipefd[1], STDOUT_FILENO);
        execvp(l_c[0], l_c);
        exit(EXIT_SUCCESS);
    }
    else
    {
        close(pipefd[1]); // Close unused read end 
        dup2(pipefd[0], STDIN_FILENO);
        execvp(r_c[0], r_c);
        waitpid(cpid, &status, 0);
    }*/
}

void exe_redirect(char *tokens[])
{
    char *text;
    FILE *fptr;
    int in, out;
    int i = 0;
    pid_t cpid;
    cpid = fork();
    if (cpid == 0)
    {
        //child
        if (r_flag == 1)
        {
            char *temp[25256];
            in = open(input, O_RDONLY);
            dup2(in, 0);
            close(in);
            for (i = 0; i < position; i++)
            {
                if (!strcmp(tokens[i], "|"))
                {
                    break;
                }
                else
                {
                    temp[i] = tokens[i];
                }
            }
            temp[i + 1] = NULL;
            execvp(temp[0], temp);
        }
        else if (r_flag == 2)
        {
            char *temp[25256];
            out = open(output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(out, 1);
            close(out);
            for (i = 0; i < position; i++)
            {
                if (!strcmp(tokens[i], "||"))
                {
                    break;
                }
                else
                {
                    temp[i] = tokens[i];
                }
            }
            temp[i + 1] = NULL;
            execvp(temp[0], temp);
        }
        else if (r_flag == 3)
        {
            char *temp[25256];
            out = open(output, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(out, 1);
            close(out);
            for (i = 0; i < position; i++)
            {
                if (!strcmp(tokens[i], "|||"))
                {
                    break;
                }
                else
                {
                    temp[i] = tokens[i];
                }
            }
            temp[i + 1] = NULL;
            execvp(temp[0], temp);
        }
        exit(EXIT_SUCCESS);
    }
    else if (cpid < 0)
    {
        //error
    }
    else
    {
        //parent
        waitpid(cpid, &status, 0); /* wait for child to exit */
    }
}

void main()
{
    //char command[MAXN];
    //char *tokens[MAXARGS] = char**
    while (1)
    {

        c_flag = 1;
        r_flag - 0;
        char *tokens[25256];
        char *command = NULL;
        command = (char *)malloc(100 * sizeof(char));

        type_prompt();                 /* display prompt on the screen */
        read_command(command, tokens); /* read input from terminal */
        /* repeat forever */
        if (c_flag == 1)
        {
            exec_args(tokens);
        }
        else if (c_flag == 2)
        {
            exec_pipe(tokens);
        }
        else if (c_flag == 3)
        {
            exe_redirect(tokens);
        }       
    }
    printf("\n");
}
