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

    int rc = fork();

    if (rc == 0) // child process
    {
      char* myArgs[2];
      myArgs[0] = strdup("/bin/ls");
      myArgs[1] = NULL;
      execv(myArgs[0], myArgs);
    }
    else // parent process
    {
      wait(NULL);
    }
  }

  return 0;
}
