/*
 * telephone_http.h
 *
 *  Created on: 2016/11/28
 *      Author: lc
 */
#ifndef SRC_TELEPHONE_HTTP_H_
#define SRC_TELEPHONE_HTTP_H_

#include <evhttp.h>

void http_replyCall(struct evhttp_request *req);
void http_replyTelephone(struct evhttp_request *req);

#endif/* SRC_TELEPHONE_HTTP_H_ */


