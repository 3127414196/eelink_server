#include <event2/event.h>
#include <curl/curl.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <event2/listener.h>

#include "log.h"
#include "version.h"
#include "server_sync.h"
#include "session.h"
#include "port.h"
#include "env.h"

struct event_base *base = NULL;


static void sig_usr(int signo)
{
	if (signo == SIGINT)
	{
		printf("oops! catch CTRL+C!!!\n");
		event_base_loopbreak(base);
	}

	if (signo == SIGTERM)
	{
		printf("oops! being killed!!!\n");
		event_base_loopbreak(base);
	}
}

int main(int argc, char **argv)
{
    int port = PORT_SYNC;

    setvbuf(stdout, NULL, _IONBF, 0);


    if (argc >= 2)
    {
    	char* strPort = argv[1];
    	int num = atoi(strPort);
    	if (num)
    	{
    		port = num;
    	}
    }

    printf("Sync Server %s, with event %s, curl %s\n",
    		VERSION_STR,
			LIBEVENT_VERSION,
			curl_version());

    base = event_base_new();
    if (!base)
        return 1; /*XXXerr*/

    int rc = log_init("../conf/sync_log.conf");

    if (rc)
    {
        LOG_ERROR("log initial failed: rc=%d", rc);
    	return rc;
    }

    if (signal(SIGINT, sig_usr) == SIG_ERR)
    {
        LOG_ERROR("Can't catch SIGINT");
    }
    if (signal(SIGTERM, sig_usr) == SIG_ERR)
    {
    	LOG_ERROR("Can't catch SIGTERM");
    }

    rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (rc != CURLE_OK)
    {
    	LOG_FATAL("curl lib initial failed:%d", rc);
    }

    struct evconnlistener*listener = server_sync(base, port);
    if (listener)
    {
        LOG_INFO("start sync server successfully at port:%d", port);
    }
    else
    {
        LOG_FATAL("start sync server failed at port:%d", port);
        return 2;
    }

    env_initial();

    //start the event loop
    LOG_INFO("start the event loop");
    event_base_dispatch(base);


    env_cleanup();

//    sk_free(SSL_COMP_get_compression_methods());
    LOG_INFO("stop mc server...");
    evconnlistener_free(listener);

    event_base_free(base);
    curl_global_cleanup();
    zlog_fini();


    return 0;
}