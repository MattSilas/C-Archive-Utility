#include <ar.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#define AR_MAGIC "!<arch>\012"

int isOdd(int value)
{
  if (((value / 2) * 2) != value)
    return(1); // It is an odd value
  else
    return(0); // It is an even value
}

int rmv(char *archiveFileName, char *file)
{
  FILE *pFile;
  char *buff;
  char *ptr;
  buff = (char *)malloc(sizeof(archiveFileName));
  memset(buff,0,sizeof(archiveFileName));
  ptr = buff;
  int arfd = open(archiveFileName, O_RDONLY);
  struct stat arst;
  int output;
  int num_written, num_read;
  output = open("test_out",O_CREAT|O_WRONLY|O_APPEND,0666);
  if(fstat(arfd,&arst)<0) printf("error reading stats");
 
  if(arst.st_size==0){
  if((num_written = write(output,AR_MAGIC,8))!=8){
      printf("error writing header");
  }
  }
  
  if (arfd < 0)
    {
      (void) fprintf(stderr, "Cannot open file '%s'\n", archiveFileName);
      return(1);
    }
  char magicString[SARMAG+1];
  ssize_t res = read(arfd, magicString, SARMAG);
  struct ar_hdr memberHeader;
  while(1){
    res = read(arfd, &memberHeader, sizeof(struct ar_hdr));
    if (res < sizeof(struct ar_hdr))
      {
	break;
      }
    
    char memberSizeChar[sizeof(memberHeader.ar_size) + 1];
    (void) strncpy(memberSizeChar, memberHeader.ar_size, sizeof(memberHeader.ar_size));
    memberSizeChar[sizeof(memberHeader.ar_size)] = 0; // make sure null terminated                                                                                        // Convert the string to an integer.                                                                                                                              
    off_t memberSize = atoi(memberSizeChar);
    off_t headSize = 60;
    char memberFileName[sizeof(memberHeader.ar_name)+1];
    (void) strncpy(memberFileName, memberHeader.ar_name, sizeof(memberHeader.ar_name));
    char *slashPos = strchr(memberFileName, '/');
    *slashPos = 0;
    int comparator = strlen(file);
    num_read = num_written = 0;
    int copied;
    char buffer[1];
    if(strncmp(file,memberFileName,comparator)!=0){
      copied = 0;      
      lseek(arfd,-60,SEEK_CUR);
       while(copied<memberSize+headSize){
        num_read = read(arfd,buffer,1);
        num_written = write(output,buffer,1);
        if(num_written!=num_read){
          perror("Problem writing file");
          unlink("test_out");
          exit(-1);
	  }
	
        copied+=num_written;
	}    
      
    }
    if (isOdd(memberSize))
      {
        memberSize++;
      }

    (void) lseek(arfd, memberSize, SEEK_CUR);
  }
  close(output);
  close(arfd);
  return(0);
}

int quickAppend(char *archiveFileName, char *file)
{
  int fileIndex = 0;
  char buffer[1];
  char magic[8];
  int in_fd, out_fd,len,num_read,num_written;
  struct stat arst, inst;
  struct ar_hdr *hdr = (struct ar_hdr*) malloc(sizeof(struct ar_hdr));
  char buf[32];  
  char *p, *name;
  out_fd = open(archiveFileName, O_CREAT|O_WRONLY|O_APPEND,0666);
  in_fd = open(file,O_RDONLY);
 
  if(fstat(out_fd,&arst)<0) exit(-1);
  if(fstat(in_fd,&inst)<0) exit(-1);

  if(arst.st_size==0){
    if((num_written = write(out_fd,AR_MAGIC,8))!=8){
      printf("error writing header");
    }
 }
  
  memset(hdr,' ',sizeof(struct ar_hdr));
  name = file;
  p = name;
  while (*p) {
    if (*p == '/' || *p == '\\') {
      name = ++p;
    } else {
      p++;
    }
  }
  len = strlen(name);
  if (len > 15) len = 15;
  memcpy(hdr->ar_name, name, len);
  hdr->ar_name[len] = '/';
  
  
  sprintf(buf, "%ld", inst.st_mtime);
  memcpy(hdr->ar_date, buf, strlen(buf));
  sprintf(buf, "%d", inst.st_uid);
  memcpy(hdr->ar_uid, buf, strlen(buf));
  sprintf(buf, "%d", inst.st_gid);
  memcpy(hdr->ar_gid, buf, strlen(buf));
  sprintf(buf, "%o",inst.st_mode);
  memcpy(hdr->ar_mode, buf, strlen(buf));
  sprintf(buf, "%lld", inst.st_size);
  memcpy(hdr->ar_size, buf, strlen(buf));
  hdr->ar_fmag[0] = '`';
  hdr->ar_fmag[1] = '\n';
  
  write(out_fd,hdr,sizeof(struct ar_hdr));
  
  int copied;
  copied = 0;
  while(copied<inst.st_size){
    num_read = read(in_fd,buffer,1);
    num_written = write(out_fd,buffer,1);
    if(num_written!=num_read){
      perror("Problem writing file");
      unlink(archiveFileName);
      exit(-1);
    }
    copied+=num_written;
  }
  
  (void) fprintf(stdout, "\t %s: all done\n", file);

  return(0);
}

int conciseTOC(char *archiveFileName)
{
  //fprintf(stdout, "%s: entry\n", __func__);

  // Open the archive file.
  int arfd = open(archiveFileName, O_RDONLY);
  if (arfd < 0)
    {
      (void) fprintf(stderr, "Cannot open file '%s'\n", archiveFileName);
      return(1);
    }
    
  // This is the "magic number" string from the very beginning of the
  // archive file.
  char magicString[SARMAG+1];
  ssize_t res = read(arfd, magicString, SARMAG);
  //magicString[SARMAG] = 0;  // you only need this if you print the value.
  //(void) fprintf(stdout, "magic %d %d ++%s++\n", res, SARMAG, magicString);
    
  // This comes from the ar.h include file.  You need to use this structure in
  // teh ar.h file to get this assignment right.
  struct ar_hdr memberHeader;
  while (1) // This is an infinate loop.  There is a break statement to exit the loop.
    {
      // Read the member header.
      // If you don't know about the sizeof() operator, you should 
      // review http://en.wikipedia.org/wiki/Sizeof
      res = read(arfd, &memberHeader, sizeof(struct ar_hdr));
      if (res < sizeof(struct ar_hdr))
        {
	  // The number of bytes read is less than the sizeof() the ar_hdr
	  // structure.  It must be the end of the file.
	  //(void) fprintf(stdout, "end of file\n");
	  break;
        }

      // Extract the member file name from the header.
      char memberFileName[sizeof(memberHeader.ar_name)+1];
      (void) strncpy(memberFileName, memberHeader.ar_name, sizeof(memberHeader.ar_name));
      char *slashPos = strchr(memberFileName, '/');
      *slashPos = 0; // Make sure the string is null terminated.
      // Strings in C must always be null terminated.
      
      // This is where the member file name is actually printed to he terminal.
      (void) fprintf(stdout, "%s\n", memberFileName);
        
      // This is where the member size is used to determine the amount to
      // skip ahead in the file.
      char memberSizeChar[sizeof(memberHeader.ar_size) + 1];
      (void) strncpy(memberSizeChar, memberHeader.ar_size, sizeof(memberHeader.ar_size));
      memberSizeChar[sizeof(memberHeader.ar_size)] = 0; // make sure null terminated
      // Convert the string to an integer.
      off_t memberSize = atoi(memberSizeChar);
        
      // I need to know if the memberSize value is even or odd.
      if (isOdd(memberSize))
        {
	  // from http://www.unix.com/man-page/opensolaris/3head/ar.h
	  // "Each archive file member begins on an even byte boundary"
	  // So, if the member size of odd,       if (isOdd(memberSize))
	                                                                                                      
	    memberSize++;
	  }
	// Skip ahead in the file.
      (void) lseek(arfd, memberSize, SEEK_CUR);
    }
    
  // Close the file descriptor to the archive file.
  close(arfd);
  //(void) fprintf(stdout, "%s: exit\n", __func__);
  return(0);
}

int printVerbose(char *archiveFileName){
  int arfd = open(archiveFileName, O_RDONLY);
  if (arfd < 0)
    {
      (void) fprintf(stderr, "Cannot open file '%s'\n", archiveFileName);
      return(1);
    }
  char magicString[SARMAG+1];
  ssize_t res = read(arfd, magicString, SARMAG);
  struct ar_hdr memberHeader;
  while (1)                                                                           
    {
      res = read(arfd, &memberHeader, sizeof(struct ar_hdr));
      if (res < sizeof(struct ar_hdr))
        {                                                                                           
          break;
        }
      char memberMode[sizeof(memberHeader.ar_mode)+1];
      (void) strncpy(memberMode,memberHeader.ar_mode,sizeof(memberHeader.ar_mode));
      memberMode[sizeof(memberHeader.ar_mode)] = 0;
      (void) fprintf(stdout,"%s",memberMode);

      char memberUID[sizeof(memberHeader.ar_uid)+1];
      (void) strncpy(memberUID,memberHeader.ar_uid,sizeof(memberHeader.ar_uid));      
      memberUID[sizeof(memberHeader.ar_uid)] = 0;
      (void) fprintf(stdout,"%s",memberUID);

      char memberGID[sizeof(memberHeader.ar_gid)+1];
      (void) strncpy(memberGID,memberHeader.ar_gid,sizeof(memberHeader.ar_gid));
      memberGID[sizeof(memberHeader.ar_gid)] = 0;
      (void) fprintf(stdout,"%s",memberGID);

      char memberSizeChar[sizeof(memberHeader.ar_size) + 1];
      (void) strncpy(memberSizeChar, memberHeader.ar_size, sizeof(memberHeader.ar_size));
      memberSizeChar[sizeof(memberHeader.ar_size)] = 0;
      (void) fprintf(stdout,"%s",memberSizeChar);

      char memberDate[sizeof(memberHeader.ar_date) + 1];
      (void) strncpy(memberDate, memberHeader.ar_date, sizeof(memberHeader.ar_date));
      memberDate[sizeof(memberHeader.ar_date)] = 0;
      (void) fprintf(stdout,"%s",memberDate);

      char memberFileName[sizeof(memberHeader.ar_name)+1];
      (void) strncpy(memberFileName, memberHeader.ar_name, sizeof(memberHeader.ar_name));
      char *slashPos = strchr(memberFileName, '/');
      *slashPos = 0;
      (void) fprintf(stdout, "%s\n", memberFileName);

      off_t memberSize = atoi(memberSizeChar);
      if (isOdd(memberSize))
	{
	  memberSize++;
	}
      (void) lseek(arfd, memberSize, SEEK_CUR);
    }
  close(arfd);
  return 0;
}

int extract(char *archiveFileName, char *file)
{
  int arfd = open(archiveFileName, O_RDONLY);
  if (arfd < 0)
    {
      (void) fprintf(stderr, "Cannot open file '%s'\n", archiveFileName);
      return(1);
    }

  char magicString[SARMAG+1];
  ssize_t res = read(arfd, magicString, SARMAG);
  struct ar_hdr memberHeader;
  while(1){
    res = read(arfd, &memberHeader, sizeof(struct ar_hdr));
    if (res < sizeof(struct ar_hdr))
        {                                                                                           
          break;
        }
    char memberSizeChar[sizeof(memberHeader.ar_size) + 1];
    (void) strncpy(memberSizeChar, memberHeader.ar_size, sizeof(memberHeader.ar_size));
    memberSizeChar[sizeof(memberHeader.ar_size)] = 0; // make sure null terminated                                                                                     
    // Convert the string to an integer.                                                                                                                              
    off_t memberSize = atoi(memberSizeChar);
    char memberFileName[sizeof(memberHeader.ar_name)+1];
    (void) strncpy(memberFileName, memberHeader.ar_name, sizeof(memberHeader.ar_name));
    char *slashPos = strchr(memberFileName, '/');
    *slashPos = 0;
    int comparator = strlen(file);
    int num_read,  num_written;
    num_read = num_written = 0;
    int copied;
    char buffer[1];
    int output;
    if(strncmp(file,memberFileName,comparator)==0){
      output = open(file,O_CREAT|O_WRONLY,0666);
      copied = 0;
      while(copied<memberSize){
	num_read = read(arfd,buffer,1);
	num_written = write(output,buffer,1);
	if(num_written!=num_read){
	  perror("Problem writing file");
	  unlink("test_out");
	  exit(-1);
	}
	copied+=num_written;
      }

      close(output);
      break;
    }
                                                     
    if (isOdd(memberSize))
      {
	memberSize++;
      }
                                                                                                                                        
    (void) lseek(arfd, memberSize, SEEK_CUR);
  }
  close(arfd);
  return(0);
} 

void appendAll(char *archiveFileName)
{
  DIR *mydir = opendir("./");
  struct dirent *entry;
  struct stat info;
  int i;
  while((entry = readdir(mydir)))
    {
      if(stat(entry->d_name, &info)){
	printf("error: stat: %s\n", entry->d_name);
	continue;
      }
      if(S_ISREG(info.st_mode)&&entry->d_name[0]!='.'){
	char file[strlen(entry->d_name)-1];
	
	for(i = 0; i<strlen(entry->d_name);i++){
	  file[i]=entry->d_name[i];
	}
	quickAppend(archiveFileName,entry->d_name);
	}
      }

  closedir(mydir);

}

int main(int argc, char *argv[])
{
  if (argc < 3)
    {
      (void) fprintf(stderr, "ERROR: %s must have at least 2 command line arguments\n", argv[0]);
      exit(1);
    }
    
  char *optionStr = argv[1]; // the command line option that specifies the operation.
  char optionChar = optionStr[1]; // just the single letter (no preceeding dash).
  char *archiveFileName = argv[2]; // the name of the archive file.
  int numFiles=0;
  int res = 0;
  //(void) fprintf(stdout, "optionStr = %s\n", optionStr);
  //(void) fprintf(stdout, "optionChar = %c\n", optionChar);
  //(void) fprintf(stdout, "archiveFileName = %s\n", archiveFileName);
  switch (optionChar)
    {
    case 'q':
      // Quickly append named files to archive.
      
      for(numFiles = 3; numFiles<argc; numFiles++){
	quickAppend(archiveFileName, argv[numFiles]);
      }
      break;
    case 'x':
      // Extract named files.
      for(numFiles = 3; numFiles<argc; numFiles++){
        extract(archiveFileName, argv[numFiles]);
      }
      break;
    case 't':
      // Print a concise table of contents of the archive.
      res = conciseTOC(archiveFileName);
      break;
    case 'v':
      // Print a verbose table of contents of the archive.
      res = printVerbose(archiveFileName);
      break;
    case 'd':
      // Delete named files from archive.
      for(numFiles = 3; numFiles<argc; numFiles++){
        rmv(archiveFileName, argv[numFiles]);
      }
      break;
    case 'A':        
      appendAll(archiveFileName);
      break;
    case 'w':
      // Extra credit: for a given timeout (in seconds), add all modified files to the archive.
      (void) fprintf(stdout, "-w not implemented yet\n");
      break;
    default:
      (void) fprintf(stderr, "ERROR: unknown command line option '%s'\n", optionStr);
      break;
    }
  return(res);
}
