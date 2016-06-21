#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main()
{
    char msg[] = "hello world/wangmh";
    char *hello = strdup(msg);
    char *p;
    char *token;
    char *saveptr1 = NULL;

    for (p = hello; ;p = NULL)
    {
        token = strtok_r(p, "/", &saveptr1);
        if (NULL == token)
        {
            break;
        }
        printf("%s\n", token);
    }

    free(hello);
    return 0;
}


