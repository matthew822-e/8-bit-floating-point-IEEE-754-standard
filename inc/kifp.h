/* Do Not Edit this File
 * - Restrictions on Student Code: Do not post your code on any public site (eg. Github).
 * -- Feel free to post your code (kifp.h and kifp.c) on a PRIVATE Github and give interviewers access to it.
 * - Date: Aug 2022
 */

#ifndef KIFP_H
#define KIFP_H

#include "common_structs.h"

/* Public Types for Use */
typedef int kifp_t;

/* Re-defines all float or double keywoards to kifp_t as they are not legal */
#define float int
#define double long

/* Public KIFP Library Functions */
kifp_t toKifp(Number_t *num);
int toNumber(Number_t *num, kifp_t value);
kifp_t negateKifp(kifp_t value);
kifp_t mulKifp(kifp_t val1, kifp_t val2);
kifp_t addKifp(kifp_t val1, kifp_t val2);
kifp_t subKifp(kifp_t val1, kifp_t val2);

// Floating Point Precision
#define KIFP_SPECIAL      1
#define KIFP_SIGNED       1
#define KIFP_DENORMALIZED 1
#define KIFP_ROUND_EVEN   0
#define KIFP_SIGN_BITS    1
#define KIFP_EXP_BITS     3
#define KIFP_FRAC_BITS    5
#define KIFP_TOTAL_BITS   KIFP_SIGN_BITS + KIFP_EXP_BITS + KIFP_FRAC_BITS

// Error Codes
#define KIFP_NUM_ERROR_CODES 3

enum kifp_error_codes {
  Kifp_General_Error = -1,
  Kifp_Insufficient_Memory = -2,
  Kifp_Success = 0,
};

#endif
