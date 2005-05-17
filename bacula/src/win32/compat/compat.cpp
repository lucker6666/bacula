//                              -*- Mode: C++ -*-
// compat.cpp -- compatibilty layer to make bacula-fd run
//               natively under windows
//
// Copyright transferred from Raider Solutions, Inc to
//   Kern Sibbald and John Walker by express permission.
//
// Copyright (C) 2004-2005 Kern Sibbald
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free
//   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//   MA 02111-1307, USA.
//
// Author          : Christopher S. Hull
// Created On      : Sat Jan 31 15:55:00 2004
// $Id$

#include "bacula.h"

#ifdef WIN32_VSS
#include "vss.h"
#endif 

#include "../../lib/winapi.h"


/* to allow the usage of the original version in this file here */
#undef fputs

#define b_errno_win32 (1<<29)

#define USE_WIN32_COMPAT_IO 1

extern void d_msg(const char *file, int line, int level, const char *fmt,...);
extern DWORD   g_platform_id;

// from MicroSoft SDK (KES) is the diff between Jan 1 1601 and Jan 1 1970
#ifdef HAVE_MINGW
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000UL //Not sure it works
#else
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000I64
#endif

#define WIN32_FILETIME_SCALE  10000000             // 100ns/second

extern "C" void
cygwin_conv_to_win32_path(const char *name, char *win32_name, DWORD dwSize)
{
    const char *fname = name;
    char *tname = win32_name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    if (*fname != 0 && win32_name[-1] == '\\') {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }

#ifdef WIN32_VSS
    /* here we convert to VSS specific file name */    
    char szFile[MAX_PATH_UNICODE];
    strncpy (szFile, tname, MAX_PATH_UNICODE-1);
    g_VSSClient.GetShadowPath(szFile,tname,dwSize);
#endif
}

int 
wchar_2_UTF8(char *pszUTF, const WCHAR *pszUCS, int cchChar)
{
   /* the return value is the number of bytes written to the buffer. 
      The number includes the byte for the null terminator. */

   if (p_WideCharToMultiByte) {
         int nRet = p_WideCharToMultiByte(CP_UTF8,0,pszUCS,-1,pszUTF,cchChar,NULL,NULL);
         ASSERT (nRet > 0);
         return nRet;
      }
   else
      return NULL;
}

int 
UTF8_2_wchar(POOLMEM **ppszUCS, const char *pszUTF)
{
   /* the return value is the number of wide characters written to the buffer. */
   /* convert null terminated string from utf-8 to ucs2, enlarge buffer if necessary */

   if (p_MultiByteToWideChar) {
      /* strlen of UTF8 +1 is enough */ 
      DWORD cchSize = (strlen(pszUTF)+1);
      *ppszUCS = check_pool_memory_size(*ppszUCS, cchSize*sizeof (WCHAR));
      
      int nRet = p_MultiByteToWideChar(CP_UTF8, 0, pszUTF, -1, (LPWSTR) *ppszUCS,cchSize);
      ASSERT (nRet > 0);
      return nRet;
   }
   else
      return NULL;
}


void
wchar_win32_path(const char *name, WCHAR *win32_name)
{
    const char *fname = name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    if (*fname != 0 && win32_name[-1] == '\\') {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }
}

int umask(int)
{
   return 0;
}

int chmod(const char *, mode_t)
{
   return 0;
}

int chown(const char *k, uid_t, gid_t)
{
   return 0;
}

int lchown(const char *k, uid_t, gid_t)
{
   return 0;
}

#ifdef needed
bool fstype(const char *fname, char *fs, int fslen)
{
   return true;                       /* accept anything */
}
#endif


long int
random(void)
{
    return rand();
}

void
srandom(unsigned int seed)
{
   srand(seed);
}
// /////////////////////////////////////////////////////////////////
// convert from Windows concept of time to Unix concept of time
// /////////////////////////////////////////////////////////////////
void
cvt_utime_to_ftime(const time_t  &time, FILETIME &wintime)
{
    uint64_t mstime = time;
    mstime *= WIN32_FILETIME_SCALE;
    mstime += WIN32_FILETIME_ADJUST;

    #ifdef HAVE_MINGW
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffUL);
    #else
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffI64);
    #endif
    wintime.dwHighDateTime = (DWORD) ((mstime>>32)& 0xffffffffUL);
}

time_t
cvt_ftime_to_utime(const FILETIME &time)
{
    uint64_t mstime = time.dwHighDateTime;
    mstime <<= 32;
    mstime |= time.dwLowDateTime;

    mstime -= WIN32_FILETIME_ADJUST;
    mstime /= WIN32_FILETIME_SCALE; // convert to seconds.

    return (time_t) (mstime & 0xffffffff);
}

static const char *
errorString(void)
{
   LPVOID lpMsgBuf;

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL);

   return (const char *) lpMsgBuf;
}

#ifndef HAVE_MINGW

static int
statDir(const char *file, struct stat *sb)
{
   WIN32_FIND_DATAW info_w;       // window's file info
   WIN32_FIND_DATAA info_a;       // window's file info

   // cache some common vars to make code more transparent
   DWORD* pdwFileAttributes;
   DWORD* pnFileSizeHigh;
   DWORD* pnFileSizeLow;
   FILETIME* pftLastAccessTime;
   FILETIME* pftLastWriteTime;
   FILETIME* pftCreationTime;

   if (file[1] == ':' && file[2] == 0) {
        d_msg(__FILE__, __LINE__, 99, "faking ROOT attrs(%s).\n", file);
        sb->st_mode = S_IFDIR;
        sb->st_mode |= S_IREAD|S_IEXEC|S_IWRITE;
        time(&sb->st_ctime);
        time(&sb->st_mtime);
        time(&sb->st_atime);
        return 0;
    }

   HANDLE h = INVALID_HANDLE_VALUE;

   // use unicode or ascii
   if (p_FindFirstFileW) {
      POOLMEM* pwszBuf = get_pool_memory (PM_FNAME);         
      UTF8_2_wchar(&pwszBuf, file);
      
      h = p_FindFirstFileW((LPCWSTR) pwszBuf, &info_w);
      free_pool_memory(pwszBuf);

      pdwFileAttributes = &info_w.dwFileAttributes;
      pnFileSizeHigh    = &info_w.nFileSizeHigh;
      pnFileSizeLow     = &info_w.nFileSizeLow;
      pftLastAccessTime = &info_w.ftLastAccessTime;
      pftLastWriteTime  = &info_w.ftLastWriteTime;
      pftCreationTime   = &info_w.ftCreationTime;
   }
   else if (p_FindFirstFileA) {
      h = p_FindFirstFileA(file, &info_a);

      pdwFileAttributes = &info_a.dwFileAttributes;
      pnFileSizeHigh    = &info_a.nFileSizeHigh;
      pnFileSizeLow     = &info_a.nFileSizeLow;
      pftLastAccessTime = &info_a.ftLastAccessTime;
      pftLastWriteTime  = &info_a.ftLastWriteTime;
      pftCreationTime   = &info_a.ftCreationTime;
   }

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99, "FindFirstFile(%s):%s\n", file, err);
        LocalFree((void *)err);
        errno = b_errno_win32;
        return -1;
    }

    sb->st_mode = 0777;               /* start with everything */
    if (*pdwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (*pdwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (*pdwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
    sb->st_mode |= S_IFDIR;

    sb->st_size = *pnFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= *pnFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;

    sb->st_atime = cvt_ftime_to_utime(*pftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(*pftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(*pftCreationTime);
    FindClose(h);

    return 0;
}

static int
stat2(const char *file, struct stat *sb)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int rval = 0;
    char tmpbuf[1024];
    cygwin_conv_to_win32_path(file, tmpbuf, 1024);

    DWORD attr = -1;

    if (p_GetFileAttributesW) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);
      UTF8_2_wchar(&pwszBuf, tmpbuf);
      
      attr = p_GetFileAttributesW((LPCWSTR) pwszBuf);
      free_pool_memory(pwszBuf);
    }
    else if (p_GetFileAttributesA) {
       attr = p_GetFileAttributesA(tmpbuf);
    }

    if (attr == -1) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetFileAttrubtes(%s): %s\n", tmpbuf, err);
        LocalFree((void *)err);
        errno = b_errno_win32;
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return statDir(tmpbuf, sb);

    h = CreateFileA(tmpbuf, GENERIC_READ,
                   FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "Cannot open file for stat (%s):%s\n", tmpbuf, err);
        LocalFree((void *)err);
        rval = -1;
        errno = b_errno_win32;
        goto error;
    }

    if (!GetFileInformationByHandle(h, &info)) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetfileInformationByHandle(%s): %s\n", tmpbuf, err);
        LocalFree((void *)err);
        rval = -1;
        errno = b_errno_win32;
        goto error;
    }

    sb->st_dev = info.dwVolumeSerialNumber;
    sb->st_ino = info.nFileIndexHigh;
    sb->st_ino <<= 32;
    sb->st_ino |= info.nFileIndexLow;
    sb->st_nlink = (short)info.nNumberOfLinks;
    if (sb->st_nlink > 1) {
       d_msg(__FILE__, __LINE__, 99,  "st_nlink=%d\n", sb->st_nlink);
    }

    sb->st_mode = 0777;               /* start with everything */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
    sb->st_mode |= S_IFREG;

    sb->st_size = info.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= info.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
    sb->st_atime = cvt_ftime_to_utime(info.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(info.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(info.ftCreationTime);

error:
    CloseHandle(h);
    return rval;
}

int
stat(const char *file, struct stat *sb)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    errno = 0;


    memset(sb, 0, sizeof(*sb));

    if (g_platform_id == VER_PLATFORM_WIN32_WINDOWS)
        return stat2(file, sb);

    // otherwise we're on NT
#if 0
    WCHAR buf[32767];
    buf[0] = '\\';
    buf[1] = '\\';
    buf[2] = '?';
    buf[3] = '\\';

    wchar_win32_path(file, buf+4);

    if (!GetFileAttributesExW((WCHAR *)buf, GetFileExInfoStandard, &data))
        return stat2(file, sb);
#else

    if (p_GetFileAttributesExW) {
      /* dynamically allocate enough space for UCS2 filename */
      POOLMEM* pwszBuf = get_pool_memory (PM_FNAME);          
      UTF8_2_wchar(&pwszBuf, file);

      BOOL b = p_GetFileAttributesExW((LPCWSTR) pwszBuf, GetFileExInfoStandard, &data);
      free_pool_memory(pwszBuf);
                        
      if (!b) 
         return stat2(file, sb);
      
    } else if (p_GetFileAttributesExA) {
         if (!p_GetFileAttributesExA(file, GetFileExInfoStandard, &data))
            return stat2(file, sb);
    }
    else
       return stat2(file, sb);

#endif

    sb->st_mode = 0777;               /* start with everything */
    if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        sb->st_mode |= S_IFDIR;
    else
        sb->st_mode |= S_IFREG;

    sb->st_nlink = 1;
    sb->st_size = data.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= data.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
    sb->st_atime = cvt_ftime_to_utime(data.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(data.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(data.ftCreationTime);
    return 0;
}

#endif //HAVE_MINGW

int
lstat(const char *file, struct stat *sb)
{
    return stat(file, sb);
}

void
sleep(int sec)
{
    Sleep(sec * 1000);
}

int
geteuid(void)
{
    return 0;
}

int
execvp(const char *, char *[]) {
    errno = ENOSYS;
    return -1;
}


int
fork(void)
{
    errno = ENOSYS;
    return -1;
}

int
pipe(int[])
{
    errno = ENOSYS;
    return -1;
}

int
waitpid(int, int*, int)
{
    errno = ENOSYS;
    return -1;
}

int
readlink(const char *, char *, int)
{
    errno = ENOSYS;
    return -1;
}


#ifndef HAVE_MINGW
int
strcasecmp(const char *s1, const char *s2)
{
    register int ch1, ch2;

    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    do
    {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
    } while (ch1 != 0 && tolower(ch1) == tolower(ch2));

  return(ch1 - ch2);
}
#endif //HAVE_MINGW

int
strncasecmp(const char *s1, const char *s2, int len)
{
    register int ch1, ch2;

    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    while (len--)
    {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
        if (ch1 == 0 || tolower(ch1) != tolower(ch2)) break;
    } 

    return (ch1 - ch2);
}

int
gettimeofday(struct timeval *tv, struct timezone *)
{
    SYSTEMTIME now;
    FILETIME tmp;
    GetSystemTime(&now);

    if (tv == NULL) {
       errno = EINVAL;
       return -1;
    }
    if (!SystemTimeToFileTime(&now, &tmp)) {
       errno = b_errno_win32;
       return -1;
    }

    int64_t _100nsec = tmp.dwHighDateTime;
    _100nsec <<= 32;
    _100nsec |= tmp.dwLowDateTime;
    _100nsec -= WIN32_FILETIME_ADJUST;

    tv->tv_sec =(long) (_100nsec / 10000000);
    tv->tv_usec = (long) ((_100nsec % 10000000)/10);
    return 0;

}

int
syslog(int type, const char *fmt, const char *msg)
{
/*#ifndef HAVE_CONSOLE
    MessageBox(NULL, msg, "Bacula", MB_OK);
#endif*/
    return 0;
}

struct passwd *
getpwuid(uid_t)
{
    return NULL;
}

struct group *
getgrgid(uid_t)
{
    return NULL;
}

// implement opendir/readdir/closedir on top of window's API

typedef struct _dir
{
    WIN32_FIND_DATAA data_a;    // window's file info (ansii version)
    WIN32_FIND_DATAW data_w;    // window's file info (wchar version)
    const char *spec;           // the directory we're traversing
    HANDLE      dirh;           // the search handle
    BOOL        valid_a;        // the info in data_a field is valid
    BOOL        valid_w;        // the info in data_w field is valid
    UINT32      offset;         // pseudo offset for d_off
} _dir;

DIR *
opendir(const char *path)
{
    /* enough space for VSS !*/
    int max_len = strlen(path) + MAX_PATH;
    _dir *rval = NULL;
    if (path == NULL) {
       errno = ENOENT;
       return NULL;
    }

    rval = (_dir *)malloc(sizeof(_dir));
    memset (rval, 0, sizeof (_dir));
    if (rval == NULL) return NULL;
    char *tspec = (char *)malloc(max_len);
    if (tspec == NULL) return NULL;

    if (g_platform_id != VER_PLATFORM_WIN32_WINDOWS) {
        // allow path to be 32767 bytes
        tspec[0] = '\\';
        tspec[1] = '\\';
        tspec[2] = '?';
        tspec[3] = '\\';
        tspec[4] = 0;
        cygwin_conv_to_win32_path(path, tspec+4, max_len-4);
    } else {
        cygwin_conv_to_win32_path(path, tspec, max_len);
    }

    strncat(tspec, "\\*", max_len);
    rval->spec = tspec;

    // convert to WCHAR
    if (p_FindFirstFileW) {
      POOLMEM* pwcBuf = get_pool_memory(PM_FNAME);;
      UTF8_2_wchar(&pwcBuf,rval->spec);
      rval->dirh = p_FindFirstFileW((LPCWSTR)pwcBuf, &rval->data_w);   

      free_pool_memory(pwcBuf);

      if (rval->dirh != INVALID_HANDLE_VALUE)
        rval->valid_w = 1;
    } else if (p_FindFirstFileA) {
      rval->dirh = p_FindFirstFileA(rval->spec, &rval->data_a);
      
      if (rval->dirh != INVALID_HANDLE_VALUE)
        rval->valid_a = 1;
    } else goto err;


    d_msg(__FILE__, __LINE__,
          99, "opendir(%s)\n\tspec=%s,\n\tFindFirstFile returns %d\n",
          path, rval->spec, rval->dirh);

    rval->offset = 0;
    if (rval->dirh == INVALID_HANDLE_VALUE)
        goto err;

    if (rval->valid_w)
      d_msg(__FILE__, __LINE__,
            99, "\tFirstFile=%s\n", rval->data_w.cFileName);

    if (rval->valid_a)
      d_msg(__FILE__, __LINE__,
            99, "\tFirstFile=%s\n", rval->data_a.cFileName);

    return (DIR *)rval;

err:
    free((void *)rval->spec);
    free(rval);
    errno = b_errno_win32;
    return NULL;
}

int
closedir(DIR *dirp)
{
    _dir *dp = (_dir *)dirp;
    FindClose(dp->dirh);
    free((void *)dp->spec);
    free((void *)dp);
    return 0;
}

/*
  typedef struct _WIN32_FIND_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    TCHAR cFileName[MAX_PATH];
    TCHAR cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA;
*/

static int
copyin(struct dirent &dp, const char *fname)
{
    dp.d_ino = 0;
    dp.d_reclen = 0;
    char *cp = dp.d_name;
    while (*fname) {
        *cp++ = *fname++;
        dp.d_reclen++;
    }
        *cp = 0;
    return dp.d_reclen;
}

int
readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    _dir *dp = (_dir *)dirp;
    if (dp->valid_w || dp->valid_a) {
      entry->d_off = dp->offset;
                
      // copy unicode
      if (dp->valid_w) {
         char szBuf[MAX_PATH_UTF8];
         wchar_2_UTF8(szBuf,dp->data_w.cFileName);
         dp->offset += copyin(*entry, szBuf);
      } else if (dp->valid_a) { // copy ansi (only 1 will be valid)
         dp->offset += copyin(*entry, dp->data_a.cFileName);
      }
                
      *result = entry;              /* return entry address */
      d_msg(__FILE__, __LINE__,
            99, "readdir_r(%p, { d_name=\"%s\", d_reclen=%d, d_off=%d\n",
            dirp, entry->d_name, entry->d_reclen, entry->d_off);
    } else {
//      d_msg(__FILE__, __LINE__, 99, "readdir_r !valid\n");
        errno = b_errno_win32;
        return -1;
    }

    // get next file, try unicode first
    if (p_FindNextFileW) 
       dp->valid_w = p_FindNextFileW(dp->dirh, &dp->data_w);
    else if (p_FindNextFileA)
       dp->valid_a = p_FindNextFileA(dp->dirh, &dp->data_a);
    else {
       dp->valid_a = FALSE;
       dp->valid_w = FALSE;
    }

    return 0;
}

/*
 * Dotted IP address to network address
 *
 * Returns 1 if  OK
 *         0 on error
 */
int
inet_aton(const char *a, struct in_addr *inp)
{
   const char *cp = a;
   uint32_t acc = 0, tmp = 0;
   int dotc = 0;

   if (!isdigit(*cp)) {         /* first char must be digit */
      return 0;                 /* error */
   }
   do {
      if (isdigit(*cp)) {
         tmp = (tmp * 10) + (*cp -'0');
      } else if (*cp == '.' || *cp == 0) {
         if (tmp > 255) {
            return 0;           /* error */
         }
         acc = (acc << 8) + tmp;
         dotc++;
         tmp = 0;
      } else {
         return 0;              /* error */
      }
   } while (*cp++ != 0);
   if (dotc != 4) {              /* want 3 .'s plus EOS */
      return 0;                  /* error */
   }
   inp->s_addr = htonl(acc);     /* store addr in network format */
   return 1;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    Sleep((req->tv_sec * 1000) + (req->tv_nsec/100000));
    return 0;
}

void
init_signals(void terminate(int sig))
{

}

void
init_stack_dump(void)
{

}


long
pathconf(const char *path, int name)
{
    switch(name) {
    case _PC_PATH_MAX :
        if (strncmp(path, "\\\\?\\", 4) == 0)
            return 32767;
    case _PC_NAME_MAX :
        return 255;
    }
    errno = ENOSYS;                                                         
    return -1;
}

int
WSA_Init(void)
{
    WORD wVersionRequested = MAKEWORD( 1, 1);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);


    if (err != 0) {
        printf("Can not start Windows Sockets\n");
        errno = ENOSYS;
        return -1;
    }

    return 0;
}


int
win32_chdir(const char *dir)
{
   if (p_SetCurrentDirectoryW) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);
      UTF8_2_wchar(&pwszBuf, dir);

      BOOL b=p_SetCurrentDirectoryW((LPCWSTR)pwszBuf);
      free_pool_memory(pwszBuf);

      if (!b) {
         errno = b_errno_win32;
         return -1;
      }
   }
   else if (p_SetCurrentDirectoryA) {
      if (0 == p_SetCurrentDirectoryA(dir)) {
         errno = b_errno_win32;
         return -1;
      }
   }
   else return -1;
   
   return 0;
}

int
win32_mkdir(const char *dir)
{
   if (p_wmkdir){
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);
      UTF8_2_wchar(&pwszBuf, dir);

      int n=p_wmkdir((LPCWSTR)pwszBuf);
      free_pool_memory(pwszBuf);      
      return n;
   }

   return _mkdir(dir);        
}


char *
win32_getcwd(char *buf, int maxlen)
{
   int n=0;

   if (p_GetCurrentDirectoryW) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);
      pwszBuf = check_pool_memory_size (pwszBuf, maxlen*sizeof(WCHAR));

      n = p_GetCurrentDirectoryW(maxlen, (LPWSTR) pwszBuf);
      n = wchar_2_UTF8 (buf, (WCHAR*)pwszBuf, maxlen)-1;
      free_pool_memory(pwszBuf);

   } else if (p_GetCurrentDirectoryA)
      n = p_GetCurrentDirectoryA(maxlen, buf);

   if (n == 0 || n > maxlen) return NULL;

   if (n+1 > maxlen) return NULL;
   if (n != 3) {
       buf[n] = '\\';
       buf[n+1] = 0;
   }
   return buf;
}

int 
win32_fputs(const char *string, FILE *stream)
{
   /* we use WriteConsoleA / WriteConsoleA
      so we can be sure that unicode support works on win32.
      with fallback if something fails
   */   

   HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
   if (hOut && (hOut != INVALID_HANDLE_VALUE) && p_WideCharToMultiByte && 
       p_MultiByteToWideChar && (stream == stdout)) {
      
      POOLMEM* pwszBuf = get_pool_memory(PM_MESSAGE);
      
      DWORD dwCharsWritten;
      DWORD dwChars;
         
      dwChars = UTF8_2_wchar(&pwszBuf, string);

      /* try WriteConsoleW */
      if (WriteConsoleW (hOut, pwszBuf, dwChars-1, &dwCharsWritten, NULL)) {
         free_pool_memory(pwszBuf);
         return dwCharsWritten;   
      }
      
      /* convert to local codepage and try WriteConsoleA */      
      POOLMEM* pszBuf = get_pool_memory(PM_MESSAGE);         
      pszBuf = check_pool_memory_size(pszBuf, dwChars+1);

      dwChars = p_WideCharToMultiByte(GetConsoleOutputCP(),0,(LPCWSTR) pwszBuf,-1,pszBuf,dwChars,NULL,NULL);      
      free_pool_memory(pwszBuf);

      if (WriteConsoleA (hOut, pszBuf, dwChars-1, &dwCharsWritten, NULL)) {
         free_pool_memory(pszBuf);
         return dwCharsWritten;   
      }
   }

   return fputs(string, stream);
}

char*
win32_cgets (char* buffer, int len)
{
   /* we use console ReadConsoleA / ReadConsoleW to be able to read unicode
      from the win32 console and fallback if seomething fails */

   HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
   if (hIn && (hIn != INVALID_HANDLE_VALUE) && p_WideCharToMultiByte && p_MultiByteToWideChar) {            
      DWORD dwRead;
      WCHAR wszBuf[1024];
      char  szBuf[1024];    

      /* nt and unicode conversion */
      if (ReadConsoleW (hIn, wszBuf, 1024, &dwRead, NULL)) {

         /* null terminate at end */                
         if (wszBuf[dwRead-1] == L'\n') {
            wszBuf[dwRead-1] = L'\0';
            dwRead --;
         }

         if (wszBuf[dwRead-1] == L'\r') {
            wszBuf[dwRead-1] = L'\0';
            dwRead --;
         }
         
         wchar_2_UTF8(buffer, wszBuf, len);
         return buffer;
      }    
      
      /* win 9x and unicode conversion */
      if (ReadConsoleA (hIn, szBuf, 1024, &dwRead, NULL)) {

         /* null terminate at end */                
         if (szBuf[dwRead-1] == L'\n') {
            szBuf[dwRead-1] = L'\0';
            dwRead --;
         }

         if (szBuf[dwRead-1] == L'\r') {
            szBuf[dwRead-1] = L'\0';
            dwRead --;
         }

         /* convert from ansii to WCHAR */
         p_MultiByteToWideChar(GetConsoleCP(), 0, szBuf, -1, wszBuf,1024);
         /* convert from WCHAR to UTF-8 */
         if (wchar_2_UTF8(buffer, wszBuf, len))
            return buffer;         
      }
   }

   /* fallback */
   if (fgets(buffer, len, stdin)) 
      return buffer;   
   else
      return NULL;
}

int
win32_unlink(const char *filename)
{
   int nRetCode;
   if (p_wunlink) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);      
      UTF8_2_wchar(&pwszBuf, filename);
      nRetCode = _wunlink((LPCWSTR) pwszBuf);
      free_pool_memory(pwszBuf);
   } else {
      nRetCode = _unlink(filename);
   }
   return nRetCode;
}


#include "mswinver.h"

char WIN_VERSION_LONG[64];
char WIN_VERSION[32];

class winver {
public:
    winver(void);
};

static winver INIT;                     // cause constructor to be called before main()

#include "bacula.h"
#include "jcr.h"

winver::winver(void)
{
    const char *version = "";
    const char *platform = "";
    OSVERSIONINFO osvinfo;
    osvinfo.dwOSVersionInfoSize = sizeof(osvinfo);

    // Get the current OS version
    if (!GetVersionEx(&osvinfo)) {
        version = "Unknown";
        platform = "Unknown";
    }
    else
        switch (_mkversion(osvinfo.dwPlatformId, osvinfo.dwMajorVersion, osvinfo.dwMinorVersion))
        {
        case MS_WINDOWS_95: (version =  "Windows 95"); break;
        case MS_WINDOWS_98: (version =  "Windows 98"); break;
        case MS_WINDOWS_ME: (version =  "Windows ME"); break;
        case MS_WINDOWS_NT4:(version =  "Windows NT 4.0"); platform = "NT"; break;
        case MS_WINDOWS_2K: (version =  "Windows 2000");platform = "NT"; break;
        case MS_WINDOWS_XP: (version =  "Windows XP");platform = "NT"; break;
        case MS_WINDOWS_S2003: (version =  "Windows Server 2003");platform = "NT"; break;
        default: version = "Windows ??"; break;
        }

    bstrncpy(WIN_VERSION_LONG, version, sizeof(WIN_VERSION_LONG));
    snprintf(WIN_VERSION, sizeof(WIN_VERSION), "%s %d.%d.%d",
             platform, osvinfo.dwMajorVersion, osvinfo.dwMinorVersion, osvinfo.dwBuildNumber);

#if 0
    HANDLE h = CreateFile("G:\\foobar", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(h);
#endif
#if 0
    BPIPE *b = open_bpipe("ls -l", 10, "r");
    char buf[1024];
    while (!feof(b->rfd)) {
        fgets(buf, sizeof(buf), b->rfd);
    }
    close_bpipe(b);
#endif
}

BOOL CreateChildProcess(VOID);
VOID WriteToPipe(VOID);
VOID ReadFromPipe(VOID);
VOID ErrorExit(LPCSTR);
VOID ErrMsg(LPTSTR, BOOL);

/**
 * Check for a quoted path,  if an absolute path name is given and it contains
 * spaces it will need to be quoted.  i.e.  "c:/Program Files/foo/bar.exe"
 * CreateProcess() says the best way to ensure proper results with executables
 * with spaces in path or filename is to quote the string.
 */
const char *
getArgv0(const char *cmdline)
{

    int inquote = 0;
    for (const char *cp = cmdline; *cp; cp++)
    {
        if (*cp == '"') {
            inquote = !inquote;
        }
        if (!inquote && isspace(*cp))
            break;
    }

        
    int len = cp - cmdline;
    char *rval = (char *)malloc(len+1);

    cp = cmdline;
    char *rp = rval;
    
    while (len--)
        *rp++ = *cp++;

    *rp = 0;
    return rval;
}


/**
 * OK, so it would seem CreateProcess only handles true executables:
 *  .com or .exe files.
 * So test to see whether we're getting a .bat file and if so grab
 * $COMSPEC value and pass batch file to it.
 */
HANDLE
CreateChildProcess(const char *cmdline, HANDLE in, HANDLE out, HANDLE err)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;
    BOOL bFuncRetn = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.

    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFO structure.

    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    // setup new process to use supplied handles for stdin,stdout,stderr
    // if supplied handles are not used the send a copy of our STD_HANDLE
    // as appropriate
    siStartInfo.dwFlags = STARTF_USESTDHANDLES;

    if (in != INVALID_HANDLE_VALUE)
        siStartInfo.hStdInput = in;
    else
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    if (out != INVALID_HANDLE_VALUE)
        siStartInfo.hStdOutput = out;
    else
        siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (err != INVALID_HANDLE_VALUE)
        siStartInfo.hStdError = err;
    else
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    // Create the child process.

    char exeFile[256];

    const char *comspec = getenv("COMSPEC");
    
    if (comspec == NULL) // should never happen
        return INVALID_HANDLE_VALUE;

    char *cmdLine = (char *)alloca(strlen(cmdline) + strlen(comspec) + 16);
    
    strcpy(exeFile, comspec);
    strcpy(cmdLine, comspec);
    strcat(cmdLine, " /c ");
    strcat(cmdLine, cmdline);

    // try to execute program
    bFuncRetn = CreateProcessA(exeFile,
                              cmdLine, // command line
                              NULL, // process security attributes
                              NULL, // primary thread security attributes
                              TRUE, // handles are inherited
                              0, // creation flags
                              NULL, // use parent's environment
                              NULL, // use parent's current directory
                              &siStartInfo, // STARTUPINFO pointer
                              &piProcInfo); // receives PROCESS_INFORMATION

    if (bFuncRetn == 0) {
        ErrorExit("CreateProcess failed\n");
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "CreateProcess(%s, %s, ...)=%s\n", exeFile, cmdLine, err);
        LocalFree((void *)err);
        return INVALID_HANDLE_VALUE;
    }
    // we don't need a handle on the process primary thread so we close
    // this now.
    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
}


void
ErrorExit (LPCSTR lpszMessage)
{
    d_msg(__FILE__, __LINE__, 0, "%s", lpszMessage);
}


/*
typedef struct s_bpipe {
   pid_t worker_pid;
   time_t worker_stime;
   int wait;
   btimer_t *timer_id;
   FILE *rfd;
   FILE *wfd;
} BPIPE;
*/

static void
CloseIfValid(HANDLE handle)
{
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
}

#ifndef HAVE_MINGW
BPIPE *
open_bpipe(char *prog, int wait, const char *mode)
{
    HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup,
        hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup,
        hInputFile;

    SECURITY_ATTRIBUTES saAttr;

    BOOL fSuccess;

    hChildStdinRd = hChildStdinWr = hChildStdinWrDup =
        hChildStdoutRd = hChildStdoutWr = hChildStdoutRdDup =
        hInputFile = INVALID_HANDLE_VALUE;

    BPIPE *bpipe = (BPIPE *)malloc(sizeof(BPIPE));
    memset((void *)bpipe, 0, sizeof(BPIPE));

    int mode_read = (mode[0] == 'r');
    int mode_write = (mode[0] == 'w' || mode[1] == 'w');


    // Set the bInheritHandle flag so pipe handles are inherited.

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (mode_read) {

        // Create a pipe for the child process's STDOUT.
        if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
            ErrorExit("Stdout pipe creation failed\n");
            goto cleanup;
        }
        // Create noninheritable read handle and close the inheritable read
        // handle.

        fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
                                   GetCurrentProcess(), &hChildStdoutRdDup , 0,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS);
        if ( !fSuccess ) {
            ErrorExit("DuplicateHandle failed");
            goto cleanup;
        }

        CloseHandle(hChildStdoutRd);
        hChildStdoutRd = INVALID_HANDLE_VALUE;
    }

    if (mode_write) {

        // Create a pipe for the child process's STDIN.

        if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
            ErrorExit("Stdin pipe creation failed\n");
            goto cleanup;
        }

        // Duplicate the write handle to the pipe so it is not inherited.
        fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdinWr,
                                   GetCurrentProcess(), &hChildStdinWrDup,
                                   0,
                                   FALSE,                  // not inherited
                                   DUPLICATE_SAME_ACCESS);
        if (!fSuccess) {
            ErrorExit("DuplicateHandle failed");
            goto cleanup;
        }

        CloseHandle(hChildStdinWr);
        hChildStdinWr = INVALID_HANDLE_VALUE;
    }
    // spawn program with redirected handles as appropriate
    bpipe->worker_pid = (pid_t)
        CreateChildProcess(prog,             // commandline
                           hChildStdinRd,    // stdin HANDLE
                           hChildStdoutWr,   // stdout HANDLE
                           hChildStdoutWr);  // stderr HANDLE

    if ((HANDLE) bpipe->worker_pid == INVALID_HANDLE_VALUE)
        goto cleanup;

    bpipe->wait = wait;
    bpipe->worker_stime = time(NULL);

    if (mode_read) {
        CloseHandle(hChildStdoutWr); // close our write side so when
                                     // process terminates we can
                                     // detect eof.
        // ugly but convert WIN32 HANDLE to FILE*
        int rfd = _open_osfhandle((long)hChildStdoutRdDup, O_RDONLY);
        if (rfd >= 0) {
           bpipe->rfd = _fdopen(rfd, "r");
        }
    }
    if (mode_write) {
        CloseHandle(hChildStdinRd); // close our read side so as not
                                    // to interfre with child's copy
        // ugly but convert WIN32 HANDLE to FILE*
        int wfd = _open_osfhandle((long)hChildStdinWrDup, O_WRONLY);
        if (wfd >= 0) {
           bpipe->wfd = _fdopen(wfd, "w");
        }
    }

    if (wait > 0) {
        bpipe->timer_id = start_child_timer(bpipe->worker_pid, wait);
    }

    return bpipe;

cleanup:

    CloseIfValid(hChildStdoutRd);
    CloseIfValid(hChildStdoutRdDup);
    CloseIfValid(hChildStdinWr);
    CloseIfValid(hChildStdinWrDup);

    free((void *) bpipe);
    errno = b_errno_win32;            /* do GetLastError() for error code */
    return NULL;
}

#endif //HAVE_MINGW

int
kill(int pid, int signal)
{
   int rval = 0;
   if (!TerminateProcess((HANDLE)pid, (UINT) signal)) {
      rval = -1;
      errno = b_errno_win32;
   }
   CloseHandle((HANDLE)pid);
   return rval;
}

#ifndef HAVE_MINGW

int
close_bpipe(BPIPE *bpipe)
{
   int rval = 0;
   int32_t remaining_wait = bpipe->wait;

   if (remaining_wait == 0) {         /* wait indefinitely */
      remaining_wait = INT32_MAX;
   }
   for ( ;; ) {
      DWORD exitCode;
      if (!GetExitCodeProcess((HANDLE)bpipe->worker_pid, &exitCode)) {
         const char *err = errorString();
         rval = b_errno_win32;
         d_msg(__FILE__, __LINE__, 0,
               "GetExitCode error %s\n", err);
         LocalFree((void *)err);
         break;
      }
      if (exitCode == STILL_ACTIVE) {
         if (remaining_wait <= 0) {
            rval = ETIME;             /* timed out */
            break;
         }
         bmicrosleep(1, 0);           /* wait one second */
         remaining_wait--;
      } else if (exitCode != 0) {
         /* Truncate exit code as it doesn't seem to be correct */
         rval = (exitCode & 0xFF) | b_errno_exit;
         break;
      } else {
         break;                       /* Shouldn't get here */
      }
   }

   if (bpipe->timer_id) {
       stop_child_timer(bpipe->timer_id);
   }
   if (bpipe->rfd) fclose(bpipe->rfd);
   if (bpipe->wfd) fclose(bpipe->wfd);
   free((void *)bpipe);
   return rval;
}

int
close_wpipe(BPIPE *bpipe)
{
    int stat = 1;

    if (bpipe->wfd) {
        fflush(bpipe->wfd);
        if (fclose(bpipe->wfd) != 0) {
            stat = 0;
        }
        bpipe->wfd = NULL;
    }
    return stat;
}

#include "findlib/find.h"

int
utime(const char *fname, struct utimbuf *times)
{
    FILETIME acc, mod;
    char tmpbuf[1024];

    cygwin_conv_to_win32_path(fname, tmpbuf, 1024);

    cvt_utime_to_ftime(times->actime, acc);
    cvt_utime_to_ftime(times->modtime, mod);

    HANDLE h = INVALID_HANDLE_VALUE;

    if (p_CreateFileW) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);
      UTF8_2_wchar(&pwszBuf, tmpbuf);

      h = p_CreateFileW((LPCWSTR) pwszBuf,
                        FILE_WRITE_ATTRIBUTES,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
     
      free_pool_memory(pwszBuf);
    } else if (p_CreateFileA) {
      h = p_CreateFileA(tmpbuf,
                        FILE_WRITE_ATTRIBUTES,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    }

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "Cannot open file \"%s\" for utime(): ERR=%s", tmpbuf, err);
        LocalFree((void *)err);
        errno = b_errno_win32;
        return -1;
    }

    int rval = SetFileTime(h, NULL, &acc, &mod) ? 0 : -1;
    CloseHandle(h);
    if (rval == -1) {
       errno = b_errno_win32;
    }
    return rval;
}

#if USE_WIN32_COMPAT_IO

int
open(const char *file, int flags, int mode)
{
   if (p_wopen) {
      POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);      
      UTF8_2_wchar(&pwszBuf, file);

      int nRet = p_wopen((LPCWSTR) pwszBuf, flags|_O_BINARY, mode);
      free_pool_memory(pwszBuf);

      return nRet;
   }

   return _open(file, flags|_O_BINARY, mode);
}

/*
 * Note, this works only for a file. If you want
 *   to close a socket, use closesocket(). 
 *   Bacula has been modified in src/lib/bnet.c
 *   to use closesocket().
 */
int
close(int fd)
{
    return _close(fd);
}

#ifndef HAVE_WXCONSOLE
ssize_t
read(int fd, void *buf, ssize_t len)
{
    return _read(fd, buf, (size_t)len);
}

ssize_t
write(int fd, const void *buf, ssize_t len)
{
    return _write(fd, buf, (size_t)len);
}
#endif


off_t
lseek(int fd, off_t offset, int whence)
{
    return _lseeki64(fd, offset, whence);
}

int
dup2(int fd1, int fd2)
{
    return _dup2(fd1, fd2);
}
#else
int
open(const char *file, int flags, int mode)
{
    DWORD access = 0;
    DWORD shareMode = 0;
    DWORD create = 0;
    DWORD msflags = 0;
    HANDLE foo = INVALID_HANDLE_VALUE;
    const char *remap = file;

    if (flags & O_WRONLY) access = GENERIC_WRITE;
    else if (flags & O_RDWR) access = GENERIC_READ|GENERIC_WRITE;
    else access = GENERIC_READ;

    if (flags & O_CREAT) create = CREATE_NEW;
    else create = OPEN_EXISTING;

    if (flags & O_TRUNC) create = TRUNCATE_EXISTING;

    if (!(flags & O_EXCL))
        shareMode = FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE;

    if (flags & O_APPEND) {
        printf("open...APPEND not implemented yet.");
        exit(-1);
    }

    if (p_CreateFileW) {
       POOLMEM* pwszBuf = get_pool_memory(PM_FNAME);       
       UTF8_2_wchar(pwszBuf, file);

       foo = p_CreateFileW((LPCWSTR) pwszBuf, access, shareMode, NULL, create, msflags, NULL);
       free_pool_memory(pwszBuf);
    }
    else if (p_CreateFileA)
       foo = CreateFile(file, access, shareMode, NULL, create, msflags, NULL);

    if (INVALID_HANDLE_VALUE == foo) {
        errno = b_errno_win32;
        return(int) -1;
    }
    return (int)foo;

}


int
close(int fd)
{
    if (!CloseHandle((HANDLE)fd)) {
        errno = b_errno_win32;
        return -1;
    }

    return 0;
}

ssize_t
write(int fd, const void *data, ssize_t len)
{
    BOOL status;
    DWORD bwrite;
    status = WriteFile((HANDLE)fd, data, len, &bwrite, NULL);
    if (status) return bwrite;
    errno = b_errno_win32;
    return -1;
}


ssize_t
read(int fd, void *data, ssize_t len)
{
    BOOL status;
    DWORD bread;

    status = ReadFile((HANDLE)fd, data, len, &bread, NULL);
    if (status) return bread;
    errno = b_errno_win32;
    return -1;
}

off_t
lseek(int fd, off_t offset, int whence)
{
    DWORD method = 0;
    DWORD val;
    switch (whence) {
    case SEEK_SET :
        method = FILE_BEGIN;
        break;
    case SEEK_CUR:
        method = FILE_CURRENT;
        break;
    case SEEK_END:
        method = FILE_END;
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    if ((val=SetFilePointer((HANDLE)fd, (DWORD)offset, NULL, method)) == INVALID_SET_FILE_POINTER) {
       errno = b_errno_win32;
       return -1;
    }
    /* ***FIXME*** I doubt this works right */
    return val;
}

int
dup2(int, int)
{
    errno = ENOSYS;
    return -1;
}


#endif

#endif //HAVE_MINGW

#ifdef HAVE_MINGW
/* syslog function, added by Nicolas Boichat */
void closelog() {}
#endif //HAVE_MINGW
