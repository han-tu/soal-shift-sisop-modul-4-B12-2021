#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

static  const  char *dirpath = "/home/zea/Downloads/";
static const char *logPath = "/home/zea/cobaprak4/SinSeiFS.log";
int mirrorOnReaddirHelper ;

void logging(int warn, char *cmd, const char *desc) {
  char buffer[80], mode[8];
  FILE * LOG = fopen(logPath, "a");
  time_t t;
  time(&t);
  struct tm *timeinfo = localtime(&t);
  //printf(".%02d%02d%02d:%d-%02d-%02d.\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday);

  if (!warn) strcpy(mode, "INFO");
  else strcpy(mode, "WARNING");
  //strftime(buffer, 80, "%y%m%d-%H:%M:%S", tm);
  sprintf(buffer, "%02d%02d%02d-%d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon+1,timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
  fprintf(LOG, "%s::%s:%s::%s\n", mode, buffer, cmd, desc);
  fclose(LOG);
}

char* mirror(char real[]) {
    char str[1024] ;
    strcpy(str, real) ;
    int i ;
    for(i = 0 ; i < strlen(str) ; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'Z' - (str[i] - 'A') ;
        }
        else if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'z' - (str[i] - 'a') ;
        }
    }
    char* res = str ;
    return res ;
}

void logRecord(char old_dir[], char new_dir[], int mode) {
    FILE* file = fopen("encode.log", "a") ;

    char str[2048] ;
    if (mode == 1) {
        sprintf(str, "(Rename)%s --> %s", old_dir, new_dir) ;

        fprintf(file, "%s\n", str) ;
    }
    else if (mode == 2) {
        sprintf(str, "(Mkdir)%s", new_dir) ;

        fprintf(file, "%s\n", str) ;
    }

    fclose(file) ;
}

char* find_fpath(char* path) {
    char fpath[1024] ;
    bzero(fpath, 1024) ;
    int i, mirrorOn = 0 ;

    // Cek Apakah ada /AtoZ_ atau nggak...
    char *ret;
    if (strcmp(path, "/") != 0) {
        
        ret = strstr(path, "/AtoZ_");
        if (ret) {
            // Jika ada, set mirrorOn = 1 
            mirrorOn = 1 ;
            mirrorOnReaddirHelper = 1 ;
            // Geser pointer kekanan agar / nya ga kena
            ret++ ;
        }
    }

    if(strcmp(path, "/") == 0)
    {
        path=dirpath;
        sprintf(fpath,"%s",path);
    } else if (mirrorOn) {
        char realPath[1024] ;
        bzero(realPath, 1024) ;
        strncpy(realPath, path, strlen(path) - strlen(ret)) ;

        char t[1024] ; strcpy(t, ret) ;
        char* token;
        char* rest = t;
        
        char temp[1024] ;
        i = 0 ;
        while ((token = strtok_r(rest, "/", &rest))) {
            bzero(temp, 1024) ;
            if(i == 0) {
                strcpy(temp, token) ;
                strcat(realPath, temp) ;
                
                i = 1 ;
                continue ;
            }
            // /abc/def/AtoZ_ghi/j.kl/mno.txt
            // Cek tipe data
            char checkType[1024] ;
            bzero(checkType, 1024) ;
            strcpy(checkType, realPath) ;
            strcat(checkType, "/") ; strcat(realPath, "/") ;
            strcat(checkType, token) ;

            if (strlen(checkType) == strlen(path)) {
                char pathFolder[1024] ;
                sprintf(pathFolder, "%s%s%s", dirpath, realPath, token) ;

                DIR *dp = opendir(pathFolder);
                if (!dp) {
                    char* dot ;
                    dot = strchr(token, '.') ;

                    char fileName[1024] ;
                    bzero(fileName, 1024) ;
                    if (dot) {
                        strncpy(fileName, token, strlen(token) - strlen(dot)) ;
                        strcpy(fileName, mirror(fileName)) ;
                        strcat(fileName, dot) ;
                    }
                    else {
                        strcpy(fileName, token) ;
                        strcpy(fileName, mirror(fileName)) ;
                    }
                    strcat(realPath, fileName) ;
                }
                else {
                    char folderName[1024] ;
                    bzero(folderName, 1024) ;
                    strcpy(folderName, token) ;
                    strcpy(folderName, mirror(folderName)) ;
                    strcat(realPath, folderName) ;
                }
            }
            else {
                char folderName[1024] ;
                bzero(folderName, 1024) ;
                strcpy(folderName, token) ;
                strcpy(folderName, mirror(folderName)) ;
                strcat(realPath, folderName) ;
            }

        }
        sprintf(fpath, "%s%s", dirpath, realPath) ; 
    }
    else sprintf(fpath, "%s%s",dirpath,path) ;
    
    char* return_fpath = fpath ;
    return return_fpath ;
}

/* XMP Field */
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    // path = /abcde
    char fpath[1000];
    bzero(fpath, 1000) ;
    mirrorOnReaddirHelper = 0 ;
    strcpy(fpath, find_fpath(path)) ;

    int res = 0 ;
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;

        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            res = (filler(buf, de->d_name, &st, 0)) ;
        }
        else if (mirrorOnReaddirHelper) {
            if (de->d_type & DT_DIR) {
                char temp[1024] ;
                bzero(temp, 1024) ;
                strcpy(temp, de->d_name) ;
                strcpy(temp, mirror(temp)) ;
                res = (filler(buf, temp, &st, 0));
            }
            else {
                char* dot ;
                dot = strchr(de->d_name, '.') ;
                
                char fileName[1024] ;
                bzero(fileName, 1024) ;
                if (dot) {
                    strncpy(fileName, de->d_name, strlen(de->d_name) - strlen(dot)) ;
                    strcpy(fileName, mirror(fileName)) ;
                    strcat(fileName, dot) ;
                }
                else {
                    strcpy(fileName, de->d_name) ;
                    strcpy(fileName, mirror(fileName)) ;
                }
                res = (filler(buf, fileName, &st, 0));
            }
        }
        else res = (filler(buf, de->d_name, &st, 0));
        
        if(res!=0) break;
	}

	closedir(dp);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
    char fpath[1000];
    bzero(fpath, 1000) ;
    strcpy(fpath, find_fpath(path)) ;
    
	int fd;
	int res;

	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    char fpath[1000];
    bzero(fpath, 1000) ;
    strcpy(fpath, find_fpath(path)) ;

	int res;
	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;
    
    //logging(0, "GETATTR", path);

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    char* lastSlash = strchr(to, '/') ;
    
    if (strstr(lastSlash, "/AtoZ_")) {
        char str[1024];
        char t1[1024] ; bzero(t1, 1024) ;
        char t2[1024] ; bzero(t2, 1024) ;
        sprintf(t1, "%s%s", dirpath, from) ;
        sprintf(t2, "%s%s", dirpath, to) ;
        //logRecord(t1, t2, 1) ;
    }

    char f_from[1024] ; char f_to[1024] ; char str[100] ;
    bzero(f_from, 1024) ; bzero(f_to, 1024) ;
    strcpy(f_from, find_fpath(from)) ;
    strcpy(f_to, find_fpath(to)) ;

	int res;

	res = rename(f_from, f_to);
	if (res == -1)
		return -errno;

    sprintf(str, "%s::%s", from, to);
    logging(0, "RENAME", str);
    
	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    char* lastSlash = strrchr(path, '/') ;
    if (strstr(lastSlash, "/AtoZ_")) {
        char temp[1024] ; bzero(temp, 1024) ;
        sprintf(temp, "%s%s", dirpath, path) ;
        //logRecord("gapenting", temp, 2) ;
        
    }

    char fpath[1024] ;
    bzero(fpath, 1024) ;
    strcpy(fpath, find_fpath(path)) ;
	int res;

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

    logging(0, "MKDIR", path);

	return 0;
}

static int xmp_rmdir(const char *path)
{
    char fpath[1024] ;
    bzero(fpath, 1024) ;
    strcpy(fpath, find_fpath(path)) ;
	int res;

    res = rmdir(fpath);
	if (res == -1)
		return -errno;
    
    logging(1, "RMDIR", path);

	return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t ofdirpathet, struct fuse_file_info *fi)
{
  char fpath[1000];
  sprintf(fpath, "%s%s", dirpath, path);
  int fd, res;

  (void) fi;
  fd = open(fpath, O_WRONLY);
  if (fd == -1) return -errno;

  res = pwrite(fd, buf, size, ofdirpathet);
  if (res == -1) res = -errno;
  logging(0, "WRITE", path);
  close(fd);

  return res;
}

static int xmp_unlink(const char *path)
{
    char fpath[1000];
  sprintf(fpath, "%s%s", dirpath, path);
	int res;

	res = unlink(fpath);
	if (res == -1)
		return -errno;
    
    logging(1, "UNLINK", path);

	return 0;
}


static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .rename = xmp_rename,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .write = xmp_write,
    .unlink = xmp_unlink,
};

/* End XMP Field */

int  main(int  argc, char *argv[])
{
    
    umask(0);

    return fuse_main(argc, argv, &xmp_oper, NULL);
}