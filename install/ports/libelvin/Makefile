# New ports collection makefile for: libelvin
# Date created:         5 February, 2003
# Whom:                 ricci@cs.utah.edu

PORTNAME=	libelvin
PORTVERSION=	4.0.3
CATEGORIES=	misc

MAINTAINER=	ricci@cs.utah.edu

RESTRICTED=	"License prohibits redistribution"
NO_PACKAGE=	"Binary redistribution prohibited"
NO_CDROM=	"License prohibits redistribution"

.include <bsd.port.pre.mk>

.if !exists(${DISTDIR}/${DISTNAME}${EXTRACT_SUFX})
IGNORE=         may not be automatically fetched due to licensing\
		restrictions.  Please grab ${DISTFILES} from \
		http://elvin.dstc.com/projects/libelvin, and place it in \
		${DISTDIR}.
.endif

HAS_CONFIGURE=	yes
CONFIGURE_ARGS=	--enable-threads --prefix=${PREFIX}

USE_GMAKE=	yes

MAN1=		ec.1 elvin-config.1 ep.1 sha1digest.1
MAN3=		elvin_async_add_io_handler.3 elvin_async_add_quench.3 \
		elvin_async_add_subscription.3 elvin_async_add_timeout.3 \
		elvin_async_cleanup.3 elvin_async_connect.3 \
		elvin_async_default_mainloop.3 \
		elvin_async_default_mainloop_quit.3 \
		elvin_async_delete_quench.3 elvin_async_delete_subscription.3 \
		elvin_async_disconnect.3 elvin_async_init_default.3 \
		elvin_async_modify_keys.3 elvin_async_modify_quench.3 \
		elvin_async_modify_subscription.3 elvin_async_notify.3 \
		elvin_async_remove_io_handler.3 \
		elvin_async_remove_subscription.3 \
		elvin_async_remove_timeout.3 elvin_async_unotify.3 \
		elvin_attrlist.3 elvin_attrlist_add.3 elvin_attrlist_add_all.3 \
		elvin_attrlist_alloc.3 elvin_attrlist_clone.3 \
		elvin_attrlist_contains.3 elvin_attrlist_contains_all.3 \
		elvin_attrlist_contains_one.3 elvin_attrlist_del.3 \
		elvin_attrlist_del_all.3 elvin_attrlist_free.3 \
		elvin_attrlist_size.3 elvin_error.3 elvin_error_alloc.3 \
		elvin_error_clear.3 elvin_error_fprintf.3 elvin_error_free.3 \
		elvin_error_is_error.3 elvin_error_set_fini.3 \
		elvin_error_set_init.3 elvin_error_set_int32.3 \
		elvin_error_set_int64.3 elvin_error_set_real64.3 \
		elvin_error_set_string.3 elvin_error_set_uint32.3 \
		elvin_error_set_uint64.3 elvin_error_shallow_copy_into.3 \
		elvin_error_snprintf.3 elvin_handle.3 \
		elvin_handle_add_consumer_key.3 elvin_handle_add_producer_key.3 \
		elvin_handle_alloc.3 elvin_handle_append_url.3 \
		elvin_handle_clear_urls.3 elvin_handle_do_discovery_default.3 \
		elvin_handle_do_discovery_no.3 elvin_handle_do_discovery_yes.3 \
		elvin_handle_free.3 elvin_handle_get_connection_retries.3 \
		elvin_handle_get_discovery_scope.3 elvin_handle_get_failover.3 \
		elvin_handle_get_idle_period.3 elvin_handle_get_property.3 \
		elvin_handle_get_request_timeout.3 elvin_handle_get_url.3 \
		elvin_handle_get_url_details.3 elvin_handle_insert_url.3 \
		elvin_handle_num_urls.3 elvin_handle_remove_url.3 \
		elvin_handle_set_connection_retries.3 \
		elvin_handle_set_discard_func.3 \
		elvin_handle_set_discard_policy.3 \
		elvin_handle_set_discovery_scope.3 elvin_handle_set_failover.3 \
		elvin_handle_set_idle_period.3 elvin_handle_set_property.3 \
		elvin_handle_set_request_timeout.3 elvin_handle_set_sort_func.3 \
		elvin_handle_set_status_cb.3 elvin_handle_set_sub_max.3 \
		elvin_handle_url.3 elvin_keys.3 elvin_keys_add.3 \
		elvin_keys_add_all.3 elvin_keys_add_hexstring.3 \
		elvin_keys_alloc.3 elvin_keys_clone.3 elvin_keys_contains_all.3 \
		elvin_keys_contains_one.3 elvin_keys_digest_for_notification.3 \
		elvin_keys_digest_for_subscription.3 elvin_keys_free.3 \
		elvin_keys_match.3 elvin_keys_remove.3 elvin_keys_remove_all.3 \
		elvin_keys_traverse.3 elvin_memory_report.3 \
		elvin_notification.3 elvin_notification_add_av_tuple.3 \
		elvin_notification_add_int32.3 elvin_notification_add_int64.3 \
		elvin_notification_add_opaque.3 elvin_notification_add_real64.3 \
		elvin_notification_add_string.3 elvin_notification_alloc.3 \
		elvin_notification_clear.3 elvin_notification_clone.3 \
		elvin_notification_count.3 elvin_notification_free.3 \
		elvin_notification_get.3 elvin_notification_is_read_only.3 \
		elvin_notification_lookup.3 elvin_notification_remove.3 \
		elvin_notification_set_read_only.3 \
		elvin_notification_to_string.3 \
		elvin_notification_to_string_free.3 \
		elvin_notification_traverse.3 elvin_quench.3 \
		elvin_quench_alloc.3 elvin_quench_free.3 \
		elvin_sync_add_io_handler.3 elvin_sync_add_quench.3 \
		elvin_sync_add_subscription.3 elvin_sync_add_timeout.3 \
		elvin_sync_cleanup.3 elvin_sync_connect.3 \
		elvin_sync_delete_quench.3 elvin_sync_delete_subscription.3 \
		elvin_sync_disconnect.3 elvin_sync_init_default.3 \
		elvin_sync_modify_keys.3 elvin_sync_modify_quench.3 \
		elvin_sync_modify_subscription.3 elvin_sync_notify.3 \
		elvin_sync_remove_io_handler.3 elvin_sync_remove_timeout.3 \
		elvin_sync_unotify.3 elvin_threaded_add_quench.3 \
		elvin_threaded_add_subscription.3 elvin_threaded_cleanup.3 \
		elvin_threaded_connect.3 elvin_threaded_delete_quench.3 \
		elvin_threaded_delete_subscription.3 \
		elvin_threaded_disconnect.3 elvin_threaded_init_default.3 \
		elvin_threaded_modify_keys.3 elvin_threaded_modify_quench.3 \
		elvin_threaded_modify_subscription.3 elvin_threaded_notify.3 \
		elvin_threaded_unotify.3
MAN7=		elvin.7 elvin_async.7 elvin_sync.7 elvin_threaded.7

INSTALLS_SHLIB=	yes

.include <bsd.port.post.mk>
