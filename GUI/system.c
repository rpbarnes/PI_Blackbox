#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void Parse_Line(char* pLine)
{
        char* token = strtok(strdup(pLine), " ");
        //++line_index;
        //printf("\t%d. %s\n", line_index, buffer);
	printf("%s\n", token);
        while(token)
        {
                printf("\t[ %s]\n", token);
                token  = strtok(NULL, " ");
        }
}


int main(int argc, char **argv)
{
    //if (argc < 2) return 1;

    FILE *fd;
    char command[10] = "df";
    //fd = popen(argv[1], "r");
    fd = popen(command, "r");
    if (!fd) return 1;

    printf("%s   FD: %d\n", command, *fd);

    char   buffer[256];
    char *  line;
    size_t chread;
    /* String to store entire command response in */
    size_t com_alloc = 256;
    size_t comlen   = 0;
    char  *com_out   = malloc(com_alloc);

    char data[512];

    if (fd) {
     while (!feof(fd))
     if (fgets(buffer, sizeof(buffer), fd) != NULL) strcat(data,buffer);
     pclose(fd);
     }



    int line_index = 0;
    /* Use fread so binary data is dealt with correctly */
    while ((chread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
        if (comlen + chread >= com_alloc-1) // Allocate more memory for reading if necessary
  	    {
            com_alloc *= 2;
            com_out = realloc(com_out, com_alloc);
        }



        memmove(com_out + comlen, buffer, chread);
        comlen += chread;
    }

        line = strtok(strdup(com_out), "\n");
        ++line_index;
        //printf("\t%d. %s\n", line_index, buffer);
        while(line)
        {
                printf("\t%d.  %s\n", line_index, line);
                ++line_index;
		            char* parseline = strdup(line);
		            //Parse_Line(parseline);
                line  = strtok(NULL, "\n");
        }


    /* We can now work with the output as we please. Just print
     * out to confirm output is as expected */
    fwrite(com_out, 1, comlen, stdout);
    free(com_out);
    pclose(fd);
    return 0;
}
