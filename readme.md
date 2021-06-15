# soal-shift-sisop-modul-4-B12-2021
## Nomor 1
### Membuat fungsi untuk enkripsi (dan dekripsi) atbash cipher
Tinggal menambah kurangkan nilai ascii sehingga didapat kode sebagai berikut
```c
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
```

### Membuat Fungsi untuk mengkonversi path fuse menjadi absolute path
1. Jika tidak ada substring "AtoZ_" dalam string path nya, maka path dikonversi seperti biasa
2. Sebaliknya :

a. Dilakukan pemrosesan tiap kata yang dipisahkan oleh karakter "/"
b. Tiap kata dicek apakah merupakan file ataukah folder
c. Jika folder, lakukan atbash pada kata tersebut dan sambungkan ke variabel real path
d. Jika file, cek apakah memiliki extension atau tidak
e. Jika tidak, lakukan atbash pada kata tersebut dan sambungkan ke variabel real path
f. Jika iya, hanya lakukan atbash cipher pada nama file nya saja (tidak mengikutsertakan extension)
g. Jika semua kata pada path sudah di proses.. sambungkan Path /home/[user]/Downloads dengan real path

Kendala : 
1. Sring sekali gagal menyocokkan path nya
2. Bingung membedakan file dengan folder

### Mengimplementasikan Readdir
Readdir di implementasikan dengan adanya modifikasi dari fuse biasa :
1. Path yang didapat di awal fungsi di konversi ke real absolute path
2. Ketika melisting isi folder tersebut, lakukan pengecekan yang hampir mirip dengan saat mengkonversi path fuse ke real absolute path
3. Jika folder yang sedang dilisting, tinggal lakukan atbash cipher pada folder tersebut
4. Jika file yang sedang dilisting, lakukan atbash hanya pada nama file nya (jika memiliki ekstensi)

Implementasi :
```c
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
```

Kendala :
1. Mirip dengan poin sebelumnya

### Mengimplementasikan Read, getattr, rename, dan mkdir
ke-4 fungsi tersebut di implementasikan dengan memodifikasi path yang masuk menjadi real absolute path menggunakan fungsi yang telah dibuat. Tidak ada perubahan signifikan dari kode asalnya
```c
char fpath[1000];
bzero(fpath, 1000) ;
strcpy(fpath, find_fpath(path)) ;
```
Hanya menambahkan perintah diatas diawal fungsi, dan merubah path yang diproses menjadi fpath

Tidak ada kendala pada poin ini.

### Mengimplementasikan Record Log untuk Fungsi rename dan mkdir
Penulisan memiliki 2 mode, yaitu mode "rename" (pada kode mode 1) dan mode "mkdir" (pada kode mode 2).

Pada mode pertama penulisan dilakukan dengan format :
```
(Rename)nama_awal --> nama_akhir
```

dan Pada mode kedua :
```
(Mkdir)nama_folder
```

Implementasi :
```c
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
```

Implementasi pada fungsi rename dan mkdir :
```c
char* lastSlash = strchr(to, '/') ;
if (strstr(lastSlash, "/AtoZ_")) {
    char str[1024];
    char t1[1024] ; bzero(t1, 1024) ;
    char t2[1024] ; bzero(t2, 1024) ;
    sprintf(t1, "%s%s", dirpath, from) ;
    sprintf(t2, "%s%s", dirpath, to) ;
    logRecord(t1, t2, 1) ;
}
```

Tidak ada kendala pada poin ini.

## Nomor 4

### Membuat log system

Fungsi untuk membuat log sebagai berikut:
```c
static const char *logPath = "/home/zea/cobaprak4/SinSeiFS.log";

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
```

Penjelasan:\
Pertanyaan a\
```FILE * LOG = fopen(logPath, "a");``` untuk menyimpan log dalam file
SinSeiFS.log. ```"a"``` berarti jika file sudah ada dan sudah terdapat
data di dalam file tersebut, maka data baru akan diappend.

Pertanyaan b
```
if (!warn) strcpy(mode, "INFO");
    else strcpy(mode, "WARNING");
``` 
digunakan variabel warn untuk membedakan level log INFO atau
WARNING.

Pertanyaan e
```
time_t t;
time(&t);
struct tm *timeinfo = localtime(&t);
``` 
untuk menyimpan data waktu.
```
sprintf(buffer, "%02d%02d%02d-%d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon+1,timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
fprintf(LOG, "%s::%s:%s::%s\n", mode, buffer, cmd, desc);
``` 
untuk format log sesuai yang diminta pertanyaan.

### Mencatat system call

Memanggil fungsi log tiap system call:
```logging([level log], "[system call yang terpanggil]", [informasi / parameter tambahan]);
```

Penjelasan:\
Pertanyaan c\
Level WARNING untuk rmdir
```
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
```
Level WARNING untuk unlink
```
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
```

Pertanyaan d\
Level INFO untuk rename
```
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
```
Level INFO untuk mkdir
```
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
```
Level INFO untuk write
```
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
```

### Hasil dan screenshot

![ss1](https://imgur.com/4s1brVb.png)
![ss2](https://imgur.com/tGCuoIJ.png)

### Kendala

1. Terdapat error ketika compile yang tidak saya sadari
2. File log tidak muncul dimana mana
3. File log muncul, tetapi tidak dapat melakukan system call
4. Terjebak di dalam infinity loop
5. Fungsi write masih belum bisa
