#include <sys/statvfs.h>

// Some sample quode to query disk usage on Pi for reporting


/* Any file on the filesystem in question */
char *filename = "/home/somefile.txt";

struct statvfs buf;
if (!statvfs(filename, &buf)) {
unsigned long blksize, blocks, freeblks, disk_size, used, free;

blksize = buf.f_bsize;
blocks = buf.f_blocks;
freeblks = buf.f_bfree;

disk_size = blocks * blksize;
free = freeblks * blksize;
used = disk_size - free;

printf("Disk usage : %lu \t Free space %lu\n", used, free);} else {
printf("Couldn't get file system statistics\n");
}

/////////////////////////////////////////////////


char filename[100];
   cout << "Enter file name to compile ";
   cin.getline(filename, 100);

   // Build command to execute.  For example if the input
   // file name is a.cpp, then str holds "gcc -o a.out a.cpp"
   // Here -o is used to specify executable file name
   string str = "gcc ";
   str = str + " -o a.out " + filename;

   // Convert string to const char * as system requires
   // parameter of type const char *
   const char *command = str.c_str();

   cout << "Compiling file using " << command << endl;
   system(command);

   cout << "\nRunning file ";
   system("./a.out");

   return 0;



   ///////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2) return 1;

    FILE *fd;
    fd = popen(argv[1], "r");
    if (!fd) return 1;

    char   buffer[256];
    size_t chread;
    /* String to store entire command contents in */
    size_t com_alloc = 256;
    size_t comlen   = 0;
    char  *com_out   = malloc(com_alloc);

    /* Use fread so binary data is dealt with correctly */
    while ((chread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
        if (comlen + chread >= com_alloc) {
            com_alloc *= 2;
            com_out = realloc(com_out, com_alloc);
        }
        memmove(com_out + comlen, buffer, chread);
        comlen += chread;
    }

    /* We can now work with the output as we please. Just print
     * out to confirm output is as expected */
    fwrite(com_out, 1, comlen, stdout);
    free(comout);
    pclose(fd);
    return 0;
}
