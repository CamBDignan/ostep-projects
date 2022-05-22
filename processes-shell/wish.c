#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
  while (1)
  {
    printf("wish> ");
    char *line = NULL;
    size_t len = 0;
    ssize_t lineSize = 0;
    lineSize = getline(&line, &len, stdin);
    line[strcspn(line, "\n")] = 0; // remove newline char

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
      exit(0);

    // if not a built-in, call execv on child process
    int rc = fork();

    if (rc == 0) // child process
    {
      execv(myArgs[0], myArgs);
    }
    else // parent process
    {
      wait(NULL);
    }
  }

  return 0;
}
