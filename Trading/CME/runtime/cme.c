#include <stdio.h>
#include <unistd.h>
#include <maxmpt/cme/mdp/mdp.h>
#include <maxmpt/cme/ilink/ilink.h>

extern max_file_t* CmeTrading_init(void);

int main(int argc, char *argv[]) {
	if(argc != 4) {
		printf("Usage: %s <topIp>:<topNetmask> <botIp>:<botNetmask> <remoteIp>:<remotePort>\n", argv[0]);
		return 1;
	}

	char top_ip_str[16];
	char top_netmask_str[16];
	char bot_ip_str[16];
	char bot_netmask_str[16];
	char remote_ip_str[16];
	uint remote_port;

	if (sscanf(argv[1], "%15[0-9.]:%15[0-9.]", top_ip_str, top_netmask_str) != 2) {
		printf("Failed to parse top port IP and netmask");
		return 1;
	}

	if (sscanf(argv[2], "%15[0-9.]:%15[0-9.]", bot_ip_str, bot_netmask_str) != 2) {
		printf("Failed to parse bot port IP and netmask");
		return 1;
	}

	if (sscanf(argv[3], "%15[0-9.]:%u", remote_ip_str, &remote_port) != 2) {
		printf("Failed to parse remote IP and port");
		return 1;
	}

	struct in_addr top_ip      = { inet_addr(top_ip_str) };
	struct in_addr top_netmask = { inet_addr(top_netmask_str) };
	struct in_addr bot_ip      = { inet_addr(bot_ip_str) };
	struct in_addr bot_netmask = { inet_addr(bot_netmask_str) };
	struct in_addr remote_ip   = { inet_addr(remote_ip_str) };

	max_file_t*   maxfile = CmeTrading_init();
	max_engine_t* engine  = max_load(maxfile, "*");

	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &top_ip, &top_netmask);
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &bot_ip, &bot_netmask);

	max_cme_mdp_handle_t* mdp;
	if (max_cme_mdp_open_handler(engine, &mdp, "md", "templates_FixBinary.xml", "config.xml")) {
		printf("Failed to open market data handler.\nError trace: %s\n", max_cme_mdp_get_error_trace(mdp));
		return 1;
	}

	max_cme_ilink_handle_t* ilink;
	// The NULL arguments here are the iLink output folders. Passing NULL disables iLink logging and session persistence.
	if (max_cme_ilink_open_handler(&ilink, engine, "orderEntry", NULL, NULL)) {
		printf("Failed to open iLink handler.\nError trace: %s\n", max_cme_ilink_get_error_trace(ilink));
		return 1;
	}

	struct timeval timeout = {5, 0}; // 5 seconds
	max_cme_ilink_session_t* session = max_cme_ilink_session_connect(ilink, &remote_ip, remote_port, &timeout);
	if (!session) {
		printf("Failed to connect iLink session.\nError trace: %s\n", max_cme_ilink_get_error_trace(ilink));
		return 1;
	}

	const char* session_id    = "ABC";
	const char* sender_sub_id = "subID";
	const char* market_seg_id = "segID";
	const char* target_sub_id = "Send123";
	const char* firm_id       = "123";
	const char* location_id   = "GB";
	const char* password      = "password";
	const int   heartbeat_interval = 30;

	if (max_cme_ilink_session_login(
			session,
			session_id,
			sender_sub_id,
			firm_id,
			market_seg_id,
			MAXCMEILINK_CUSTOMERORFIRM_CUSTOMER,
			MAXCMEILINK_CTICODE_1,
			location_id,
			password,
			MAXELER_APPLICATION_SYSTEM_NAME,
			MAXELER_TRADING_SYSTEM_VERSION,
			MAXELER_APPLICATION_SYSTEM_VENDOR,
			target_sub_id,
			heartbeat_interval,
			&timeout))
	{
		printf("Failed to login iLink session.\nError trace: %s\n", max_cme_ilink_get_error_trace(ilink));
		return 1;
	}

	// allow iLink to send some orders from the hardware
	if (max_cme_ilink_alloc_orders(session, 512)) {
		printf("Failed to alloc orders.\nError trace: %s\n", max_cme_ilink_get_error_trace(ilink));
		return 1;
	}

	const uint channel_id   = 342;
	const uint security_id  = 295722;
	const uint market_depth = 10;

	const max_cme_mdp_channel_t* channel = max_cme_mdp_get_channel_by_id(mdp, channel_id);
	if (!channel) {
		printf("Failed to find channel with id '%d'.\nError trace: %s\n", channel_id, max_cme_mdp_get_error_trace(mdp));
		return 1;
	}

	max_cme_mdp_instrument_t* instrument = max_cme_mdp_new_instrument(mdp, channel, security_id, market_depth);
	if (!instrument) {
		printf("Failed to create new instrument.\nError trace: %s\n", max_cme_mdp_get_error_trace(mdp));
		return 1;
	}

	if (max_cme_mdp_add_instrument(mdp, instrument) < 0) {
		printf("Failed to add instrument.\nError trace: %s\n", max_cme_mdp_get_error_trace(mdp));
		return 1;
	}

	// Don't wait for iLink events here as market data events might pile up.
	struct timeval no_wait = {0};

	while (1) {
		int activity = 0;

		max_cme_mdp_event_t* md_event;
		while (max_cme_mdp_get_next_event(mdp, &md_event) == 1) {
			max_cme_mdp_print_event(md_event);
			max_cme_mdp_destroy_event(md_event);
			activity = 1;
		}

		max_cme_ilink_session_event_t session_event;
		while (max_cme_ilink_get_next_session_event(session, &session_event, &no_wait) == 1) {
			max_cme_ilink_display_session_event(&session_event);
			activity = 1;
		}

		max_cme_ilink_order_event_t order_event;
		while (max_cme_ilink_get_next_order_event(session, &order_event, &no_wait) == 1) {
			max_cme_ilink_display_order_event(&order_event);
			activity = 1;
		}

		if (!activity)
			usleep(1000);
	}

	return 0;
}
