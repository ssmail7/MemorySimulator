#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// If all pages and frames are 4KB (2^2 * 2^10 = 2^12) in a 32-bit address space,
// Page Table Entries (pages) = 2^32 address / 2^12 offset = 2^20 = 1048576

// Page Table Entries
#define SIZE 1048576

struct Entry
{
    int vpn;        // Virtual Page Number
    int valid;      // Valid bit
    int present;    // Present bit (present in physical memory)
    int dirty;      // Dirty bit (written on)
    int used;       // Use frequency (for LRU)
};

// Declarations
void memInit(struct Entry mem[], int frames);
void PTInit(struct Entry PT[], int size);
void rdm(int frames, char *tracefile, int debug);
void lru(int frames, char *tracefile, int debug);
void fifo(int frames, char *tracefile, int debug);
//void vms(int frames, char *tracefile, int debug);
int inMem(struct Entry mem[], int frames, unsigned pageNum, int debug);
void memPrint(struct Entry mem[], int frames);

// Initialize output counters
int eventNum = 0;
int readNum = 0;
int writeNum = 0;
int hits = 0;

int main(int argc, char *argv[])
{
    if(argc < 5)
    {
      printf("Invalid option.\n");
      printf("Format: memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>.\n");
    }
    else
    {
        char *tracefile;
        int frames;
        char *fun;
        char *mode;

        // Arg inputs
        tracefile = argv[1];
        frames = atoi(argv[2]);
        fun = argv[3];
        mode = argv[4];

        // Set Debug Mode
        int debug = 0;
        if(strcmp(mode,"debug") == 0)
            debug = 1;

        // Determine algorithm input
        if(strcmp(fun, "rdm") == 0)         // Random
            rdm(frames, tracefile, debug);
        else if(strcmp(fun, "lru") == 0)    // LRU
            lru(frames, tracefile, debug);
        else if(strcmp(fun, "fifo") == 0)   // FIFO
            fifo(frames, tracefile, debug);
        else if(strcmp(fun, "vms") == 0)    // VMS
            printf("VMS is still being developed.\n");
        else
            printf("Format: memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>.\n");
    }
    return 0;
}

// Initializes the Memory array
void memInit(struct Entry mem[], int frames)
{
    int i;
    for(i = 0; i < frames; i++)
    {
        mem[i].vpn = -1;
        mem[i].used = -1;   // for LRU
    }
}

// Initializes the Page Table array
void PTInit(struct Entry PT[], int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        PT[i].vpn = -1;
        PT[i].valid = 0;
        PT[i].present = -1;
        PT[i].dirty = 0;
    }
}

// Random (RDM) Page Replacement
void rdm(int frames, char *tracefile, int debug)
{
    struct Entry mem[frames];       // Declare the Memory array
    memInit(mem, frames);           // Initialize the Memory array

    static struct Entry PT[SIZE]; // Declare the Page Table array
    PTInit(PT, SIZE);             // Initialize the Page Table array

    unsigned addr;  // Holds the address from the file
	char rw;        // Holds the read/write from the file

	unsigned pageNum;   // Page number

	int frontMem = -1;  // Front memory slot (-1 is empty)
	int nextMem = 0;    // Next available memory slot

    int found = -1; // Found or Not Found

    // RDM Variables
    srand(time(NULL));  // Random number generator
    int random;

    // Open the file to be read
    FILE *file;
	file = fopen(tracefile, "r");

    if(file)
	{
		// Scan the file
		while(fscanf(file, "%x %c", &addr, &rw) != EOF)
		{
            // Get the page number (Offset: 12 bits; 2^12; 4096)
			pageNum = addr / 4096;

			// Set the valid bit
			if(PT[pageNum].valid == 0)
			{
				PT[pageNum].vpn = pageNum;
				PT[pageNum].valid = 1;
			}

            // Check to see if the frame is loaded in memory
			found = inMem(mem, frames, pageNum, debug);
			
			if(found >= 0)  // Page is in memory
			{
				if(rw == 'W')  // Page is written on
				{
					mem[found].dirty = 1;  // Mark as dirty
				}
			}
			else if(frontMem == -1)  // Memory is empty
			{
				if(debug)
				{
					printf("Put in first memory slot.\n");
				}
				frontMem = 0;
				nextMem = 1;
				mem[frontMem].vpn = pageNum;
				PT[pageNum].present = 1;
				readNum++;

                if(rw == 'W')  // Page is written on
				{
					mem[frontMem].dirty = 1;  // Mark as dirty
				}			
			}
			else if(frontMem == nextMem)  // Memory is full
			{
				if(debug)
				{
					printf("Memory is full.\n");
				}

                // Replace a random page
				random = rand() % frames;
                if(mem[random].dirty == 1)
				{
					PT[mem[random].vpn].present = 0;
					if(debug)
					{
						printf("Write to disk.\n");
					}
					writeNum++;
				}
				if(debug)
				{
					printf("Read from disk.\n");
				}
                readNum++;
				PT[pageNum].present = 1;
				mem[random] = PT[pageNum];
				if(rw == 'W')  // Page is written on
				{
					mem[random].dirty = 1;  // Mark as dirty
				}
				else
				{
					mem[random].dirty = 0;
				}
			}
			else  // Memory is not full
			{
				if(debug)
				{
					printf("Slot %d in array is filled.\n", nextMem);
				}
				mem[nextMem].vpn = pageNum;
				if(debug)
				{
					printf("Read from disk.\n");
				}
				readNum++;
				PT[pageNum].present = 1;
				if(rw == 'W')  // Page is written on
				{
					mem[nextMem].dirty = 1;  // Mark as dirty
				}
				nextMem = (nextMem + 1) % frames;
			}
			eventNum++;
			found = -1;
			if(debug)
			{
				memPrint(mem, frames);
			}
		}
		fclose(file);  // Close file to be read
	}
	printf("Total memory frames: %d\n", frames);
	printf("Events in trace: %d\n", eventNum);
	printf("Total disk reads: %d\n", readNum);
	printf("Total disk writes: %d\n", writeNum);
    printf("Hits: %d\n", hits);
    printf("Hits percentage: %f%%\n", ((double)hits / eventNum) * 100);
}

// Least Recently Used (LRU) Page Replacement
void lru(int frames, char *tracefile, int debug)
{
    struct Entry mem[frames];       // Declare the Memory array
    memInit(mem, frames);           // Initialize the Memory array

    static struct Entry PT[SIZE]; // Declare the Page Table array
    PTInit(PT, SIZE);             // Initialize the Page Table array

    unsigned addr;  // Holds the address from the file
    char rw;        // Holds the read/write from the file

    unsigned pageNum;  // Page number

    int frontMem = -1;  // Front memory slot (-1 is empty)
    int nextMem = 0;    // Next available memory slot
    
    int found = -1;  // Found or Not Found

    // LRU Variables
    int recent = 0;     // Determine when a page was used
    int leastRecent;    // Least recently used page
    int leastRecentMem; // Memory spot of the least recently used page
    int i;              // Iterator

    // Open the file to be read
    FILE *file;
    file = fopen(tracefile, "r");

    if(file)
    {
        // Scan the file
        while(fscanf(file, "%x %c", &addr, &rw) != EOF)
        {
            // Get the page number (Offset: 12 bits; 2^12; 4096)
            pageNum = addr / 4096;

            // Set the valid bit
            if(PT[pageNum].valid == 0)
            {
                PT[pageNum].vpn = pageNum;
                PT[pageNum].valid = 1;
            }

            // Check to see if the frame is loaded in memory
            found = inMem(mem, frames, pageNum, debug);
            
            if(found >= 0)  // Page is in memory
            {
                if(rw == 'W')  // Page is written on
                {
                    mem[found].dirty = 1;  // Mark as dirty
                }
                mem[found].used = recent;  // Update used (LRU)
                recent++;
            }
            else if(frontMem == -1)  // Memory is empty
            {
                if(debug)
                {
                    printf("Put in first memory slot.\n");
                }
                frontMem = 0;
                nextMem = 1;
                mem[frontMem].vpn = pageNum;
                PT[pageNum].present = 1;
                readNum++;
                mem[frontMem].used = recent;  // Update used (LRU)
                recent++;

                if(rw == 'W')  // Page is written on
                {
                    mem[frontMem].dirty = 1;  // Mark as dirty
                }
            }
            else if(frontMem == nextMem)  // Memory is full
            {
                if(debug)
                {
                    printf("Memory is full.\n");
                }
                leastRecent = mem[0].used;
                leastRecentMem = 0;

                // Find and replace the least recently used page in memory
                for(i = 1; i < frames; i++)
                {
                    if(mem[i].used < leastRecent)
                    {
                        leastRecent = mem[i].used;
                        leastRecentMem = i;
                    }
                }
                if(mem[leastRecentMem].dirty == 1)
                {
                    PT[mem[leastRecentMem].vpn].present = 0;
                    if(debug)
                    {
                        printf("Write to disk.\n");
                    }
                    writeNum++;
                }
                if(debug)
                {
                    printf("Read from disk.\n");
                }
                readNum++;
                PT[pageNum].present = 1;
                mem[leastRecentMem] = PT[pageNum];
                mem[leastRecentMem].used = recent;
                recent++;
                if(rw == 'W')  // Page is written on
                {
                    mem[leastRecentMem].dirty = 1;  // Mark as dirty
                }
                else
                {
                    mem[leastRecentMem].dirty = 0;
                }
            }
            else  // Memory is not full
            {
                if(debug)
                {
                    printf("Slot %d in array is filled.\n", nextMem);
                }
                mem[nextMem].vpn = pageNum;
                mem[nextMem].used = recent;
                recent++;
                if(debug)
                {
                    printf("Read from disk.\n");
                }
                readNum++;
                PT[pageNum].present = 1;
                if(rw == 'W')  // Page is written on
                {
                    mem[nextMem].dirty = 1;  // Mark as dirty
                }
                nextMem = (nextMem + 1) % frames;
            }
            eventNum++;
            found = -1;
            if(debug)
            {
                memPrint(mem, frames);
            }
        }
        fclose(file);  // Close file to be read
    }
    printf("Total memory frames: %d\n", frames);
    printf("Events in trace: %d\n", eventNum);
    printf("Total disk reads: %d\n", readNum);
    printf("Total disk writes: %d\n", writeNum);
    printf("Hits: %d\n", hits);
    printf("Hits percentage: %f%%\n", ((double)hits / eventNum) * 100);
}

// First In First Out (FIFO) Page Replacement
void fifo(int frames, char *tracefile, int debug)
{
    struct Entry mem[frames];       // Declare the Memory array
    memInit(mem, frames);           // Initialize the Memory array

    static struct Entry PT[SIZE]; // Declare the Page Table array
    PTInit(PT, SIZE);             // Initialize the Page Table array

    unsigned addr;  // Holds the address from the file
    char rw;        // Holds the read/write from the file

    unsigned pageNum;  // Page number

    int frontMem = -1;  // Front memory slot (-1 is empty)
    int nextMem = 0;    // Next available memory slot
    
    int found = -1;  // Found or Not Found

    // FIFO Variables
    // (none)

    // Open the file to be read
    FILE *file;
    file = fopen(tracefile, "r");

    if(file)
    {
        // Scan the file
        while(fscanf(file, "%x %c", &addr, &rw) != EOF)
        {
            // Get the page number (Offset: 12 bits; 2^12; 4096)
            pageNum = addr / 4096;

            // Set the valid bit
            if(PT[pageNum].valid == 0)
            {
                PT[pageNum].vpn = pageNum;
                PT[pageNum].valid = 1;
            }

            // Check to see if the frame is loaded in memory
            found = inMem(mem, frames, pageNum, debug);
            
            if(found >= 0)  // Page is in memory
            {
                if(rw == 'W')  // Page is written on
                {
                    mem[found].dirty = 1;  // Mark as dirty
                }
            }
            else if(frontMem == -1)  // Memory is empty
            {
                if(debug)
                {
                    printf("Put in first memory slot.\n");
                }
                frontMem = 0;
                nextMem = 1;
                mem[frontMem].vpn = pageNum;
                PT[pageNum].present = 1;
                readNum++;

                if(rw == 'W')  // Page is written on
                {
                    mem[frontMem].dirty = 1;  // Mark as dirty
                }
            }
            else if(frontMem == nextMem)  // Memory is full
            {
                if(debug)
                {
                    printf("Memory is full.\n");
                }

                // Replace the first page (First In, First Out)
                if(mem[frontMem].dirty == 1)
                {
                    PT[mem[frontMem].vpn].present = 0;
                    if(debug)
                    {
                        printf("Write to disk.\n");
                    }
                    writeNum++;
                }
                if(debug)
                {
                    printf("Read from disk.\n");
                }
                readNum++;
                PT[pageNum].present = 1;
                mem[frontMem] = PT[pageNum];
                if(rw == 'W')  // Page is written on
                {
                    mem[frontMem].dirty = 1;  // Mark as dirty
                }
                else
                {
                    mem[frontMem].dirty = 0;
                }
                // Move the front of the memory
                frontMem = (frontMem + 1) % frames;
                nextMem = frontMem;
            }
            else  // Memory is not full
            {
                if(debug)
                {
                    printf("Slot %d in array is filled.\n", nextMem);
                }
                mem[nextMem].vpn = pageNum;
                if(debug)
                {
                    printf("Read from disk.\n");
                }
                readNum++;
                PT[pageNum].present = 1;
                if(rw == 'W')  // Page is written on
                {
                    mem[nextMem].dirty = 1;  // Mark as dirty
                }
                nextMem = (nextMem + 1) % frames;
            }
            eventNum++;
            found = -1;
            if(debug)
            {
                memPrint(mem, frames);
            }
        }
        fclose(file);  // Close file to be read
    }
    printf("Total memory frames: %d\n", frames);
    printf("Events in trace: %d\n", eventNum);
    printf("Total disk reads: %d\n", readNum);
    printf("Total disk writes: %d\n", writeNum);
    printf("Hits: %d\n", hits);
    printf("Hits percentage: %f%%\n", ((double)hits / eventNum) * 100);
}

/*void vms(int frames, char *tracefile, int debug)
{
    
}
*/

// Checks if frame is loaded into memory and returns the location
int inMem(struct Entry mem[], int frames, unsigned pageNum, int debug)
{
    int i;
    for(i = 0; i < frames; i++)
    {
        if(mem[i].vpn == pageNum)
        {
            if(debug)
            {
                printf("Found in memory.\n");
            }
            hits++;
            return i;   // Page was found in memory
        }
    }
    return -1;  // Page was not found in memory
}

// Prints the memory location in debug mode
void memPrint(struct Entry mem[], int frames)
{
    int i;
    printf("Memory: ");
    for(i = 0; i < frames; i++)
    {
        printf("%d  ", mem[i].vpn);
    }
    printf("\n");
}
