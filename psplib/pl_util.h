/** PSP helper library ***************************************/
/**                                                         **/
/**                      pl_util.h                          **/
/**                                                         **/
/** This file contains declarations for various utility     **/
/** routines                                                **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PL_UTIL_H
#define _PL_UTIL_H

#include <psptypes.h>
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sequential image saves */
int pl_util_save_image_seq(const char *path,
                           const char *filename,
                           const PspImage *image);
int pl_util_save_vram_seq(const char *path,
                          const char *prefix);

/* Date compare */
int pl_util_date_compare(const ScePspDateTime *date1,
                         const ScePspDateTime *date2);

/* CRC32 */
int pl_util_compute_crc32_buffer(const void *buf,
                                 size_t buf_len,
                                 uint32_t *crc_32);
int pl_util_compute_crc32_fd(FILE *file,
                             uint32_t *outCrc32);
int pl_util_compute_crc32_file(const char *path,
                               uint32_t *outCrc32);

#ifdef __cplusplus
}
#endif

#endif // _PL_UTIL_H
