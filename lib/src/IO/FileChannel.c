#include <__oo2c.h>
#include <__config.h>
#include <IO/FileChannel.d>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/* if the system imposes no limit on the length of a file name, choose an 
   arbitrary large value instead */
#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#define DEFAULT_MASK 0666

static const OOC_UINT32 rd_mask = (1U<<IO_FileChannel__read);
static const OOC_UINT32 wr_mask = (1U<<IO_FileChannel__write);
static const OOC_UINT32 rdwr_mask = ((1U<<IO_FileChannel__read)|
				     (1U<<IO_FileChannel__write));
static const OOC_UINT32 creat_mask = (1U<<IO_FileChannel__create);
static const OOC_UINT32 excl_mask = (1U<<IO_FileChannel__exclusive);
static const OOC_UINT32 trunc_mask = (1U<<IO_FileChannel__truncate);
static const OOC_UINT32 append_mask = (1U<<IO_FileChannel__append);
static const OOC_UINT32 tmp_mask = (1U<<IO_FileChannel__tmpFile);

static IO_FileChannel__Channel *tmp_files = NULL;
static int tmp_files_size = -1;
static int tmp_files_used = 0;

static void add_tmp_file(IO_FileChannel__Channel ch) {
  if (tmp_files) {
    if (tmp_files_used == tmp_files_size) {
      IO_FileChannel__Channel *n;
      n = RT0__NewObject(OOC_TYPE_DESCR(IO_FileChannel,ChannelList),
			 tmp_files_size);
      memcpy(n, tmp_files, tmp_files_size*sizeof(void*));
      tmp_files_size *= 2;
    }
  } else {
    tmp_files_size = 8;
    tmp_files = RT0__NewObject(OOC_TYPE_DESCR(IO_FileChannel,ChannelList),
			       tmp_files_size);
  }
  ch->tmpIndex = tmp_files_used;
  tmp_files[tmp_files_used] = ch;
  tmp_files_used++;
}

static void remove_tmp_file(IO_FileChannel__Channel ch) {
  int i = ch->tmpIndex;
  if (i != tmp_files_used-1) {
    tmp_files[i] = tmp_files[tmp_files_used-1];
    tmp_files[i]->tmpIndex = i;
  }
  ch->tmpIndex = -1;
  tmp_files_used--;
}

static void cleanup_tmp_files() {
  char* tname;
  IO_FileChannel__Channel ch;
  
  while (tmp_files_used != 0) {
    tmp_files_used--;
    
    /* close and unlink all temporary files, ignoring any errors */
    ch = tmp_files[tmp_files_used];
    close(ch->fd);
    tname = (char*)OOC_METHOD(ch->tmpName,
			      Object__String8Desc_CharsLatin1)(ch->tmpName);
    unlink(tname);
    remove_tmp_file(ch);
  }
}


IO_FileChannel__Channel IO_FileChannel__Open(Object__String8 file,
					       OOC_UINT32 mode) {
  int flags = 0;
  char* fname = (char*)OOC_METHOD(file,Object__String8Desc_CharsLatin1)(file);
  int fd;
  Object__String8 tmpName = NULL;
  
  if ((mode & rdwr_mask) == rdwr_mask) {
    flags |= O_RDWR;
  } else if (mode & rd_mask) {
    flags |= O_RDONLY;
  } else if (mode & wr_mask) {
    flags |= O_WRONLY;
  }
  if (mode & creat_mask) {
    flags |= O_CREAT;
  }
  if (mode & excl_mask) {
    flags |= O_EXCL;
  }
  if (mode & trunc_mask) {
    flags |= O_TRUNC;
  }
  if (mode & append_mask) {
    flags |= O_APPEND;
  }

  if (mode & tmp_mask) {
    char tname[PATH_MAX+16];
    int count = 0;
    
    flags |= (O_CREAT|O_EXCL);
    do {
      /* repeat trying to open the file until an unused file is found, or
	 an error occured */
      if (count) {
	(void)sprintf(tname, "%s^%d", fname, count);
      } else {
	(void)sprintf(tname, "%s^", fname);
      }
      fd = open(tname, flags, DEFAULT_MASK);
      count++;
    } while ((fd < 0) && (errno == EEXIST));
    tmpName = Object__NewLatin1((OOC_CHAR8*)tname, PATH_MAX+16);
  } else {
    fd = open(fname, flags, DEFAULT_MASK);
  }
  
  if (fd < 0) {
    IO_PFD__IOError((Object__String)file);
  } else {
    IO_FileChannel__Channel ch =
      RT0__NewObject(OOC_TYPE_DESCR(IO_FileChannel,ChannelDesc));
    IO__InitByteChannel((IO__ByteChannel)ch);
    ch->fd = fd;
    ch->origName = file;
    ch->tmpName = tmpName;
    if (tmpName) {
      add_tmp_file(ch);
    } else {
      ch->tmpIndex = -1;
    }
    return ch;
  }
}

void IO_FileChannel__ChannelDesc_SetBlocking(IO_FileChannel__Channel ch,
					      OOC_CHAR8 block) {
  /* not supported */
}

OOC_INT32 IO_FileChannel__ChannelDesc_Read(IO_FileChannel__Channel ch,
				    OOC_CHAR8 buffer[], OOC_LEN x_0d,
				    OOC_INT32 start, OOC_INT32 length) {
  ssize_t res;
  
  do {
    res = read(ch->fd, buffer+start, (size_t)length);
  } while ((res < 0) && (errno == EINTR));
  
  if (res < 0) {		/* check error condition */
    IO_PFD__IOError(NULL);
  } else if ((res == 0) && (length != 0)) {
    return -1;			/* end of file */
  }
  return res;			/* can be zero if size is zero */
}

OOC_INT32 IO_FileChannel__ChannelDesc_Write(IO_FileChannel__Channel ch,
				     const OOC_BYTE buffer[], OOC_LEN x_0d,
				     OOC_INT32 start, OOC_INT32 length) {
  ssize_t res;
  
  do {
    res = write(ch->fd, buffer+start, (size_t)length);
  } while ((res < 0) && (errno == EINTR));
  
  if (res < 0) {
    IO_PFD__IOError(NULL);
  }
  return res;
}

void IO_FileChannel__ChannelDesc_Close(IO_FileChannel__Channel ch) {
  int res = close(ch->fd);
  
  if (res >= 0) {
    ch->fd = -1;
    IO__ChannelDesc_Close((IO__Channel)ch);
    
    if (ch->tmpIndex >= 0) {
      char* tname = (char*)OOC_METHOD(ch->tmpName,Object__String8Desc_CharsLatin1)(ch->tmpName);
      res = unlink(tname);
      remove_tmp_file(ch);
    }
  }
    
  if (res < 0) {
    IO_PFD__IOError((Object__String)(ch->tmpIndex<0?ch->tmpName:ch->origName));
  }
}

void IO_FileChannel__ChannelDesc_CloseAndRegister(IO_FileChannel__Channel ch) {
  int res = close(ch->fd);
  
  if (res >= 0) {
    ch->fd = -1;
    IO__ChannelDesc_Close((IO__Channel)ch);
    
    if (ch->tmpIndex >= 0) {
      char* fname = (char*)OOC_METHOD(ch->origName,Object__String8Desc_CharsLatin1)(ch->origName);
      char* tname = (char*)OOC_METHOD(ch->tmpName,Object__String8Desc_CharsLatin1)(ch->tmpName);
      res = rename(tname, fname);
      remove_tmp_file(ch);
    }
  }
  
  if (res < 0) {
    IO_PFD__IOError((Object__String)(ch->tmpIndex<0?ch->tmpName:ch->origName));
  }
}

void IO_FileChannel__ChannelDesc_SetPosition(IO_FileChannel__Channel ch,
					     OOC_INT32 pos) {
  if (lseek(ch->fd, (off_t)pos, SEEK_SET) < 0) {
    IO_PFD__IOError((Object__String)(ch->tmpIndex<0?ch->tmpName:ch->origName));
  }
}

OOC_INT32 IO_FileChannel__ChannelDesc_FileDescriptor(IO_FileChannel__Channel ch) {
  return ch->fd;
}

void OOC_IO_FileChannel_init(void) {
  Termination__RegisterProc(cleanup_tmp_files);
}