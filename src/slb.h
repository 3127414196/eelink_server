/*
 * slb.h
 *
 *  Created on: 2015��6��5��
 *      Author: jk
 */

#ifndef SRC_SLB_H_
#define SRC_SLB_H_

#include <event2/event.h>

struct evconnlistener* slb_start(struct event_base*, int);

#endif /* SRC_SLB_H_ */
