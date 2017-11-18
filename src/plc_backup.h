/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _DBG_BACKUP_H_
#define _DBG_BACKUP_H_

/*YAPLC related functions*/
void plc_backup_init(void);
void plc_backup_reset(void);

/*Bereiz related functions: these functions are used by Beremiz generated plc_debug.c file*/
void plc_backup_invalidate(void);
void plc_backup_validate(void);
int plc_backup_check(void);
void plc_backup_remind(unsigned int offset, unsigned int count, void *p);
void plc_backup_retain(unsigned int offset, unsigned int count, void *p);

#endif /* _DBG_BACKUP_H_ */
