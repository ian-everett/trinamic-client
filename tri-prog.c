#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <xlocale.h>

#define DEBUG

enum {
    STANDARD,
    FILEMODE
};

struct _key_code
{
	char * str;
	unsigned char code;
};

struct _key_code codes[] = {
	{ "ROR", 1 },
	{ "ROL", 2 },
	{ "MST", 3 },
	{ "MVP", 4 },
	{ "SAP", 5 },
	{ "GAP", 6 },
	{ "STAP", 7 },
	{ "RSAP", 8 },
	{ "SGP", 9 },
	{ "GGP", 10 },
	{ "STGP", 11 },
	{ "RSGP", 12 },
	{ "SIO", 14 },
	{ "GIO", 15 },
	{ NULL , 0 }
};

static int get_range(char *, unsigned char *, unsigned char *);
static int check_code(char *);
static void usage(void);
static int get_commands(char *, unsigned char *, unsigned char *, int *);
static int do_command(int, int, char*, char*);
static int SendMessage(unsigned char, unsigned char, unsigned char, unsigned char, int);

int main (int argc, char *argv[])
{
	unsigned char drive_start;
	unsigned char drive_end;
    int mode = STANDARD;
    FILE * fptr;
    char buffer[32];

    if (strstr(argv[0], "tri-prog-file"))
    {
        mode = FILEMODE;
        if (argc < 4)
            usage();
        
    }
    else if (argc < 5)
        usage();
	
	/*
	 * Get range of drives
	 * 1-6 = scan drives 1 to 6
	 */
	if ((argv[1][0] == '-') && (argv[1][1] == 'd'))
	{
		/*
		 * Get drive range
		 */
		if (get_range(argv[2], &drive_start, &drive_end))
		{
			printf("Connecting to drives %d to %d\n", drive_start, drive_end);
		
            if (mode == FILEMODE)
            {
                fptr = fopen(argv[3], "r");
                if (fptr)
                {
                    printf("***** Running commands in %s file *****\n", argv[3]);
                    while(fgets(&buffer[0], 32, fptr))
                    {
                        /*
                         * Read command line commands and execute
                         */
                        do_command(drive_start, drive_end, buffer, NULL);
                    }
                    fclose(fptr);
                }
                else
                {
                    printf("Could not open file %s\n", argv[3]);
                }
            }
            else
            {
                /*
                 * Read comman line commands and execute
                 */
                do_command(drive_start, drive_end, argv[3], argv[4]);
            }
		}
	}
	
	return 0;
}


static int
do_command(int drive_start, int drive_end, char *cmd, char *cmd2)
{
    unsigned char command;
    unsigned char type;
    unsigned char motor;
    int value;
    char *c;
    int res;
    
    if (cmd2 == NULL)
    {
        c = strtok(cmd, " ");
        if (c)
            cmd2 = strtok(NULL, " ");
    }
    
    /*
     * Check command, if not found see
     * if valid number else fail
     */
    command = check_code(cmd);
    if (command == 0)
        command = atoi(cmd);
    if (command > 0)
    {
        /*
         * Command OK
         */
        printf("Trying command %s (%d) %s", cmd, command, cmd2);
    }
    else
    {
        printf("%s command not found\n", cmd);
    }
    
    /*
     * Get rest of instruction
     */
    get_commands(cmd2, &type, &motor, &value);
    
    while (drive_start <= drive_end)
    {
        printf("Drive %d:", drive_start);
        /*
         * Send command and wait reply
         */
        res = SendMessage(drive_start, command, type, motor, value);
        switch(res)
        {
            case 100:
                printf(" Answered with OK\n");
                break;
                
                
            default:
                printf(" Answered with Error of %d\n", res);
                break;
        }

        

        
        drive_start++;
    }
    
    return 0;
}


static int
get_commands(char * str, unsigned char * type, unsigned char * motor, int * value)
{
	char *c;

	*motor = *type = *value = 0;
    c = strchr(str, ',');
	if (c)
	{
		*c++ = '\0';
		*type = atoi(str);
		str = c;
	}
	c = strchr(str, ',');
	if (c)
	{
		*c++ = '\0';
		*motor = atoi(str);
		str = c;
	}
	*value = atoi(str);

	return 0;
}


static int
get_range(char * str, unsigned char * first, unsigned char * last)
{
	int res,retval,swap;
	char * c;
	
	retval = 1;
	*last = *first = 0;
	
	/*
	 * Look for '-' and get last drive
	 */
	c = strchr(str, '-');
	if (c++)
	{
		res = atoi(c);
		if (res > 0)
			*last = res;

		res = atoi(str);
		if (res > 0)
			*first = res;
		else
			retval = 0;
	}
	else
	{
		res = atoi(str);
		if (res > 0)
			*last = *first = res;
		else
			retval = 0;
	}

	/*
	 * Swap if nec
	 */
	if (*last < *first)
	{
		swap = *last;
		*last = *first;
		*first = swap;
	}
	 
	return retval;
}



static int
check_code(char * str)
{
	int i;
	struct _key_code * p = &codes[0];
	
	for (i = 0; p->str; ++i, p++)
	{
		if (strcmp(str, p->str) == 0)
			return p->code;
	}
	
	return 0;
}


static int
SendMessage(unsigned char drive, unsigned char command, unsigned char type, unsigned char motor, int value)
{
    
#ifdef DEBUG
    printf("\n");
    printf("Address = %d\n", drive);
    printf("Command = %d\n", command);
    printf("Type = %d\n", type);
    printf("Bank = %d\n", motor);
    printf("Value = %d\n", value);
    printf("\n");
#endif
    
    
    return 100;
}


static
void usage(void)
{
	printf("CMD Error");
	exit(1);
}