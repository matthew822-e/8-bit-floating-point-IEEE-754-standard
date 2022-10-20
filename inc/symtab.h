/* DO NOT EDIT THIS FILE
 * - symtab.h (Zuon Calculator)
 * - Copyright: Prof. Kevin Andrea, George Mason University.  All Rights Reserved
 * - Date: Aug 2022
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include "kifp.h"

void initialize_symtab();
int sym_exists(const char *name);
kifp_t get_value(const char *name);
void insert_symbol(const char *name, kifp_t value);
void teardown_symtab();

#endif
