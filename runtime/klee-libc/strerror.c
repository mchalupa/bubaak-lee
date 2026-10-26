/*===-- strerror.c --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "nondet.h"

#define NMESSAGES 134

/* The messages of glibc (Linux, x86_64) for errnum 0..133, as one blob of
 * NMESSAGES strings, so that a symbolic errnum selects a message by an offset
 * (a read of a constant array, no fork); the state forks only when the
 * returned string is used. */
static const char messages[] =
  "Success\0"
  "Operation not permitted\0"
  "No such file or directory\0"
  "No such process\0"
  "Interrupted system call\0"
  "Input/output error\0"
  "No such device or address\0"
  "Argument list too long\0"
  "Exec format error\0"
  "Bad file descriptor\0"
  "No child processes\0"
  "Resource temporarily unavailable\0"
  "Cannot allocate memory\0"
  "Permission denied\0"
  "Bad address\0"
  "Block device required\0"
  "Device or resource busy\0"
  "File exists\0"
  "Invalid cross-device link\0"
  "No such device\0"
  "Not a directory\0"
  "Is a directory\0"
  "Invalid argument\0"
  "Too many open files in system\0"
  "Too many open files\0"
  "Inappropriate ioctl for device\0"
  "Text file busy\0"
  "File too large\0"
  "No space left on device\0"
  "Illegal seek\0"
  "Read-only file system\0"
  "Too many links\0"
  "Broken pipe\0"
  "Numerical argument out of domain\0"
  "Numerical result out of range\0"
  "Resource deadlock avoided\0"
  "File name too long\0"
  "No locks available\0"
  "Function not implemented\0"
  "Directory not empty\0"
  "Too many levels of symbolic links\0"
  "Unknown error 41\0"
  "No message of desired type\0"
  "Identifier removed\0"
  "Channel number out of range\0"
  "Level 2 not synchronized\0"
  "Level 3 halted\0"
  "Level 3 reset\0"
  "Link number out of range\0"
  "Protocol driver not attached\0"
  "No CSI structure available\0"
  "Level 2 halted\0"
  "Invalid exchange\0"
  "Invalid request descriptor\0"
  "Exchange full\0"
  "No anode\0"
  "Invalid request code\0"
  "Invalid slot\0"
  "Unknown error 58\0"
  "Bad font file format\0"
  "Device not a stream\0"
  "No data available\0"
  "Timer expired\0"
  "Out of streams resources\0"
  "Machine is not on the network\0"
  "Package not installed\0"
  "Object is remote\0"
  "Link has been severed\0"
  "Advertise error\0"
  "Srmount error\0"
  "Communication error on send\0"
  "Protocol error\0"
  "Multihop attempted\0"
  "RFS specific error\0"
  "Bad message\0"
  "Value too large for defined data type\0"
  "Name not unique on network\0"
  "File descriptor in bad state\0"
  "Remote address changed\0"
  "Can not access a needed shared library\0"
  "Accessing a corrupted shared library\0"
  ".lib section in a.out corrupted\0"
  "Attempting to link in too many shared libraries\0"
  "Cannot exec a shared library directly\0"
  "Invalid or incomplete multibyte or wide character\0"
  "Interrupted system call should be restarted\0"
  "Streams pipe error\0"
  "Too many users\0"
  "Socket operation on non-socket\0"
  "Destination address required\0"
  "Message too long\0"
  "Protocol wrong type for socket\0"
  "Protocol not available\0"
  "Protocol not supported\0"
  "Socket type not supported\0"
  "Operation not supported\0"
  "Protocol family not supported\0"
  "Address family not supported by protocol\0"
  "Address already in use\0"
  "Cannot assign requested address\0"
  "Network is down\0"
  "Network is unreachable\0"
  "Network dropped connection on reset\0"
  "Software caused connection abort\0"
  "Connection reset by peer\0"
  "No buffer space available\0"
  "Transport endpoint is already connected\0"
  "Transport endpoint is not connected\0"
  "Cannot send after transport endpoint shutdown\0"
  "Too many references: cannot splice\0"
  "Connection timed out\0"
  "Connection refused\0"
  "Host is down\0"
  "No route to host\0"
  "Operation already in progress\0"
  "Operation now in progress\0"
  "Stale file handle\0"
  "Structure needs cleaning\0"
  "Not a XENIX named type file\0"
  "No XENIX semaphores available\0"
  "Is a named type file\0"
  "Remote I/O error\0"
  "Disk quota exceeded\0"
  "No medium found\0"
  "Wrong medium type\0"
  "Operation canceled\0"
  "Required key not available\0"
  "Key has expired\0"
  "Key has been revoked\0"
  "Key was rejected by service\0"
  "Owner died\0"
  "State not recoverable\0"
  "Operation not possible due to RF-kill\0"
  "Memory page has hardware error\0"
  ;

/* the offsets of the messages in `messages` */
static const unsigned short offsets[NMESSAGES] = {
  0, 8, 32, 58, 74, 98, 117, 143, 166, 184, 204, 223,
  256, 279, 297, 309, 331, 355, 367, 393, 408, 424, 439, 456,
  486, 506, 537, 552, 567, 591, 604, 626, 641, 653, 686, 716,
  742, 761, 780, 805, 825, 859, 876, 903, 922, 950, 975, 990,
  1004, 1029, 1058, 1085, 1100, 1117, 1144, 1158, 1167, 1188, 1201, 1218,
  1239, 1259, 1277, 1291, 1316, 1346, 1368, 1385, 1407, 1423, 1437, 1465,
  1480, 1499, 1518, 1530, 1568, 1595, 1624, 1647, 1686, 1723, 1755, 1803,
  1841, 1891, 1935, 1954, 1969, 2000, 2029, 2046, 2077, 2100, 2123, 2149,
  2173, 2203, 2244, 2267, 2299, 2315, 2338, 2374, 2407, 2432, 2458, 2498,
  2534, 2580, 2615, 2636, 2655, 2668, 2685, 2715, 2741, 2759, 2784, 2812,
  2842, 2863, 2880, 2900, 2916, 2934, 2953, 2980, 2996, 3017, 3045, 3056,
  3078, 3116,
};

/* glibc formats "Unknown error N" into a static buffer */
static char unknown[sizeof("Unknown error -2147483648")];

/* The glibc function (the result is a message of the table for the errnum
 * 0..133 and the errnum can be symbolic). For the other values, the number is
 * formatted into a static buffer, which needs a concrete errnum (the value is
 * chosen, so the model under-approximates). */
char *strerror(int errnum) {
  if (errnum >= 0 && errnum < NMESSAGES)
    return (char *)messages + offsets[errnum];

  klee_report_under_approximation("strerror");
  long n = klee_get_value_i32(errnum);
  static const char prefix[] = "Unknown error ";
  char *p = unknown;
  char digits[11];
  int nd = 0;
  unsigned long u = n < 0 ? -n : n;
  for (unsigned i = 0; i + 1 < sizeof(prefix); ++i)
    *p++ = prefix[i];
  if (n < 0)
    *p++ = '-';
  do {
    digits[nd++] = '0' + u % 10;
    u /= 10;
  } while (u);
  while (nd)
    *p++ = digits[--nd];
  *p = 0;
  return unknown;
}
