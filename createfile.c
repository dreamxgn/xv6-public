#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int fd, ret;
    const char *data = "hello world";

    unlink("file");

    fd = open("file", O_CREATE | O_RDWR);

    if (fd < 0)
    {
        printf(1, "createfile: cannot create file");
        return exit();
    }

    ret = write(fd, data, strlen(data) + 1);
    if (ret < 0)
    {
        printf(1, "createfile: cannot write to file");
        return exit();
    }

    ret = close(fd);
    if (ret < 0)
    {
        printf(1, "createfile: cannot close file");
        return exit();
    }
    printf(1, "createfile: file created successfully");
    return exit();
}