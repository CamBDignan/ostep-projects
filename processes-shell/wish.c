#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int numPaths;
char** paths;

void freePaths()
{
  for (int i = 0; i < numPaths; i++)
    free(paths[i]);

  free(paths);
}

void error()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

// 0 = continue, 1 = exit successfully, 2 = exit with error
int MainHelper(char* line3)
{
    // line3: original string from getline
    line3[strcspn(line3, "\n")] = 0; // remove newline char

    // add space around > and &
    int numberOfOperators = 0;
    for (int i = 0; i < strlen(line3); i++)
    {
      if (line3[i] == '>' || line3[i] == '&')
        ++numberOfOperators;
    }

    int lineLen = strlen(line3) + 2 * numberOfOperators;
    // line4: newly allocated string with spaces around > and &
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

    // free(line3); // done with line3 at this point

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
    {
      free(line4);
      return 0;
    }

    int len4 = strlen(line4);
    if (indexOfLastNonSpace < len4 - 1)
      ++numberOfExtraSpaces;

    lineLen = strlen(line4) - numberOfExtraSpaces;
    // line: newly allocated string with extra spaces removed
    char* line = malloc((lineLen + 1) * sizeof(char));
    char* line5 = line;
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

    free(line4); // done with line4 at this point

    // TODO: line2: newly allocated exact copy of line
    char* line2 = strdup(line);
    char* line6 = line2;
    int count = 0;

    // get number of strings passed in by user
    while (strsep(&line, " \t") != NULL )
      ++count;

    free(line5); // done with line at this point

    char* myArgs[count + 1];
    int index = 0;

    // build arguments array
    char* temp;
    while ((temp = strsep(&line2, " \t")) != NULL)
    {
      myArgs[index] = strdup(temp);
      ++index;
    }

    free(line6);

    myArgs[count] = NULL;

    // check for built-in commands
    if (strcmp(myArgs[0], "exit") == 0)
    {
      if (count > 1)
      {
        error();
	for (int i = 0; i < count; i++)
        {
//          printf("freeing %s\n", myArgs[i]);
          free(myArgs[i]);
	}

        return 0;
      }

      for (int i = 0; i < count; i++)
      {
//        printf("freeing %s\n", myArgs[i]);
        free(myArgs[i]);
      }

      return 1;
    }
    else if (strcmp(myArgs[0], "cd") == 0)
    {
      if (count != 2 || chdir(myArgs[1]) != 0)
        error();

      for (int i = 0; i < count; i++)
      {
//	printf("freeing %s\n", myArgs[i]);
        free(myArgs[i]);
      }
    }
    else if (strcmp(myArgs[0], "path") == 0)
    {
      freePaths();
      numPaths = count - 1;
      paths = malloc(numPaths * sizeof(char*));

      for (int i = 1; i < count; i++)
      {
        paths[i - 1] = strdup(myArgs[i]);
      }

      for (int i = 0; i < count; i++)
      {
//        printf("freeing %s\n", myArgs[i]);
        free(myArgs[i]);
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
//          printf("freeing %s\n", myArgs[i]);
          free(myArgs[i]);
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

          if (myArgs[startIndexes[j]] == NULL)
          {
            for (int i = 0; i < count; i++)
            {
              if (myArgs[i] != NULL)
              {
//                printf("freeing %s\n", myArgs[i]);
                free(myArgs[i]);
              }
            }

            return 0;
	  }

          for (int i = 0; i < numPaths; i++)
          {
            strcpy(fullCommand, paths[i]);
            strcat(fullCommand, "/");
            strcat(fullCommand, myArgs[startIndexes[j]]);

            if (access(fullCommand, X_OK) == 0)
            {
              foundExecutable = 1;
              free(myArgs[startIndexes[j]]);
	      myArgs[startIndexes[j]] = strdup(fullCommand);
              break;
            }

            for (int i = 0; i < 1023; i++)
              fullCommand[i] = 0;
          }

          if (!foundExecutable)
          {
            error();
	    for (int i = 0; i < count; i++)
            {
              if (myArgs[i] != NULL)
              {
//                printf("freeing %s\n", myArgs[i]);
                free(myArgs[i]);
              }
            }

            return 2;
          }

          // get count of strings
          int newCount = 0;
          while (myArgs[newCount + startIndexes[j]] != NULL)
            ++newCount;

          // create new args array
          char* newArgs[newCount + 1];
          newArgs[newCount] = NULL;
          for (int i = 0; i < newCount; i++)
          {
            newArgs[i] = myArgs[startIndexes[j] + i];
          }

          // check for redirection
          for (int i = 0; i < newCount; i++)
          {
            if (strcmp(newArgs[i], ">") == 0)
            {
              // check for invalid redirection
              if (newCount <= 2 || i != newCount - 2 || strcmp(newArgs[newCount - 1], ">") == 0)
              {
                error();
		for (int i = 0; i < count; i++)
                {
                  if (myArgs[i] != NULL)
                  {
//                    printf("freeing %s\n", myArgs[i]);
                    free(myArgs[i]);
		  }
                }
                return 2;
              }

              // ok, we have a valid redirection
              newArgs[i] = NULL;
              int fd = open(newArgs[newCount - 1], O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
              dup2(fd, 1);
              dup2(fd, 2);
              close(fd);
            }
          }

          if (execv(newArgs[0], newArgs) != 0)
          {
            error();
	    for (int i = 0; i < count; i++)
            {
              if (myArgs[i] != NULL)
              {
//                printf("freeing %s\n", myArgs[i]);
                free(myArgs[i]);
              }
            }

            return 2;
          }
        }
      }

      while (wait(NULL) > 0);
      for (int i = 0; i < count; i++)
      {
        if (myArgs[i] != NULL)
        {
//          printf("freeing %s\n", myArgs[i]);
          free(myArgs[i]);
	}
      }
    }

    return 0;
}

int main(int argc, char* argv[])
{
  numPaths = 1;
  paths = malloc(numPaths * sizeof(char*));
  paths[0] = strdup("/bin");

  char *line3 = NULL;
  size_t len = 0;
  ssize_t lineSize = 0;
  int rc = 0;

  if (argc == 1) // interactive mode
  {
    while (1)
    {
      printf("wish> ");
      lineSize = getline(&line3, &len, stdin);
      rc = MainHelper(line3);
      free(line3);
      line3 = NULL;

      if (rc > 0)
      {
        freePaths();
        exit(rc - 1);
      }
    }
  }
  else if (argc == 2) // batch mode
  {
    FILE* stream = fopen(argv[1], "r");

    if (stream == NULL)
    {
      freePaths();
      error();
      return 1;
    }

    while((lineSize = getline(&line3, &len, stream)) != -1)
    {
      rc = MainHelper(line3);
      free(line3);
      line3 = NULL;

      if (rc > 0)
      {
        freePaths();
        fclose(stream);
        exit(rc - 1);
      }
    }

    if (line3 != NULL)
      free(line3);

    fclose(stream);
  }
  else
  {
    freePaths();
    error();
    return 1;
  }

  freePaths();
  return 0;
}
