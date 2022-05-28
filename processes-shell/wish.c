#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

void error()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char* argv[])
{
  int numPaths = 1;
  char** paths = malloc(numPaths * sizeof(char*));
  paths[0] = strdup("/bin");

  while (1)
  {
    printf("wish> ");
    char *line3 = NULL;
    size_t len = 0;
    ssize_t lineSize = 0;
    lineSize = getline(&line3, &len, stdin);
    line3[strcspn(line3, "\n")] = 0; // remove newline char

    // add space around > and &
    int numberOfOperators = 0;
    for (int i = 0; i < strlen(line3); i++)
    {
      if (line3[i] == '>' || line3[i] == '&')
        ++numberOfOperators;
    }

    int lineLen = strlen(line3) + 2 * numberOfOperators;
    char* line4 = malloc((lineLen + 1) * sizeof(char));
    line4[lineLen] = '\0';
    int lineIndex = 0;
    for (int i = 0; i < strlen(line3); i++)
    {
      if (line3[i] == '>' || line3[i] == '&')
      {
        line4[lineIndex] = ' ';
        line4[lineIndex + 1] = line3[i];
        line4[lineIndex + 2] = ' ';
        lineIndex += 3;
      }
      else
      {
        line4[lineIndex] = line3[i];
        ++lineIndex;
      }
    }

    // remove extra spaces
    int numberOfExtraSpaces = 0;
    for (int i = 1; i < strlen(line4); i++)
    {
      if (line4[i] == ' ' && line4[i - 1] == ' ')
        ++numberOfExtraSpaces;
    }

    lineLen = strlen(line4) - numberOfExtraSpaces;
    char* line = malloc((lineLen + 1) * sizeof(char));
    line[lineLen] = '\0';
    line[0] = line4[0];
    lineIndex = 1;
    for (int i = 1; i < strlen(line4); i++)
    {
      if (line4[i] == ' ' && line4[i - 1] == ' ')
      {
        continue;
      }
      else
      {
        line[lineIndex] = line4[i];
        ++lineIndex;
      }
    }

    char* line2 = strdup(line);
    int count = 0;

    // get number of strings passed in by user
    while (strsep(&line, " ") != NULL )
      ++count;

    char* myArgs[count + 1];
    int index = 0;

    // build arguments array
    while ((*(myArgs + index) = strsep(&line2, " ")) != NULL)
      ++index;

    myArgs[count] = NULL;

    // check for built-in commands
    if (strcmp(myArgs[0], "exit") == 0)
    {
      exit(0);
    }
    else if (strcmp(myArgs[0], "cd") == 0)
    {
      if (count != 2 || chdir(myArgs[1]) != 0)
        error();
    }
    else if (strcmp(myArgs[0], "path") == 0)
    {
      numPaths = count - 1;
      paths = malloc(numPaths * sizeof(char*));

      for (int i = 1; i < count; i++)
      {
        paths[i - 1] = myArgs[i];
      }
    }
    else
    {
      // if not a built-in, call execv on child process
      int rc = fork();

      if (rc == 0) // child process
      {
        // chech if we can find executable
        int foundExecutable = 0;
        char fullCommand[1024];

        for (int i = 0; i < numPaths; i++)
        {
          strcpy(fullCommand, paths[i]);
          strcat(fullCommand, "/");
          strcat(fullCommand, myArgs[0]);

          if (access(fullCommand, X_OK) == 0)
          {
            foundExecutable = 1;
            myArgs[0] = strdup(fullCommand);
            break;
          }

          for (int i = 0; i < 1023; i++)
            fullCommand[i] = 0;
        }

        if (!foundExecutable)
        {
          error();
          exit(1);
        }

        // check for redirection
        for (int i = 0; i < count; i++)
        {
          if (strcmp(myArgs[i], ">") == 0)
          {
            // check for invalid redirection
            if (i != count - 2 || strcmp(myArgs[count - 1], ">") == 0)
            {
              error();
              exit(1);
            }

            // ok, we have a valid redirection
            myArgs[i] = NULL;
            int fd = open(myArgs[count - 1], O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
          }
        }

        if (execv(myArgs[0], myArgs) != 0)
        {
          error();
          exit(1);
        }
      }
      else // parent process
      {
        wait(NULL);
      }
    }
  }

  return 0;
}
