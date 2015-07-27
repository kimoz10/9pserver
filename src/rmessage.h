/*
 * rmessage.h
 *
 *  Created on: Jul 17, 2015
 *      Author: kelghamrawy
 */

#ifndef SRC_RMESSAGE_H_
#define SRC_RMESSAGE_H_

#include "rfunctions.h"
#include "9p.h"
#include "fid.h"

void prepare_reply(p9_obj_t *T_p9_obj, p9_obj_t *R_p9_obj, fid_list **fid_table);

#endif /* SRC_RMESSAGE_H_ */
