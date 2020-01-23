#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

//#define DEBUG

#define PACKET_SIZE 9

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

static int send_message(unsigned char *);
static void build_message(unsigned char, unsigned char, unsigned char, unsigned char, int);
static int parse_message(unsigned char*);
static int connect_drive(void);
static int get_range(char *, unsigned char *, unsigned char *);
static int check_code(char *);
static void usage(void);
static int get_commands(char *, unsigned char *, unsigned char *, int *);
static int do_command(int, int, char*, char*);

static int fd;
static unsigned char txbuffer[PACKET_SIZE];
static unsigned char rxbuffer[PACKET_SIZE];
static int mode = STANDARD;


int main (int argc, char *argv[])
{
	unsigned char drive_start;
	unsigned char drive_end;
	FILE * fptr;
	char buffer[100];

	if (connect_drive())
	{
		printf("Could not connect to drive\n");
		return 1;
	}

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
					while(fgets(&buffer[0], 100, fptr))
					{
						if ((buffer[0] != '\n') && (buffer[0] != '#'))
						{
							/*
							 * Read command line commands and execute
							 */
							do_command(drive_start, drive_end, buffer, NULL);
						}
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
connect_drive(void)
{
	struct termios ttydata;					// Structure for port settings

	fd = open("/dev/stepper-drive", O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd > 0)
	{
		/*
		 * Setup Comms
		 */
		tcgetattr(fd, &ttydata);
		//cfsetospeed(&ttydata, B115200);
		//cfsetospeed(&ttydata, B9600);
		cfsetospeed(&ttydata, B38400);
		cfsetispeed(&ttydata, 0);
		cfmakeraw(&ttydata);
		tcflush(fd, TCIOFLUSH);
		tcsetattr(fd, TCSANOW, &ttydata);

		return 0;
	}

	return 1;
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
		{
			cmd2 = strtok(NULL, " ");

			c = strchr(cmd2, '\n');
			if (c)
				*c = '\0';
		}
		else
		{
			printf("Incorrect command structure\n");
			return 1;
		}
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
		printf("Trying command %s (%d) %s\n", cmd, command, cmd2);
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
		printf("\tDrive %d:", drive_start);
		/*
		 * Send command and wait reply
		 */
		build_message(drive_start, command, type, motor, value);
		if (send_message(txbuffer))
		{
			res = parse_message(rxbuffer);
			switch(res)
			{
				case 0:
					printf(" CRC error\n");
					break;

				case 100:
					printf(" Answered with OK\n");
					break;

				case 101:
					printf(" Command loaded into TMCL program EEPROM\n");
					break;

				case 1:
					printf(" Wrong Checksum\n");
					break;

				case 2:
					printf(" Invalid command\n");
					break;

				case 3:
					printf(" Wrong type\n");
					break;

				case 4:
					printf(" Invalid value\n");
					break;

				case 5:
					printf(" EEPROM locked\n");
					break;

				case 6:
					printf(" Command not available\n");
					break;

				default:
					printf(" Answered with Error of %d\n", res);
					break;
			}
		}
		else
			printf(" Did not answer!!\n");

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


static void
build_message(unsigned char Address, unsigned char Command, unsigned char Type, unsigned char Motor, int Value)
{
	int i;

	txbuffer[0] = Address;
	txbuffer[1] = Command;
	txbuffer[2] = Type;
	txbuffer[3] = Motor;
	txbuffer[4] = Value >> 24;
	txbuffer[5] = Value >> 16;
	txbuffer[6] = Value >> 8;
	txbuffer[7] = Value & 0xff;
	txbuffer[8] = 0;

	for (i = 0; i < PACKET_SIZE-1; i++)
		txbuffer[8] += txbuffer[i];
}


static int
parse_message(unsigned char * mesg)
{
	unsigned char chksum = 0;
	int i;

	for (i = 0; i < PACKET_SIZE-1; i++)
		chksum += mesg[i];

	if (chksum != mesg[8])
		return 0;

#ifdef DEBUG
	printf("Reply Address = %d\n", mesg[0]);
	printf("Module Address = %d\n", mesg[1]);
	printf("Status = %d\n", mesg[2]);
	printf("Command number %d\n", mesg[3]);
#endif

	printf(" Value = %d ", (mesg[4] << 24) | (mesg[5] << 16) | (mesg[6] << 8) | mesg[7]);

	return (mesg[2]);
}


static int
send_message(unsigned char * mesg)
{
	int cnt, i, retval;
	fd_set rfds;
	unsigned char c;
	struct timeval tv;

	cnt = 0;

#ifdef DEBUG
	printf("SendMessage\t");
	for (i=0; i < PACKET_SIZE; i++)
		printf("[0x%x]", mesg[i]);

	printf("\n");
#endif

	/*
	 * Message
	 *
	 * [DRIVE_ID][INSTRUCTION][NEW_LINE]
	 * DRIVE_ID = ASCII char of the
	 *
	 * LF = Line feed
	 * y = 254 ASCII
	 * b = 266 ASCII
	 * yMR 51200[LF]			0254
	 * bMR 51200[LF]			0255
	 */
	retval = write(fd, mesg, PACKET_SIZE);
	if (retval > 0)
	{
		tcdrain(fd);

		/*
		 * Is there a reply
		 */
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		cnt = 0;
		do
		{
			tv.tv_sec = 0;
			tv.tv_usec = 25000;
			retval = select(fd+1, &rfds, NULL, NULL, &tv);

			if (retval == -1)
				perror("select()");
			else if (retval)
			{
				read(fd, &c, 1);
#ifdef DEBUG
				printf("[%x]", c);
#endif
				if (cnt < PACKET_SIZE)
					rxbuffer[cnt++] = c;
			}
		}while(retval > 0);
	}
	else
		printf("Error writing\n");

#ifdef DEBUG
	printf("\n");
#endif

	return cnt;
}


static
void usage(void)
{
	printf("usage:\n");

	if (mode == STANDARD)
		printf("./tri-prog -d 1-4 GGP 53,2,0\n");
	else
		printf("./tri-prog-file -d 1-4 filename\n");

	printf("\t-d 1-4, talk to drives 1 to 4\n");
	printf("\t-d 2, talk to drive 2\n");


	exit(1);
}
