#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <xlocale.h>

#define DEBUG

struct _key_code
{
	char * str;
	int code;
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

static int get_range(char *, int *, int *);
static int check_code(char *);
static void usage(void);
static int get_commands(char *, char *, char *, int*);

int main (int argc, char *argv[])
{
	char * drives;
	char * cmd;
	int command;
	int drive_start;
	int drive_end;
	char type;
	char motor;
	int value;
	
	if (argc < 5)
		usage();
	
	/*
	 * Get range of drives
	 * 1-6 = scan drives 1 to 6
	 */
	drives = argv[1];
	if ((drives[0] == '-') && (drives[1] == 'd')) 
	{
		/*
		 * Get drive range
		 */
		if (get_range(argv[2], &drive_start, &drive_end))
		{
			printf("Connecting to drives %d to %d\n", drive_start, drive_end);
		
			cmd = argv[3];
	
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
				printf("Setting command %s (%d) OK\n", cmd, command);
			}
			else
			{
				printf("%s Command not found\n", cmd);
				usage();
			}
			
			/*
			 * Get rest of instruction
			 */
			cmd = argv[4];
			get_commands(cmd, &type, &motor, &value);
			
			while (drive_start <= drive_end)
			{
				printf("Address = %d\n", drive_start);
				printf("Command = %d\n", command);
				printf("Type = %d\n", type);
				printf("Motor = %d\n", motor);
				printf("Value = %d\n", value);
				printf("\n");
				
				drive_start++;
			}
		}
	}
	
	return 0;
}


static int
get_commands(char * str, char * type, char * motor, int * value)
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
get_range(char * str, int * first, int * last)
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

static
void usage(void)
{
	printf("CMD Error");
	exit(1);
}