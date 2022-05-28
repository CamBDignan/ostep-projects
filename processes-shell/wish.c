#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int numPaths;
char** paths;

void error()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

void MainHelper(char* line3)
{
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
    int numberOfExtraSpaces = line4[0] == ' ' || line4[0] == '\t';
    int indexOfFirstNonSpace = -1;
    int indexOfLastNonSpace = -1;
    if (line4[0] != ' ' && line4[0] != '\t')
    {
      indexOfFirstNonSpace = 0;
      indexOfLastNonSpace = 0;
    }

    for (int i = 1; i < strlen(line4); i++)
    {
      if ((line4[i] == ' ' || line4[i] == '\t') && ((line4[i - 1] == ' ' || line4[i - 1] == '\t')))
        ++numberOfExtraSpaces;

      if (line4[i] != ' ' && line4[i] != '\t')
      {
        indexOfLastNonSpace = i;

        if (indexOfFirstNonSpace == -1)
          indexOfFirstNonSpace = i;
      }
    }

    if (indexOfFirstNonSpace == -1 || strlen(line4) == 0)
      return;

    numberOfExtraSpaces += strlen(line4) - 1 - indexOfLastNonSpace;

    lineLen = strlen(line4) - numberOfExtraSpaces;

    char* line = malloc((lineLen + 1) * sizeof(char));
    line[lineLen] = '\0';
    line[0] = line4[indexOfFirstNonSpace];
    lineIndex = 1;
    for (int i = indexOfFirstNonSpace + 1; i <= indexOfLastNonSpace; i++)
    {
      if ((line4[i] == ' ' || line4[i] == '\t') && ((line4[i - 1] == ' ' || line4[i - 1] == '\t')))
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
    while (strsep(&line, " \t") != NULL )
      ++count;

    char* myArgs[count + 1];
    int index = 0;

    // build arguments array
    while ((*(myArgs + index) = strsep(&line2, " \t")) != NULL)
      ++index;

    myArgs[count] = NULL;

    // check for built-in commands
    if (strcmp(myArgs[0], "exit") == 0)
    {
      if (count > 1)
      {
        error();
        return;
      }

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
      // check number of parallel commands
      int numberOfParallelCommands = 1;
      for (int i = 0; i < count; i++)
      {
        if (strcmp(myArgs[i], "&") == 0)
          ++numberOfParallelCommands;
      }

      int startIndexes[numberOfParallelCommands];
      startIndexes[0] = 0;
      int currIndex = 1;
      for (int i = 0; i < count; i++)
      {
        if (strcmp(myArgs[i], "&") == 0)
        {
          myArgs[i] = NULL;
          startIndexes[currIndex] = i + 1;
          ++currIndex;
        }
      }

      for (int j = 0; j < numberOfParallelCommands; j++)
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
            strcat(fullCommand, myArgs[startIndexes[j]]);

            if (access(fullCommand, X_OK) == 0)
            {
              foundExecutable = 1;
              myArgs[startIndexes[j]] = strdup(fullCommand);
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

          // get count of strings
          count = 0;
          while (myArgs[count + startIndexes[j]] != NULL)
            ++count;

          // create new args array
          char* newArgs[count + 1];
          newArgs[count] = NULL;
          for (int i = 0; i < count; i++)
          {
            newArgs[i] = myArgs[startIndexes[j] + i];
          }

          // check for redirection
          for (int i = 0; i < count; i++)
          {
            if (strcmp(newArgs[i], ">") == 0)
            {
              // check for invalid redirection
              if (count <= 2 || i != count - 2 || strcmp(newArgs[count - 1], ">") == 0)
              {
                error();
                exit(1);
              }

              // ok, we have a valid redirection
              newArgs[i] = NULL;
              int fd = open(newArgs[count - 1], O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
              dup2(fd, 1);
              dup2(fd, 2);
              close(fd);
            }
          }

          if (execv(newArgs[0], newArgs) != 0)
          {
            error();
            exit(1);
          }
        }
      }

      while (wait(NULL) > 0);
    }
}

int main(int argc, char* argv[])
{
  numPaths = 1;
  paths = malloc(numPaths * sizeof(char*));
  paths[0] = strdup("/bin");

  char *line3 = NULL;
  size_t len = 0;
  ssize_t lineSize = 0;

  if (argc == 1) // interactive mode
  {
    while (1)
    {
      printf("wish> ");
      lineSize = getline(&line3, &len, stdin);
      MainHelper(line3);
    }
  }
  else if (argc == 2) // batch mode
  {
    FILE* stream = fopen(argv[1], "r");

    if (stream == NULL)
    {
      error();
      return 1;
    }

    while((lineSize = getline(&line3, &len, stream)) != -1)
    {
      MainHelper(line3);
    }
  }
  else
  {
    error();
    return 1;
  }

  return 0;
}
