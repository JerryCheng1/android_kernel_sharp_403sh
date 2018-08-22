/* drivers/sharp/shtps/sy3000/shtps_filter_diagonal_cling_check.c
 *
 * Copyright (c) 2014, Sharp. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/* -------------------------------------------------------------------------- */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>

#include <sharp/shtps_dev.h>

#include "shtps_rmi.h"
#include "shtps_param_extern.h"
#include "shtps_log.h"

#include "shtps_filter.h"
/* -------------------------------------------------------------------------- */
#if defined(SHTPS_DIAGONAL_CLING_CHECK_ENABLE)
struct shtps_filter_diagonal_cling_reject_info{	
	u8				state;
	unsigned short	finger_x[2];
	unsigned short	finger_y[2];
	unsigned long	tu_time;
	unsigned int	diagonal_cnt[SHTPS_FINGER_MAX];
};


int shtps_filter_diagonal_cling_check_rezero(struct shtps_rmi_spi *ts, struct shtps_touch_info *info)
{
	int fingerMax = shtps_get_fingermax(ts);
	int i = 0;
	u8 is_cling_detect = 0;
	int ret = 0;
	
	for(i = 0; i < fingerMax; i++){
		if(ts->diagonal_cling_reject_p->diagonal_cnt[i] > SHTPS_DIAGONAL_CLING_CHECK_COUNT_THRESH){
			is_cling_detect = 1;
		}
	}

	if(is_cling_detect != 0){
		SHTPS_LOG_DBG_PRINT("rezero execute(diagonal cling)\n");
		shtps_rezero_request(ts, SHTPS_REZERO_REQUEST_REZERO, 0);
		memset(ts->diagonal_cling_reject_p, 0, sizeof(struct shtps_filter_diagonal_cling_reject_info));
		
		shtps_event_force_touchup(ts);
		memset(info, 0, sizeof(struct shtps_touch_info));
		#if defined( SHTPS_PHYSICAL_KEY_ENABLE )
			shtps_key_event_force_touchup(ts);
		#endif /* SHTPS_PHYSICAL_KEY_ENABLE */
		
		ret = 1;
	}
	
	return ret;
}


/* -------------------------------------------------------------------------- */
static int shtps_filter_is_diagonal(int x1, int y1, int x2, int y2, int x3, int y3)
{
	int diagonal_x1 = x1;
	int diagonal_y1 = y2;
	int diagonal_x2 = x2;
	int diagonal_y2 = y1;
	int diff_x, diff_y;

	SHTPS_DIAGONAL_CLING_PRINT(" diagonal point (x1=%d, y1=%d) (x2=%d, y2=%d)\n", diagonal_x1, diagonal_y1, diagonal_x2, diagonal_y2);

	diff_x = diagonal_x1 > x3 ? diagonal_x1 - x3 : x3 - diagonal_x1;
	diff_y = diagonal_y1 > y3 ? diagonal_y1 - y3 : y3 - diagonal_y1;
	if(diff_x < SHTPS_DIAGONAL_CLING_CHECK_AREA &&
		diff_y < SHTPS_DIAGONAL_CLING_CHECK_AREA) {
		return 1;
	}

	diff_x = diagonal_x2 > x3 ? diagonal_x2 - x3 : x3 - diagonal_x2;
	diff_y = diagonal_y2 > y3 ? diagonal_y2 - y3 : y3 - diagonal_y2;
	if(diff_x < SHTPS_DIAGONAL_CLING_CHECK_AREA &&
		diff_y < SHTPS_DIAGONAL_CLING_CHECK_AREA) {
		return 1;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
void shtps_filter_diagonal_cling_check(struct shtps_rmi_spi *ts, struct shtps_touch_info *info)
{
	int i;
	int fingerMax = shtps_get_fingermax(ts);
	int finger_x[2];
	int finger_y[2];
	int cnt;
	int diagonal;
	u8 numOfFingers = 0;

	if(SHTPS_PRM_DIAGONAL_CLING_CHECK_ENABLE == 0){
		return;
	}

	if(ts->state_mgr.state != SHTPS_STATE_ACTIVE){
		SHTPS_DIAGONAL_CLING_PRINT("%s() do not process by drv state[%d]\n", __func__, ts->state_mgr.state);
		return;
	}

	cnt = 0;
	for (i = 0; i < fingerMax; i++) {
		if ((info->fingers[i].state == SHTPS_TOUCH_STATE_FINGER) && (ts->report_info.fingers[i].state == SHTPS_TOUCH_STATE_FINGER)){
			if (cnt < 2) {
				finger_x[cnt] = info->fingers[i].x;
				finger_y[cnt] = info->fingers[i].y;
				cnt++;
			}
		}

		if(info->fingers[i].state == SHTPS_TOUCH_STATE_FINGER){
			numOfFingers++;
		}
	}

	if( (ts->diagonal_cling_reject_p->state == 2) || (ts->diagonal_cling_reject_p->state == 3) ){
		if(numOfFingers > SHTPS_DIAGONAL_CLING_CHECK_MONITORING_FINGER_NUM_MAX){
			ts->diagonal_cling_reject_p->state = 0;
			SHTPS_DIAGONAL_CLING_PRINT(" stop monitoring by numOfFingers[%d]\n", numOfFingers);
		}
	}

	if(cnt != 2) {
		if(cnt < 2) {
			if (ts->diagonal_cling_reject_p->state == 1){
				SHTPS_DIAGONAL_CLING_PRINT(" start monitoring.\n");
				ts->diagonal_cling_reject_p->state   = 2;
				ts->diagonal_cling_reject_p->tu_time = jiffies;
				SHTPS_DIAGONAL_CLING_PRINT(" state:[%d]\n", ts->diagonal_cling_reject_p->state);
			}
		}else{
			ts->diagonal_cling_reject_p->state = 0;
			SHTPS_DIAGONAL_CLING_PRINT(" state:[%d]\n", ts->diagonal_cling_reject_p->state);
		}
	}else{
		if(ts->diagonal_cling_reject_p->state == 0 ){
			ts->diagonal_cling_reject_p->state = 1;
			SHTPS_DIAGONAL_CLING_PRINT(" state:[%d]\n", ts->diagonal_cling_reject_p->state);
		}
	}
	
	if(ts->diagonal_cling_reject_p->state == 2 && 
			time_after(jiffies, ts->diagonal_cling_reject_p->tu_time + msecs_to_jiffies(SHTPS_DIAGONAL_CLING_TIMEOUT)) )
	{
		SHTPS_DIAGONAL_CLING_PRINT(" chek timeout.\n");
		ts->diagonal_cling_reject_p->state = 0;
	}
	
	if(ts->diagonal_cling_reject_p->state != 0) {
		if( ts->diagonal_cling_reject_p->state == 1)
		{
			ts->diagonal_cling_reject_p->finger_x[0] = finger_x[0];
			ts->diagonal_cling_reject_p->finger_x[1] = finger_x[1];
			ts->diagonal_cling_reject_p->finger_y[0] = finger_y[0];
			ts->diagonal_cling_reject_p->finger_y[1] = finger_y[1];
		}
		else if ((ts->diagonal_cling_reject_p->state == 2) ||
			(ts->diagonal_cling_reject_p->state == 3)){
			for (i = 0; i < fingerMax; i++) {
				if (((info->fingers[i].state == SHTPS_TOUCH_STATE_FINGER) && ts->report_info.fingers[i].state == SHTPS_TOUCH_STATE_NO_TOUCH) ||
				    ((info->fingers[i].state == SHTPS_TOUCH_STATE_FINGER) && ts->diagonal_cling_reject_p->diagonal_cnt[i] > 0)
				){
					diagonal = shtps_filter_is_diagonal(
								ts->diagonal_cling_reject_p->finger_x[0],
								ts->diagonal_cling_reject_p->finger_y[0],
								ts->diagonal_cling_reject_p->finger_x[1],
								ts->diagonal_cling_reject_p->finger_y[1],
								info->fingers[i].x, 
								info->fingers[i].y);
					if (diagonal) {
						SHTPS_DIAGONAL_CLING_PRINT(" add count diagonal cling. (id=%d, x=%d, y=%d, z=%d, cnt=%d)\n",
								i,
								info->fingers[i].x,
								info->fingers[i].y,
								info->fingers[i].z,
								ts->diagonal_cling_reject_p->diagonal_cnt[i]);
						ts->diagonal_cling_reject_p->diagonal_cnt[i]++;
					}else{
						SHTPS_DIAGONAL_CLING_PRINT(" NG : P1(%d,%d) P2(%d,%d) finder(%d,%d)\n",
								ts->diagonal_cling_reject_p->finger_x[0],
								ts->diagonal_cling_reject_p->finger_y[0],
								ts->diagonal_cling_reject_p->finger_x[1],
								ts->diagonal_cling_reject_p->finger_y[1],
								info->fingers[i].x, 
								info->fingers[i].y);
						ts->diagonal_cling_reject_p->diagonal_cnt[i] = 0;
					}
				}else{
					ts->diagonal_cling_reject_p->diagonal_cnt[i] = 0;
				}
			}

			cnt = 0;
			for (i = 0; i < fingerMax; i++){
				cnt += ts->diagonal_cling_reject_p->diagonal_cnt[i];
			}
			if(ts->diagonal_cling_reject_p->state == 2){
				if ( cnt != 0 ){
					ts->diagonal_cling_reject_p->state = 3;
					SHTPS_DIAGONAL_CLING_PRINT(" state:[%d]\n", ts->diagonal_cling_reject_p->state);
				}
			} else { 
				if ( cnt == 0 ){
					SHTPS_DIAGONAL_CLING_PRINT("All finger touches up. \n");
					ts->diagonal_cling_reject_p->state = 0;
				}
			}
			if(shtps_filter_diagonal_cling_check_rezero(ts, info)){
				ts->diagonal_cling_reject_p->state = 0;
			}
		}
	}
}

/* -------------------------------------------------------------------------- */
void shtps_filter_diagonal_cling_sleep(struct shtps_rmi_spi *ts)
{
	ts->diagonal_cling_reject_p->state = 0;
}

/* -------------------------------------------------------------------------- */
void shtps_filter_diagonal_cling_init(struct shtps_rmi_spi *ts)
{
	ts->diagonal_cling_reject_p = kzalloc(sizeof(struct shtps_filter_diagonal_cling_reject_info), GFP_KERNEL);
	if(ts->diagonal_cling_reject_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return;
	}
	ts->diagonal_cling_reject_p->state = 0;
}

/* -------------------------------------------------------------------------- */
void shtps_filter_diagonal_cling_deinit(struct shtps_rmi_spi *ts)
{
	if(ts->diagonal_cling_reject_p)	kfree(ts->diagonal_cling_reject_p);
	ts->diagonal_cling_reject_p = NULL;
}

/* -------------------------------------------------------------------------- */
#endif /* SHTPS_DIAGONAL_CLING_CHECK_ENABLE */
