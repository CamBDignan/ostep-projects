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

    int rc = fork();

    if (rc == 0) // child process
    {
      char* line2 = strdup(line);
      int count = 0;

      while (strsep(&line, " ") != NULL )
        ++count;

      char* myArgs[count + 1];
      int index = 0;

      while ((*(myArgs + index) = strsep(&line2, " ")) != NULL)
        ++index;

      myArgs[count] = NULL;
      execv(myArgs[0], myArgs);
    }
    else // parent process
    {
      wait(NULL);
    }
  }

  return 0;
}
