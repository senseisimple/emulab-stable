#include <arpa/inet.h>

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"

#include "libtmcd.h"

module MODULE_VAR_EXPORT tmcd_module;

static char **make_argv(char *args, int *argc, char separator)
{
	char **argv = NULL;
	char *p, *token;
	int count;
	int i;

	count = 1;
	p = args;
	while (*p) {
		if (*p == separator) {
			count++;
		}

		p++;
	}

	argv = malloc(sizeof(char *) * count);
	if (argv == NULL)
		goto err;
	memset(argv, 0, sizeof(char *) * count);

	p = args;
	token = args;
	i = 0;
	while (*p) {
		if (*p == separator) {
			*p = '\0';
			argv[i] = token;
			token = p + 1;
			i++;
		}

		p++;
	}
	argv[i] = token;

	*argc = count;
err:
	return argv;
}

static int handle_request(request_rec *r)
{
	tmcdreq_t tmcdreq, *reqp = &tmcdreq;
	tmcdresp_t *response = NULL;
	char *command;
	struct in_addr local_addr;
	struct in_addr remote_addr;
	struct sockaddr_in redir_client;
	int tmcd_status;
	int status = OK;
	char *status_line = NULL;
	char *args = NULL;
	char *function_args = NULL;
	char *p;
	char **argv = NULL;
	int argc, i;

	reqp->istcp = 1;
	reqp->isssl = 1; /* FIXME */

	if (strcmp(r->handler, "tmcd")) {
		status = DECLINED;
		goto err;
	}

#if 0
	r->allowed |= (AP_METHOD_BIT << M_GET);
	if (r->method_number != M_GET) {
		status = DECLINED;
		goto err;
	}
#endif

	memset(reqp, 0, sizeof(*reqp));

	local_addr = r->connection->local_addr.sin_addr;
	remote_addr = r->connection->remote_addr.sin_addr;

	reqp->version = 1; /* FIXME need sane default */
	tmcd_init(reqp, &local_addr, NULL);

	command = r->path_info;
	while (*command && *command == '/') {
		command++;
	}
	if (command[0] == '\0') {
		status = HTTP_BAD_REQUEST;
		goto err;
	}

	if (r->args) {
		args = malloc(strlen(r->args) + 1);
		if (args == NULL) {
			status = HTTP_INTERNAL_SERVER_ERROR;
			goto err;
		}

		strcpy(args, r->args);
		argv = make_argv(args, &argc, '&');
		if (argv == NULL) {
			status = HTTP_INTERNAL_SERVER_ERROR;
			goto err;
		}

		for (i = 0; i < argc; i++) {
			/* Unescape the arguments */
			p = args;
			while (*p) {
				if (*p == '+')
					*p = ' ';
				p++;
			}

			status = ap_unescape_url(args);
			if (status != OK) {
				goto err;
			}

			if (strncasecmp(argv[i], "version=", 8) == 0) {
				long version;
				char *end;
				version = strtol(argv[i] + 8, &end, 10);
				if (*end != '\0' || *(argv[i] + 8) == '\0') {
					status = HTTP_BAD_REQUEST;
					status_line = "Invalid Version";
					goto err;
				}

				reqp->version = version;
			} else if (strncasecmp(argv[i], "redirect=", 9) == 0) {
				if (inet_pton(AF_INET, argv[i] + 9,
				              &redir_client.sin_addr) <= 0) {
					status = HTTP_BAD_REQUEST;
					status_line = "Invalid IP Address";
					goto err;
				}
				/* FIXME info message */

				if (remote_addr.s_addr != local_addr.s_addr) {
					status = HTTP_FORBIDDEN;
					status_line = "Redirection Not Allowed";
					goto err;
				}

				remote_addr =
				    redir_client.sin_addr;

			} else if (strncasecmp(argv[i], "vnodeid=", 8) == 0) {
				if (strlen(argv[i] + 8) >=
				           sizeof(reqp->vnodeid)) {
					status = HTTP_BAD_REQUEST;
					status_line =
					    "Virtual Node ID Too Long";
					goto err;
				}
				reqp->isvnode = 1;
				strcpy(reqp->vnodeid, argv[i] + 8);
			} else if (strncasecmp(argv[i], "args=", 5) == 0) {
				function_args = argv[i] + 5;
			}
		}

	}

	/* FIXME handle wanodekey */
	if ((tmcd_status = iptonodeid(reqp, remote_addr, NULL))) {
		if (reqp->isvnode) {
			status_line = "Invalid Virtual Node";
		}
		else {
			status_line = "Invalid Node";
		}
		status = HTTP_NOT_FOUND;
		goto err;
	}

	if (reqp->tmcd_redirect[0]) {
		/* FIXME what if https should be used? */
		/* FIXME do I need to specify the args should be passed too? */
		char *uri = ap_psprintf(r->pool, "http://%s%s?%s", reqp->tmcd_redirect,
		                        r->uri, r->args);
		ap_table_setn(r->headers_out, "Location", uri);
		status = HTTP_MOVED_TEMPORARILY;
		goto done;
	}

	tmcd_status = tmcd_handle_request(reqp, &response, command,
	                                  function_args);

	if (tmcd_status == TMCD_STATUS_OK) {
		r->content_type = response->type;
		ap_set_content_length(r, response->length);
		/* FIXME doctype */
		ap_soft_timeout("tmcd response call trace", r);
		ap_send_http_header(r);
		ap_rprintf(r, "%s", response->data);
		ap_kill_timeout(r);
		status = OK;
		goto done;
	} else {
		switch(tmcd_status) {
			case TMCD_STATUS_UNKNOWN_COMMAND:
				status = HTTP_NOT_FOUND;
				status_line = "Unknown Command";
				break;
			case TMCD_STATUS_REQUIRES_ENCRYPTION:
				status = HTTP_FORBIDDEN;
				status_line = "SSL Required";
				break;
			case TMCD_STATUS_NODE_NOT_ALLOCATED:
				status = HTTP_FORBIDDEN;
				status_line = "Node Not Allocated";
				break;
			case TMCD_STATUS_COMMAND_FAILED:
				status = HTTP_INTERNAL_SERVER_ERROR;
				if (response && response->data) {
					status_line = response->data;
				}
				break;
			case TMCD_STATUS_MALLOC_FAILED:
				status = HTTP_INTERNAL_SERVER_ERROR;
				break;
		}

		goto err;
	}
err:
done:
	if (argv)
		free(argv);

	if (args)
		free(args);

	if (response)
		tmcd_free_response(response);

	if (status_line) {
		r->status_line = ap_psprintf(r->pool, "%3.3u %s", status,
		                             status_line);
	}
	return status;
}

static const handler_rec tmcd_handlers[] =
{
	{"tmcd", handle_request},
	{NULL}
};

module MODULE_VAR_EXPORT tmcd_module = {
	STANDARD_MODULE_STUFF,
	NULL,               /* module initializer */
	NULL,  /* per-directory config creator */
	NULL,   /* dir config merger */
	NULL,       /* server config creator */
	NULL,        /* server config merger */
	NULL,               /* command table */
	tmcd_handlers,           /* [9] list of handlers */
	NULL,  /* [2] filename-to-URI translation */
	NULL,      /* [5] check/validate user_id */
	NULL,       /* [6] check user_id is valid *here* */
	NULL,     /* [4] check access by host address */
	NULL,       /* [7] MIME type checker/setter */
	NULL,        /* [8] fixups */
	NULL,             /* [10] logger */
#if MODULE_MAGIC_NUMBER >= 19970103
	NULL,      /* [3] header parser */
#endif
#if MODULE_MAGIC_NUMBER >= 19970719
	NULL,         /* process initializer */
#endif
#if MODULE_MAGIC_NUMBER >= 19970728
	NULL,         /* process exit/cleanup */
#endif
#if MODULE_MAGIC_NUMBER >= 19970902
	NULL  /* [1] post read_request handling */
#endif
};
