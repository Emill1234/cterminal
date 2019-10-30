#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

/* reads a complete line from the standard input and returns a pointer to an */
/* allocated buffer with the read string */
char *read_input()
{
    int n,i;
    char c,*buffer;
    i=0;
    buffer=(char*)malloc(512);
    while(i<512)    /* while there's space in buffer */
    {
        n=read(STDIN_FILENO,&c,1);  /* read one char at a time */
        if(n==1 && c!='\n')
          buffer[i++]=c;
        else if(n<=0)
        {
          free(buffer);
          return NULL;
        }
        else
          break;
    }
    buffer[i]=0;  /* insert end of string */
    return buffer;
}

/* reads a single char from the standard input, waits until a return is entered */
char read_char()
{
    int n;
    char c,b;
    c=0;
    while(1)    /* while there's space in buffer */
    {
        n=read(STDIN_FILENO,&b,1);  /* read one char at a time */
        if(n==1 && b!='\n')
        {
          if(c==0)
            c=b;
        }
        else if(n<=0)
          return 0;
        else
          break;
    }
    return c;
}

/* prints a string on the standard output */
void print(char *str)
{
  int i;
  for(i=0; i<strlen(str); i++)
    write(STDOUT_FILENO,&str[i],1); 
}

/* splits a string using the given delimiters, returns an array of pointers to the */
/* split parts and the number of parts is saved in the nargs argument */
char **split(char *str,char delim1,char delim2,int *nargs)
{
  int i,n,len;
  int word;    /* indicates if we are in a word */
  char **args;
  word=0;
  len=strlen(str); 
  n=0;
  args=(char **)malloc(sizeof(char*));  /* start with one pointer */
  for(i=0; i<=len; i++)
  {
      if(str[i]!=delim1 && str[i]!=delim2 && str[i]!=0)
      {
        if(!word)
        {
          args[n++]=&str[i];
          args=(char **)realloc(args,(n+1)*sizeof(char*));  /* add one more pointer                      */
        }
        word=1;
      }
      else
      {
        if(word)
          str[i]=0;
        word=0;
      }
  }
  args[n]=NULL; /* last pointer is a NULL */
  *nargs=n;
  return args;
}

/* removes spaces at the end and the start of a string, it returns a pointer to */
/* a new string without spaces */
char *trim(char *str)
{
  char *p;
  /* remove spaces at end of string */
  while(str[strlen(str)-1]==' ' || str[strlen(str)-1]=='\t')
    str[strlen(str)-1]=0;
  p=str;
  /* remove spaces at start of string */
  while(*p && (*p==' ' || *p=='\t'))
    p++;
  return p;
}

/* scans a given string looking for redirection characters > and <, */
/* it takes out the filenames found after the redirection and saves them in rin */
/* and rout, the string is updated by removing the redirection parts */
void set_redirection(char *str,char **rin,char **rout)
{
  int len;
  int i;
  int redir;
  len=strlen(str);
  redir=0;
  *rin=NULL;
  *rout=NULL;
  for(i=0; i<len; i++)
  {
    if(str[i]=='>' || str[i]=='<')
    {       
      redir=str[i];
      str[i]=0;
    }
    else if(redir)
    {
      if(redir=='>')
        *rout=&str[i];
      else /* if(redir=='<') */
        *rin=&str[i];
      redir=0;
    }
  }
  if(*rin)
    *rin=trim(*rin);
  if(*rout)
    *rout=trim(*rout);
}

/* executes the dir command */
void dir_command(char **args,int nargs)
{
  DIR * dir;
  struct dirent* dir_entry;
  if(nargs>1)
    fprintf(stderr,"error: too many parameters for dir\n");
  else
  {
	  dir = opendir("."); /* open current directory */
  	while((dir_entry=readdir(dir))!=NULL) /* read all directory entries */
		{
  		if(dir_entry->d_name[0]!='.') /* skip files starting with . */
        printf("%s\n",dir_entry->d_name);
    }    
  }  
  exit(EXIT_SUCCESS);
}

/* gets the options given to mv and saves them in the corresponding */
/* argument variables, returns 1 if the operation was successfule, 0 if there */
/* was an error in one of the options */
int get_mvarguments(char **args,int nargs,int *interactive,char *dest,char *bext)
{
  int i;
  *interactive=0;
  dest[0]=0;
  bext[0]=0;
  for(i=1; i<nargs; i++)
  {
    if(!strcmp(args[i],"-i")) /* interactive */
    {
      *interactive=1;
      args[i][0]=0;
    } 
    else if(!strcmp(args[i],"-t")) /* move to directory */
    {
      if(i+1>=nargs)
      {
        fprintf(stderr,"error: missing directory name for -t option.\n");
        return 0;
      }      
      strcpy(dest,args[i+1]);
      args[i][0]=0;
      args[i+1][0]=0;
      i++; /* skip two places */
    } 
    else if(!strcmp(args[i],"-S")) /* backup suffix */
    {
      if(i+1>=nargs)
      {
        fprintf(stderr,"error: missing suffix for -S option.\n");
        return 0;
      }      
      strcpy(bext,args[i+1]);
      args[i][0]=0;
      args[i+1][0]=0;
      i++; /* skip two places */
    }
  }
  return 1;
}

/* takes the filename part from the path and returns a pointer to it */
char *get_filename(char *path)
{
  char *p;
  p=&path[strlen(path)-1];  /* point to tail of path */
  while(p!=path && *p!='/')  /* search for the last slash char */
    p--;
  if(*p=='/') /* point to next char after the slash */
    p++;
  return p;
}

void mv_dir(char *,char *,int ,char *);

/* moves a file with name src to a new called dest using the given options */
void mv_file(char *src,char *dest,int interactive,char *bext)
{
  char buffer[512];
  struct stat stat_entry,src_stat;  
  char c;
  
	if (stat(src,&src_stat)==-1)
	{
    fprintf(stderr,"error: not such file or directory '%s'\n",src);
    return;
	}
  if(S_ISDIR(src_stat.st_mode)) /* if source is a directory */
  {
    mv_dir(src,dest,interactive,bext);  /* move directory */
    return;
  }
	if (stat(dest,&stat_entry)!=-1)
  {
    if(interactive)
    {
      print("Overwrite file '");
      print(dest);
      print("'? (y/n) ");
      c=read_char();
      if(c!='y' && c!='Y')
        return;
    }
    if(bext[0])
    {
      sprintf(buffer,"%s%s",dest,bext);   /* create backup using the suffix */

    	if (stat(buffer,&stat_entry)!=-1)
    	{
        if(unlink(buffer)==-1)
        {
          fprintf(stderr,"error: unable to remove file '%s'\n",buffer);
          return;
        }
      }
      if(link(dest,buffer)==-1)      {
        fprintf(stderr,"error: unable to create backup '%s'\n",buffer);
        return;
      }
    }
    if(unlink(dest)==-1)
    {
      fprintf(stderr,"error: unable to overwrite file '%s'\n",dest);
      return;
    }
  }  
  if(link(src,dest)==-1)
    fprintf(stderr,"error: unable to move file '%s' to '%s'\n",src,dest);
  else if(unlink(src)==-1)
    fprintf(stderr,"error: unable to remove file '%s'\n",src);
}

/* moves a directory with name src to a new called dest using the given options */
void mv_dir(char *src,char *dest,int interactive,char *bext)
{
  char srcpath[512],dstpath[512];
  struct stat stat_entry,src_stat;  
  DIR * dir;
  struct dirent* dir_entry;
  char c;
  
	if (stat(src,&src_stat)==-1)
	{
    fprintf(stderr,"error: not such file or directory '%s'\n",src);
    return;
	}
  
	if (stat(dest,&stat_entry)!=-1)
  {
    if(interactive)
    {
      print("Overwrite '");
      print(dest);
      print("'? (y/n) ");
      c=read_char();
      if(c!='y' && c!='Y')
        return;
    }
    if(rmdir(dest)==-1)
    {
      fprintf(stderr,"error: directory '%s' is not empty\n",dest);
      return;
    }
  }  	
  if(mkdir(dest,src_stat.st_mode)==-1)
    fprintf(stderr,"error: unable to move file '%s' to '%s'\n",src,dest);
  else
  {
    dir = opendir(src); /* open src directory */
    if(dir==NULL)
      fprintf(stderr,"error: unable to open directory '%s'\n",src);
    else
    {
	    while((dir_entry=readdir(dir))!=NULL) /* read all directory entries */
		    if(strcmp(dir_entry->d_name,".") && strcmp(dir_entry->d_name,"..")) /* skip . and .. files */
		    {
          sprintf(srcpath,"%s/%s",src,dir_entry->d_name);
          sprintf(dstpath,"%s/%s",dest,dir_entry->d_name);
          mv_file(srcpath,dstpath,interactive,bext);
        }
      if(rmdir(src)==-1)
        fprintf(stderr,"error: directory '%s' is not empty\n",src);
    }
  }
}


/* executes the mv command */
void mv_command(char **args,int nargs)
{
  struct stat stat_entry;  
  char dest[100];
  char bext[100];
  char path[512];
  int interactive;
  char **files,*filename;
  int i,n;

  if(!get_mvarguments(args,nargs,&interactive,dest,bext))
    exit(EXIT_FAILURE);
  n=0;
  files=(char**)malloc(nargs*sizeof(char*));
  for(i=1; i<nargs; i++)
  {
    if(args[i][0]!=0)
      files[n++]=args[i];
  }
  if(dest[0])
  {
    if(n<1)
    {
      fprintf(stderr,"error: not enough parameters for mv\n");
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    if(n<2)
    {
      fprintf(stderr,"error: not enough parameters for mv\n");
      exit(EXIT_FAILURE);
    }
  }
 
  if(dest[0]) /* the destination is a directory */
  {
	  if(stat(dest, &stat_entry)==-1)
	    fprintf(stderr,"error: unable to open directory '%s'\n",dest);
    else if(S_ISDIR(stat_entry.st_mode)) /* if it's a directory */
    {
      for(i=0; i<n; i++)
      {
        filename=get_filename(files[i]);  /* get only the filename */
        sprintf(path,"%s/%s",dest,filename);
        mv_file(files[i],path,interactive,bext);
      }
    }
    else
      fprintf(stderr,"error: the destination '%s' is not a directory\n",dest);
  }
  else if(n>2)
  {
	  if(stat(files[n-1], &stat_entry)==-1)
	    fprintf(stderr,"error: unable to open directory '%s'\n",dest);
    else if(S_ISDIR(stat_entry.st_mode)) /* if it's a directory */
    {
      for(i=0; i<n-1; i++)
      {
        filename=get_filename(files[i]);  /* get only the filename */
        sprintf(path,"%s/%s",files[n-1],filename);
        mv_file(files[i],path,interactive,bext);
      }
    }
    else
      fprintf(stderr,"error: the destination '%s' is not a directory\n",dest);
  }
  else /* n==2 */
  {
	  if(stat(files[1], &stat_entry)==-1) /*doesn't exist */
      mv_file(files[0],files[1],interactive,bext);
    else if(S_ISDIR(stat_entry.st_mode)) /* if it's a directory */
    {
      filename=get_filename(files[0]);  /* get only the filename    */
      sprintf(path,"%s/%s",files[1],filename);
      mv_file(files[0],path,interactive,bext);
    }
    else if(stat(files[0], &stat_entry)!=-1) /*if the source exists */
    {
      if(!S_ISDIR(stat_entry.st_mode)) /* if source is not a directory */
        mv_file(files[0],files[1],interactive,bext);
      else
        fprintf(stderr,"error: the destination '%s' is not a directory\n",files[1]);
    }
    else
      fprintf(stderr,"error: not such file or directory '%s'\n",files[0]);
  }
  exit(EXIT_SUCCESS);
}

/* gets the options given to rm and saves them in the corresponding */
/* argument variables */
void get_rmarguments(char **args,int nargs,int *interactive,int *recursive,int *verbose)
{
  int i;
  *interactive=0;
  *recursive=0;
  *verbose=0;
  for(i=1; i<nargs; i++)
  {
    if(!strcmp(args[i],"-i")) /* interactive */
    {
      *interactive=1;
      args[i][0]=0;
    } 
    else if(!strcmp(args[i],"-r") || !strcmp(args[i],"-R")) /* recursive */
    {
      *recursive=1;
      args[i][0]=0;
    } 
    else if(!strcmp(args[i],"-v")) /* verbose */
    {
      *verbose=1;
      args[i][0]=0;
    }
  }
}

/* removes a file from the system, uses the given argument options to */
/* print the operation results or ask the user for confirmation */
void rm_file(char *file,int interactive,int recursive,int verbose)
{
  DIR * dir;
  struct dirent* dir_entry;
  struct stat stat_entry;  
  char c;
  char path[512];
  
	if (stat(file, &stat_entry)==-1)
	  fprintf(stderr,"error: unable to delete file '%s'\n",file);
  else
  {
    if(S_ISDIR(stat_entry.st_mode)) /* if it's a directory */
    {
      if(!recursive)
        fprintf(stderr,"error: unable to delete directory '%s'\n",file);
      else
      {
        if(interactive)
        {
          print("Descend into directory '");
          print(file);
          print("'? (y/n) ");
          c=read_char();
          if(c!='y' && c!='Y')
            return;
        }
  	    dir = opendir(file); /* open current directory */
  	    if(dir==NULL)
  	      fprintf(stderr,"error: unable to open directory '%s'\n",file);
        else
        {
    	    while((dir_entry=readdir(dir))!=NULL) /* read all directory entries */
    		    if(dir_entry->d_name[0]!='.') /* skip files starting with .        */
    		    {
              sprintf(path,"%s/%s",file,dir_entry->d_name);
              rm_file(path,interactive,recursive,verbose);
            }
          if(interactive)
          {
            print("Remove directory '");
            print(file);
            print("'? (y/n) ");
            c=read_char();
            if(c!='y' && c!='Y')
              return;
          }
          if(rmdir(file)==-1)
            fprintf(stderr,"error: directory '%s' is not empty\n",file);
          else if(verbose)
            printf("Removed directory '%s'\n",file);
        }
      }
    }
    else
    {
      if(interactive)
      {
        print("Remove file '");
        print(file);
        print("'? (y/n) ");
        c=read_char();
        if(c!='y' && c!='Y')
          return;
      }
      if(unlink(file)==-1)
        fprintf(stderr,"error: unable to remove file '%s'\n",file);
      else if(verbose)
          printf("Removed file '%s'\n",file);
    }
  }
}

/* executes the rm command */
void rm_command(char **args,int nargs)
{
  int interactive,recursive,verbose;
  char **files;
  int i,n;

  get_rmarguments(args,nargs,&interactive,&recursive,&verbose);

  n=0;
  files=(char**)malloc(nargs*sizeof(char*));
  for(i=1; i<nargs; i++)
  {
    if(args[i][0]!=0)
      files[n++]=args[i];
  }
  if(n<1)
  {
    fprintf(stderr,"error: not enough parameters for rm\n");
    exit(EXIT_FAILURE);
  }
  for(i=0; i<n; i++)
    rm_file(files[i],interactive,recursive,verbose);
  free(files);
  exit(EXIT_SUCCESS);
}

/* executes the help command */
void help_command(char **args,int nargs)
{
  printf("\n    Internal Command help.\n\n");
  printf("help        Displays this help screen\n");
  printf("exit        Exits the terminal\n");
  printf("dir <dir>   Display the contents of an existing directory\n");
  printf("mv [-i] [-t <dir>] [-S <suffix>] <file> <newfile>  Move a file to a new location\n");
  printf("rm [-i] [-r] [-R] [-v] <file>       Remove a file\n");
  printf("\n");
  exit(EXIT_SUCCESS);
}

/* executes a single command using the input and output redirection from the pipes */
int execute(char **args,int nargs,int pipe_in,int pipe_out)
{
  pid_t pid;
  int status;

  if(!strcmp(args[0],"exit"))
    return 1;
  pid=fork(); /* create a new process */
  if(pid<0)
  {
    fprintf(stderr,"error: fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if(pid==0) /* child process */
  {
    if(pipe_in!=-1)
    {
      dup2(pipe_in,STDIN_FILENO);
      close(pipe_in);      
    }
    if(pipe_out!=-1)
    {
      dup2(pipe_out,STDOUT_FILENO);
      close(pipe_out);
    }
    if(!strcmp(args[0],"dir"))
      dir_command(args,nargs);
    else if(!strcmp(args[0],"mv"))
      mv_command(args,nargs);
    else if(!strcmp(args[0],"rm"))
      rm_command(args,nargs);
    else if(!strcmp(args[0],"help"))
      help_command(args,nargs);
    else if(execvp(args[0],args)==-1)
    {
      fprintf(stderr,"error: invalid command: %s\n",args[0]);
      exit(EXIT_FAILURE);
    }
  }
  wait(&status);  /* wait for the child to end */
  return 0;
}

/* executes a piped command that could include a i/o redirection to a file */
int execute_command(char *command,int pipe_in,int pipe_out)
{
  char **args;
  char *infile,*outfile;
  int nargs;
  int is_exit;
  is_exit=0;
  set_redirection(command,&infile,&outfile);
  if(infile!=NULL)
  {
    if(pipe_in!=-1)
      close(pipe_in);
    pipe_in=open(infile,O_RDONLY);
  }   
  if(outfile!=NULL)
  {
    if(pipe_out!=-1)
      close(pipe_out);
    pipe_out=open(outfile,O_WRONLY | O_CREAT | O_TRUNC,0666);
  }
  args=split(command,' ','\t',&nargs); /* split the command by spaces */
  if(nargs>0)
      is_exit=execute(args,nargs,pipe_in,pipe_out);
  if(pipe_in!=-1)
    close(pipe_in);
  if(pipe_out!=-1)
    close(pipe_out);
  free(args);
  return is_exit;
}

/* executes a command line input, the command line could include a series of */
/* piped commands */
int execute_command_line(char *command_line)
{
  char **args;
  int nargs;
  int i;
  int is_exit;
  int **pipes;
  
  is_exit=0;
  args=split(command_line,'|','|',&nargs); /* split the command by pipes */
  if(nargs>0)
  {
    if(nargs==1)  /* only one command */
      is_exit=execute_command(args[0],-1,-1);
    else
    {
      pipes = (int**)malloc((nargs-1)*sizeof(int*));
      for(i=0; i<nargs-1; i++)
      {
        pipes[i]=(int*)malloc(2*sizeof(int));
        pipe(pipes[i]);
      }
      for(i=0; i<nargs && !is_exit; i++)
      {       
          if(i==0)
            is_exit=execute_command(args[i],-1,pipes[i][1]);
          else if(i==nargs-1)
            is_exit=execute_command(args[i],pipes[i-1][0],-1);
          else
            is_exit=execute_command(args[i],pipes[i-1][0],pipes[i][1]);
      } 
      for(i=0; i<nargs-1; i++)
        free(pipes[i]);
      free(pipes);
    }
  } 
  free(args);
  return is_exit;  
}

/* main function */
int main(int argc,char **argv)
{
    int exit_terminal=0;
    char *command_line;

    while(!exit_terminal)   /* repeat until the user exits the terminal    */
    {
        print("> ");   /* print the prompt */
        if((command_line=read_input())!=NULL) /* read the command line input */
        {
          exit_terminal=execute_command_line(command_line);
          free(command_line); /* free the allocated memory for the command line */
        }
        else 
          exit_terminal=1;
    }
    return (0);
}

