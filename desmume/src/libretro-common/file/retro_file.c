/* Copyright  (C) 2010-2015 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (retro_file.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
#  include <compat/posix_string.h>
#  ifdef _MSC_VER
#    define setmode _setmode
#  endif
#  ifdef _XBOX
#    include <xtl.h>
#    define INVALID_FILE_ATTRIBUTES -1
#  else
#    include <io.h>
#    include <fcntl.h>
#    include <direct.h>
#    include <windows.h>
#  endif
#elif defined(VITA)
#  include <psp2/io/fcntl.h>
#  include <psp2/io/dirent.h>

#define PSP_O_RDONLY PSP2_O_RDONLY
#define PSP_O_RDWR   PSP2_O_RDWR
#define PSP_O_CREAT  PSP2_O_CREAT
#define PSP_O_WRONLY PSP2_O_WRONLY
#define PSP_O_TRUNC  PSP2_O_TRUNC
#else
#  if defined(PSP)
#    include <pspiofilemgr.h>
#  endif
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <dirent.h>
#  include <unistd.h>
#endif

#ifdef __CELLOS_LV2__
#include <cell/cell_fs.h>
#else
#include <fcntl.h>
#endif

#ifdef RARCH_INTERNAL
#include <retro_log.h>
#endif

#include <retro_file.h>

#if 1
#define HAVE_BUFFERED_IO 1
#endif

struct RFILE
{
#if defined(PSP) || defined(VITA)
   SceUID fd;
#elif defined(__CELLOS_LV2__)
   int fd;
#elif defined(HAVE_BUFFERED_IO)
   FILE *fd;
#else
   int fd;
#endif
};

int retro_get_fd(RFILE *stream)
{
   if (!stream)
      return -1;
#if defined(VITA) || defined(PSP) || defined(__CELLOS_LV2__)
   return stream->fd;
#elif defined(HAVE_BUFFERED_IO)
   return fileno(stream->fd);
#else
   return stream->fd;
#endif
}

RFILE *retro_fopen(const char *path, unsigned mode, ssize_t len)
{
   int            flags = 0;
   int         mode_int = 0;
   const char *mode_str = NULL;
   RFILE        *stream = (RFILE*)calloc(1, sizeof(*stream));

   if (!stream)
      return NULL;

   (void)mode_str;
   (void)mode_int;
   (void)flags;

   switch (mode)
   {
      case RFILE_MODE_READ:
#if defined(VITA) || defined(PSP)
         mode_int = 0777;
         flags    = PSP_O_RDONLY;
#elif defined(__CELLOS_LV2__)
         mode_int = 0777;
         flags    = CELL_FS_O_RDONLY;
#elif defined(HAVE_BUFFERED_IO)
         mode_str = "rb";
#else
         flags    = O_RDONLY;
#endif
         break;
      case RFILE_MODE_WRITE:
#if defined(VITA) || defined(PSP)
         mode_int = 0777;
         flags    = PSP_O_CREAT | PSP_O_WRONLY | PSP_O_TRUNC;
#elif defined(__CELLOS_LV2__)
         mode_int = 0777;
         flags    = CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC;
#elif defined(HAVE_BUFFERED_IO)
         mode_str = "wb";
#else
         flags    = O_WRONLY | O_CREAT | O_TRUNC | S_IRUSR | S_IWUSR;
#endif
         break;
      case RFILE_MODE_READ_WRITE:
#if defined(VITA) || defined(PSP)
         mode_int = 0777;
         flags    = PSP_O_RDWR;
#elif defined(__CELLOS_LV2__)
         mode_int = 0777;
         flags    = CELL_FS_O_RDWR;
#elif defined(HAVE_BUFFERED_IO)
         mode_str = "w+";
#else
         flags    = O_RDWR;
#ifdef _WIN32
         flags   |= O_BINARY;
#endif
#endif
         break;
   }

#if defined(VITA) || defined(PSP)
   stream->fd = sceIoOpen(path, flags, mode_int);
#elif defined(__CELLOS_LV2__)
   cellFsOpen(path, flags, &stream->fd, NULL, 0);
#elif defined(HAVE_BUFFERED_IO)
   stream->fd = fopen(path, mode_str);
#else
   stream->fd = open(path, flags);
#endif

#if defined(HAVE_BUFFERED_IO)
   if (!stream->fd)
      goto error;
#else
   if (stream->fd == -1)
      goto error;
#endif

   return stream;

error:
   retro_fclose(stream);
   return NULL;
}

ssize_t retro_fseek(RFILE *stream, ssize_t offset, int whence)
{
   int ret = 0;
   if (!stream)
      return -1;

   (void)ret;

#if defined(VITA) || defined(PSP)
   ret = sceIoLseek(stream->fd, (SceOff)offset, whence);
   if (ret == -1)
      return -1;
   return 0;
#elif defined(__CELLOS_LV2__)
   uint64_t pos = 0;
   if (cellFsLseek(stream->fd, offset, whence, &pos) != CELL_FS_SUCCEEDED)
      return -1;
   return 0;
#elif defined(HAVE_BUFFERED_IO)
   return fseek(stream->fd, (long)offset, whence);
#else
   ret = lseek(stream->fd, offset, whence);
   if (ret == -1)
      return -1;
   return 0;
#endif
}

ssize_t retro_ftell(RFILE *stream)
{
   if (!stream)
      return -1;
#if defined(VITA) || defined(PSP)
   return sceIoLseek(stream->fd, 0, SEEK_CUR);
#elif defined(__CELLOS_LV2__)
   uint64_t pos = 0;
   if (cellFsLseek(stream->fd, 0, CELL_FS_SEEK_CUR, &pos) != CELL_FS_SUCCEEDED)
      return -1;
   return 0;
#elif defined(HAVE_BUFFERED_IO)
   return ftell(stream->fd);
#else 
   return lseek(stream->fd, 0, SEEK_CUR);
#endif
}

void retro_frewind(RFILE *stream)
{
   retro_fseek(stream, 0L, SEEK_SET);
}

ssize_t retro_fread(RFILE *stream, void *s, size_t len)
{
   if (!stream)
      return -1;
#if defined(VITA) || defined(PSP)
   return sceIoRead(stream->fd, s, len);
#elif defined(__CELLOS_LV2__)
   uint64_t bytes_written;
   if (cellFsRead(stream->fd, s, len, &bytes_written) != CELL_FS_SUCCEEDED)
      return -1;
   return bytes_written;
#elif defined(HAVE_BUFFERED_IO)
   return fread(s, 1, len, stream->fd);
#else
   return read(stream->fd, s, len);
#endif
}

ssize_t retro_fwrite(RFILE *stream, const void *s, size_t len)
{
   if (!stream)
      return -1;
#if defined(VITA) || defined(PSP)
   return sceIoWrite(stream->fd, s, len);
#elif defined(__CELLOS_LV2__)
   uint64_t bytes_written;
   if (cellFsWrite(stream->fd, s, len, &bytes_written) != CELL_FS_SUCCEEDED)
      return -1;
   return bytes_written;
#elif defined(HAVE_BUFFERED_IO)
   return fwrite(s, 1, len, stream->fd);
#else
   return write(stream->fd, s, len);
#endif
}

int retro_fclose(RFILE *stream)
{
   if (!stream)
      return -1;

#if defined(VITA) || defined(PSP)
   if (stream->fd > 0)
      sceIoClose(stream->fd);
#elif defined(__CELLOS_LV2__)
   if (stream->fd > 0)
      cellFsClose(stream->fd);
#elif defined(HAVE_BUFFERED_IO)
   if (stream->fd)
      fclose(stream->fd);
#else
   if (stream->fd > 0)
      close(stream->fd);
#endif
   free(stream);

   return 0;
}

/**
 * retro_read_file:
 * @path             : path to file.
 * @buf              : buffer to allocate and read the contents of the
 *                     file into. Needs to be freed manually.
 *
 * Read the contents of a file into @buf.
 *
 * Returns: number of items read, -1 on error.
 */
int retro_read_file(const char *path, void **buf, ssize_t *len)
{
   ssize_t ret              = 0;
   ssize_t content_buf_size = 0;
   void *content_buf        = NULL;
   RFILE *file              = retro_fopen(path, RFILE_MODE_READ, -1);

   if (!file)
      goto error;

   if (retro_fseek(file, 0, SEEK_END) != 0)
      goto error;

   content_buf_size = retro_ftell(file);
   if (content_buf_size < 0)
      goto error;

   retro_frewind(file);

   content_buf = malloc(content_buf_size + 1);

   if (!content_buf)
      goto error;

   if ((ret = retro_fread(file, content_buf, content_buf_size)) < content_buf_size)
   {
#ifdef RARCH_INTERNAL
      RARCH_WARN("Didn't read whole file: %s.\n", path);
#else
      printf("Didn't read whole file: %s.\n", path);
#endif
   }

   if (!content_buf)
      goto error;

   *buf    = content_buf;

   /* Allow for easy reading of strings to be safe.
    * Will only work with sane character formatting (Unix). */
   ((char*)content_buf)[content_buf_size] = '\0';

   if (retro_fclose(file) != 0)
      printf("Failed to close file stream.\n");

   if (len)
      *len = ret;

   return 1;

error:
   retro_fclose(file);
   if (content_buf)
      free(content_buf);
   if (len)
      *len = -1;
   *buf = NULL;
   return 0;
}

/**
 * retro_write_file:
 * @path             : path to file.
 * @data             : contents to write to the file.
 * @size             : size of the contents.
 *
 * Writes data to a file.
 *
 * Returns: true (1) on success, false (0) otherwise.
 */
bool retro_write_file(const char *path, const void *data, ssize_t size)
{
   ssize_t ret   = 0;
   RFILE *file   = retro_fopen(path, RFILE_MODE_WRITE, -1);
   if (!file)
      return false;

   ret = retro_fwrite(file, data, size);
   retro_fclose(file);

   return (ret == size);
}
