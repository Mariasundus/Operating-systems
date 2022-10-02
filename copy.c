#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

// EOPSY Lab 6 - Operations on files, mmap() and getopt()

// Write a program which copies one file to another. Syntax:
//   copy [-m] <file_name> <new_file_name>
//   copy [-h]

// Without option -m use read() and write() functions to copy file contents. If
// the option -m is given, do not use neither read() nor write() but map files
// to memory regions with mmap() and copy the file with memcpy() instead.

// Important remarks: 
// - use getopt() function to process command line options and arguments,
//   should be the same, but in some place either copy_read_write(int fd_from,

void usage() {
    printf("Usage: copy [-m] <file_name> <new_file_name>\n");
    printf("    - if -m flag is set, file will be copied using mmap() and memcpy()\n");
    printf("    - if -m flag is not set, file will be copied using read() and write()\n");
    printf("To display help: copy [-h]\n");
}
// Gets all of the non-option arguments/filter them out
void getNonArgs(int argc, char *argv[]) {
    if (optind < argc) {
        while(optind < argc) {
            if (strcmp(argv[optind], "-h") == 0) {
                usage();
            }
            optind++; // index value handled by gett=opt()
        }
    }
}
// Copy file using read() and write()
void copy_read_write(int fd_from, int fd_to) {
    off_t fsize;
    if ((fsize = lseek(fd_from, 0, SEEK_END)) == -1) { // Get size of source file using lseek()
        printf("Error getting size of source file : %s\n", strerror(errno));
        exit(1);
    }
    if ((lseek(fd_from, 0, SEEK_SET)) == -1) { // reset file offset
        perror("Error resetting source file offset");
        exit(1);
    }
    int buf_size = 100;
    printf("buff size: %d\n", buf_size);
    printf("fsize: %lld\n", fsize);
    if (fsize < buf_size) { // For small files
        char buf[fsize]; // initialize buffer array with file size
        if (read(fd_from, buf, sizeof(buf)) == -1) { // read from source file to buffer array
            printf("Error reading from source file: %s", strerror(errno));
        }
        if (write(fd_to, buf, sizeof(buf)) == -1) { // write buffer to destination file
            printf("Error writing to file : %s\n", strerror(errno));
            exit(1);
        }
    } else { // This runs for larger files using chunks
        size_t nd;
        char buf[buf_size];
        while (nd = read(fd_from, buf, 100)) { // Read from file to buffer. Read() returns 0 when end of file is reached.
            if(write(fd_to, buf, nd) == -1) { // Write buffer to destination file
                printf("Error writing to file : %s\n", strerror(errno));
                
                exit(1);
            }
        }
    }
    
    printf("File copied successfully.\n");
}
// Copy file using mmap() and memcpy()
void copy_mmap(int fd_from, int fd_to) {
    off_t fsize;
    char *src, *dest;
    // Get size of source file using lseek()
    if ((fsize = lseek(fd_from, 0, SEEK_END)) == -1) { 
        printf("Error getting size of source file : %s\n", strerror(errno));
        exit(1);
    }
    if ((lseek(fd_from, 0, SEEK_SET)) == -1) { // reset file offset
        perror("Error resetting source file offset");
        exit(1);
    }
    // Map source file to memory - returns pointer to mapped area
    src = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd_from, 0); 
    if (src == MAP_FAILED) {
        printf("Error mapping source file: %s\n", strerror(errno));
        exit(1);
    }
    // Truncate destination file to file size (without this, exec format error occurs)
    if (ftruncate(fd_to, fsize) == -1) { 
        printf("Error truncating destination file: %s\n", strerror(errno));
    }
    // Map destination file to memory
    dest = mmap(NULL, fsize, PROT_WRITE, MAP_SHARED, fd_to, 0); 
    if (dest == MAP_FAILED) {
        printf("Error mapping destination file: %s\n", strerror(errno));
        exit(1);
    }
    //Copy bytes from source memory area to destination memory area
    memcpy(dest, src, fsize); 
    // Unmap source and destination memory areas
    if (munmap(src, fsize) == -1) { 
        printf("Error unmapping source memory area: %s\n", strerror(errno));
        exit(1);
    }
    if (munmap(dest, fsize) == -1) {
        printf("Error unmapping destination memory area: %s\n", strerror(errno));
        exit(1);
    }
    printf("File copied successfully.\n");
}

int main(int argc, char *argv[]) {
    int option;
    bool mflag = false;
    char *source_file = NULL;
    char *dest_file = NULL;
    int fd_to, fd_from;

    // If program is called without arguments, print help info
    if (argc < 2) {
        usage();
        exit(1);
    }
    // Get options and option arguments
    while (optind < argc) {
        // Disable automatic error printing by putting colon as first character in optstring "hm:"
        if ((option = getopt(argc, argv, ":hm:")) != -1) {
            switch(option) {
                case 'm':
                    mflag = true; // set m flag
                    if (optarg == NULL || argv[optind] == NULL) { // check if we are missing file names
                        printf("Error: Missing arguments for -m\n");
                        exit(1);
                    } else {
                        // Source file is first argument provided to -m option
                        source_file = optarg;
                         // Second argument should be destination file
                         // optind is an integer which contains index value of next argument that should be handled by getopt()
                        dest_file = argv[optind];
                        optind++; 
                    }
                    break;
                case 'h':
                    usage();
                    exit(0);
                case '?':
                    // getopt() returns ? if it does not recognize an option given to it
                    // option character is assigned to optopt
                    printf("Error: Unknown option: -%c\n", optopt);
                    usage();
                    exit(1);
                case ':':
                    printf("Error: Missing arguments for -%c\n", optopt);
                    exit(1);
            }
            getNonArgs(argc, argv); // filter out rest of non-option arguments
        } else { // if no options, then get filenames from first two non-option arguments
            if (argv[optind] != NULL && argv[optind+1] != NULL) {
                source_file = argv[optind];
                optind++;
                dest_file = argv[optind];
                optind++;
            } else {
                printf("Error: missing arguments\n");
                exit(1);
            }
            getNonArgs(argc, argv); // filter out rest of non-option arguments
            break;
        }   
    }
    if (strcmp(source_file, dest_file) == 0) {
        printf("Error: Source and destination files should be different.\n");
        exit(1);
    }
    if (strcmp("copy.c", dest_file) == 0) {
        printf("Error: Cannot copy to program file.\n");
        exit(1);
    }
    // Get file descriptors from source and destination file names
    if ((fd_from = open(source_file, O_RDONLY)) == -1) {
        printf("Error opening source file: %s. %s\n", source_file, strerror(errno));
        exit(1);
    }
    if ((fd_to = open(dest_file, O_CREAT | O_RDWR | O_TRUNC, 0600)) == -1) {
        printf("Error opening destination file: %s. %s\n", dest_file, strerror(errno));
        exit(1);
    }
    // Check if mflag is set
    if (mflag) {
        copy_mmap(fd_from, fd_to);
    } else {
        copy_read_write(fd_from, fd_to);
    }
    // Close source and destination files
    if (close(fd_from) == -1 || close(fd_to) == -1) { 
        perror("Error closing files");
        exit(1);
    }
    return 0;
}
