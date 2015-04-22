/*
 * msg_proc_mc.c
 *
 *  Created on: Apr 19, 2015
 *      Author: jk
 */
#include <assert.h>
#include <string.h>

#include "msg_proc_mc.h"
#include "msg_mc.h"
#include "object_mc.h"

#include "log.h"

#define LOG_DEBUG(...) \
	zlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_DEBUG, __VA_ARGS__)

#define LOG_INFO(...) \
	zlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_INFO, __VA_ARGS__)

#define LOG_WARNNING(...) \
	zlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_WARNNING, __VA_ARGS__)

#define LOG_ERROR(...) \
	zlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_ERROR, __VA_ARGS__)

#define LOG_FATAL(...) \
	zlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_FATAL, __VA_ARGS__)

#define LOG_DEBUG_HEX(buf, buf_len) \
	hzlog(cat[MOD_PROC_MC], __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_DEBUG, buf, buf_len)

int mc_msg_send(void* msg, size_t len, CB_CTX* ctx)
{
	MSG_SEND pfn = ctx->pSendMsg;

	pfn(ctx->bev, msg, len);

	LOG_INFO("send response msg of cmd(%d), length(%ld)", get_msg_cmd(msg), len);
	LOG_DEBUG_HEX(msg, len);

	free(msg);

	return 0;
}

int mc_login(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_LOGIN_REQ* req = msg;


	LOG_DEBUG("mc login: IMEI=%s", get_IMEI_STRING(req->IMEI));

	OBJ_MC* obj = ctx->obj;
	if (!obj)
	{
		obj = mc_obj_new();

		memcpy(obj->IMEI, req->IMEI, sizeof(IMEI_LENGTH));
		obj->language = req->language;
		obj->locale = req->locale;

		ctx->obj = obj;
	}

	MC_MSG_LOGIN_RSP *rsp = alloc_rspMsg(msg);
	if (rsp)
	{
		mc_msg_send(rsp, sizeof(MC_MSG_LOGIN_RSP), ctx);
	}

	start_fsm(ctx);

	return 0;
}

int mc_gps(const void* msg, CB_CTX* ctx __attribute__((unused)))
{
	const MC_MSG_GPS_REQ* req = msg;

	if (!req)
	{
		LOG_ERROR("msg handle empty");
		return -1;
	}

	if (req->header.length < sizeof(MC_MSG_LOGIN_REQ) - MC_MSG_HEADER_LEN)
	{
		LOG_ERROR("message length not enough");
		return -1;
	}

	LOG_INFO("GPS: lat(%d), lon(%d), speed(%d), course(%d)",
			ntohl(req->lat), ntohl(req->lon), req->speed, ntohs(req->course));

	OBJ_MC* obj = ctx->obj;
	if (obj)
	{
		obj->lat = ntohl(req->lat);
		obj->lon = ntohl(req->lon);
		obj->speed = req->speed;
		obj->course = ntohs(req->course);
		obj->cell = req->cell;
	}

	//no response message needed

	return 0;
}

int mc_ping(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_PING_REQ *req = msg;

	short status = ntohs(req->status);

	LOG_INFO("GPS located:%s", (status & 1) ? "YES" : "NO");

	MC_MSG_PING_RSP* rsp = alloc_rspMsg(msg);
	if (rsp)
	{
		mc_msg_send(rsp, sizeof(MC_MSG_PING_RSP), ctx);
	}

	return 0;
}

int mc_alarm(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_ALARM_REQ* req = msg;

	switch (req->type)
	{
	case FENCE_IN:
		LOG_INFO("FENCE_IN Alarm");
		break;
	case FENCE_OUT:
		LOG_INFO("FENCE_OUT Alarm");
		break;

	default:
		LOG_INFO("Alarm type = %#x", req->type);
	}

	OBJ_MC* obj = ctx->obj;
	if (obj)
	{
		obj->lat = ntohl(req->lat);
		obj->lon = ntohl(req->lon);
		obj->speed = req->speed;
		obj->course = ntohs(req->course);
		obj->cell = req->cell;
	}

	size_t rspMsgLength = sizeof(MC_MSG_ALARM_RSP) + 0; //TODO: currently without any message content
	MC_MSG_ALARM_RSP* rsp = alloc_msg(req->header.cmd, rspMsgLength);
	if (rsp)
	{
		set_msg_seq(rsp, get_msg_seq(req));

		mc_msg_send(rsp, rspMsgLength, ctx);
	}
	return 0;
}

int mc_status(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_STATUS_REQ* req = msg;

	OBJ_MC* obj = ctx->obj;
	if (obj)
	{
		LOG_INFO("MC(%s) Status %x", req->status);
	}
	else
	{
		LOG_INFO("MC Status %x", req->status);
	}

	if (obj)
	{
		obj->lat = ntohl(req->lat);
		obj->lon = ntohl(req->lon);
		obj->speed = req->speed;
		obj->course = ntohs(req->course);
		obj->cell = req->cell;
	}

	MC_MSG_STATUS_RSP* rsp = alloc_rspMsg(msg);
	if (rsp)
	{
		mc_msg_send(rsp, sizeof(MC_MSG_PING_RSP), ctx);
	}
	return 0;
}

int mc_sms(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_SMS_REQ* req = msg;

	MC_MSG_SMS_RSP* rsp = alloc_rspMsg(msg);
	if (rsp)
	{
		mc_msg_send(rsp, sizeof(MC_MSG_PING_RSP), ctx);
	}
	return 0;
}

int mc_operator(const void* msg, CB_CTX* ctx)
{
	const MC_MSG_OPERATOR_RSP* req = msg;

	LOG_DEBUG("MC response %s", req->data);

	return 0;
}

int mc_data(const void* msg, CB_CTX* ctx)
{
	return 0;
}