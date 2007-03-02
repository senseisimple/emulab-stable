# This file was created automatically by SWIG 1.3.29.
# Don't modify this file, modify the SWIG interface instead.
package rmcp;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package rmcpc;
bootstrap rmcp;
package rmcp;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package rmcp;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package rmcp;

*rmcp_setstrbuf = *rmcpc::rmcp_setstrbuf;
*rmcp_asf_ping = *rmcpc::rmcp_asf_ping;
*rmcp_asf_get_capabilities = *rmcpc::rmcp_asf_get_capabilities;
*rmcp_asf_get_sysstate = *rmcpc::rmcp_asf_get_sysstate;
*c_rmcp_asf_ping = *rmcpc::c_rmcp_asf_ping;
*c_rmcp_asf_get_capabilities = *rmcpc::c_rmcp_asf_get_capabilities;
*c_rmcp_asf_get_sysstate = *rmcpc::c_rmcp_asf_get_sysstate;
*rmcp_asf_reset = *rmcpc::rmcp_asf_reset;
*rmcp_asf_power_up = *rmcpc::rmcp_asf_power_up;
*rmcp_asf_power_cycle = *rmcpc::rmcp_asf_power_cycle;
*rmcp_asf_power_down = *rmcpc::rmcp_asf_power_down;
*rmcp_error_tostr = *rmcpc::rmcp_error_tostr;
*rmcp_asf_sysstate_tostr = *rmcpc::rmcp_asf_sysstate_tostr;
*rmcp_asf_wdstate_tostr = *rmcpc::rmcp_asf_wdstate_tostr;
*rmcp_rsp_rakp_msg_code_tostr = *rmcpc::rmcp_rsp_rakp_msg_code_tostr;
*rmcp_set_debug_level = *rmcpc::rmcp_set_debug_level;
*rmcp_set_enable_warn_err = *rmcpc::rmcp_set_enable_warn_err;
*rmcp_ctx_init = *rmcpc::rmcp_ctx_init;
*rmcp_ctx_setsecure = *rmcpc::rmcp_ctx_setsecure;
*rmcp_ctx_setuid = *rmcpc::rmcp_ctx_setuid;
*rmcp_open = *rmcpc::rmcp_open;
*rmcp_start_secure_session = *rmcpc::rmcp_start_secure_session;
*rmcp_finalize = *rmcpc::rmcp_finalize;

############# Class : rmcp::rmcp_ctx_t ##############

package rmcp::rmcp_ctx_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_sockfd_get = *rmcpc::rmcp_ctx_t_sockfd_get;
*swig_sockfd_set = *rmcpc::rmcp_ctx_t_sockfd_set;
*swig_client_sa_get = *rmcpc::rmcp_ctx_t_client_sa_get;
*swig_client_sa_set = *rmcpc::rmcp_ctx_t_client_sa_set;
*swig_timeout_get = *rmcpc::rmcp_ctx_t_timeout_get;
*swig_timeout_set = *rmcpc::rmcp_ctx_t_timeout_set;
*swig_retries_get = *rmcpc::rmcp_ctx_t_retries_get;
*swig_retries_set = *rmcpc::rmcp_ctx_t_retries_set;
*swig_rmcp_seqno_get = *rmcpc::rmcp_ctx_t_rmcp_seqno_get;
*swig_rmcp_seqno_set = *rmcpc::rmcp_ctx_t_rmcp_seqno_set;
*swig_rmcp_rsp_seqno_get = *rmcpc::rmcp_ctx_t_rmcp_rsp_seqno_get;
*swig_rmcp_rsp_seqno_set = *rmcpc::rmcp_ctx_t_rmcp_rsp_seqno_set;
*swig_rsp_init_get = *rmcpc::rmcp_ctx_t_rsp_init_get;
*swig_rsp_init_set = *rmcpc::rmcp_ctx_t_rsp_init_set;
*swig_secure_get = *rmcpc::rmcp_ctx_t_secure_get;
*swig_secure_set = *rmcpc::rmcp_ctx_t_secure_set;
*swig_role_get = *rmcpc::rmcp_ctx_t_role_get;
*swig_role_set = *rmcpc::rmcp_ctx_t_role_set;
*swig_role_key_get = *rmcpc::rmcp_ctx_t_role_key_get;
*swig_role_key_set = *rmcpc::rmcp_ctx_t_role_key_set;
*swig_role_key_len_get = *rmcpc::rmcp_ctx_t_role_key_len_get;
*swig_role_key_len_set = *rmcpc::rmcp_ctx_t_role_key_len_set;
*swig_gen_key_get = *rmcpc::rmcp_ctx_t_gen_key_get;
*swig_gen_key_set = *rmcpc::rmcp_ctx_t_gen_key_set;
*swig_gen_key_len_get = *rmcpc::rmcp_ctx_t_gen_key_len_get;
*swig_gen_key_len_set = *rmcpc::rmcp_ctx_t_gen_key_len_set;
*swig_uid_len_get = *rmcpc::rmcp_ctx_t_uid_len_get;
*swig_uid_len_set = *rmcpc::rmcp_ctx_t_uid_len_set;
*swig_uid_get = *rmcpc::rmcp_ctx_t_uid_get;
*swig_uid_set = *rmcpc::rmcp_ctx_t_uid_set;
*swig_msid_get = *rmcpc::rmcp_ctx_t_msid_get;
*swig_msid_set = *rmcpc::rmcp_ctx_t_msid_set;
*swig_csid_get = *rmcpc::rmcp_ctx_t_csid_get;
*swig_csid_set = *rmcpc::rmcp_ctx_t_csid_set;
*swig_mrand_get = *rmcpc::rmcp_ctx_t_mrand_get;
*swig_mrand_set = *rmcpc::rmcp_ctx_t_mrand_set;
*swig_crand_get = *rmcpc::rmcp_ctx_t_crand_get;
*swig_crand_set = *rmcpc::rmcp_ctx_t_crand_set;
*swig_cGUID_get = *rmcpc::rmcp_ctx_t_cGUID_get;
*swig_cGUID_set = *rmcpc::rmcp_ctx_t_cGUID_set;
*swig_sik_get = *rmcpc::rmcp_ctx_t_sik_get;
*swig_sik_set = *rmcpc::rmcp_ctx_t_sik_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_ctx_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_ctx_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_supported_t ##############

package rmcp::rmcp_asf_supported_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_iana_id_get = *rmcpc::rmcp_asf_supported_t_iana_id_get;
*swig_iana_id_set = *rmcpc::rmcp_asf_supported_t_iana_id_set;
*swig_oem_get = *rmcpc::rmcp_asf_supported_t_oem_get;
*swig_oem_set = *rmcpc::rmcp_asf_supported_t_oem_set;
*swig_ipmi_get = *rmcpc::rmcp_asf_supported_t_ipmi_get;
*swig_ipmi_set = *rmcpc::rmcp_asf_supported_t_ipmi_set;
*swig_asf_1_get = *rmcpc::rmcp_asf_supported_t_asf_1_get;
*swig_asf_1_set = *rmcpc::rmcp_asf_supported_t_asf_1_set;
*swig_secure_rmcp_get = *rmcpc::rmcp_asf_supported_t_secure_rmcp_get;
*swig_secure_rmcp_set = *rmcpc::rmcp_asf_supported_t_secure_rmcp_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_supported_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_supported_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_capabilities_t ##############

package rmcp::rmcp_asf_capabilities_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_iana_id_get = *rmcpc::rmcp_asf_capabilities_t_iana_id_get;
*swig_iana_id_set = *rmcpc::rmcp_asf_capabilities_t_iana_id_set;
*swig_oem_get = *rmcpc::rmcp_asf_capabilities_t_oem_get;
*swig_oem_set = *rmcpc::rmcp_asf_capabilities_t_oem_set;
*swig_firmware_capabilities_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_get;
*swig_firmware_capabilities_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_set;
*swig_capabilities_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_get;
*swig_capabilities_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_set;
*swig_commands_get = *rmcpc::rmcp_asf_capabilities_t_commands_get;
*swig_commands_set = *rmcpc::rmcp_asf_capabilities_t_commands_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_capabilities_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_capabilities_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_capabilities_t_firmware_capabilities ##############

package rmcp::rmcp_asf_capabilities_t_firmware_capabilities;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_sleep_button_lock_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_sleep_button_lock_get;
*swig_sleep_button_lock_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_sleep_button_lock_set;
*swig_lock_keyboard_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_lock_keyboard_get;
*swig_lock_keyboard_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_lock_keyboard_set;
*swig_reset_button_lock_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_reset_button_lock_get;
*swig_reset_button_lock_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_reset_button_lock_set;
*swig_power_button_lock_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_power_button_lock_get;
*swig_power_button_lock_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_power_button_lock_set;
*swig_firmware_screen_blank_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_screen_blank_get;
*swig_firmware_screen_blank_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_screen_blank_set;
*swig_config_data_reset_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_config_data_reset_get;
*swig_config_data_reset_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_config_data_reset_set;
*swig_firmware_quiet_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_quiet_get;
*swig_firmware_quiet_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_quiet_set;
*swig_firmware_verbose_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_verbose_get;
*swig_firmware_verbose_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_firmware_verbose_set;
*swig_forced_progress_events_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_forced_progress_events_get;
*swig_forced_progress_events_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_forced_progress_events_set;
*swig_user_passwd_bypass_get = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_user_passwd_bypass_get;
*swig_user_passwd_bypass_set = *rmcpc::rmcp_asf_capabilities_t_firmware_capabilities_user_passwd_bypass_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_capabilities_t_firmware_capabilities(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_capabilities_t_firmware_capabilities($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_capabilities_t_capabilities ##############

package rmcp::rmcp_asf_capabilities_t_capabilities;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_reset_both_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_reset_both_get;
*swig_reset_both_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_reset_both_set;
*swig_power_up_both_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_up_both_get;
*swig_power_up_both_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_up_both_set;
*swig_power_down_both_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_down_both_get;
*swig_power_down_both_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_down_both_set;
*swig_power_cycle_both_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_cycle_both_get;
*swig_power_cycle_both_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_cycle_both_set;
*swig_reset_secure_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_reset_secure_get;
*swig_reset_secure_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_reset_secure_set;
*swig_power_up_secure_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_up_secure_get;
*swig_power_up_secure_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_up_secure_set;
*swig_power_down_secure_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_down_secure_get;
*swig_power_down_secure_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_down_secure_set;
*swig_power_cycle_secure_get = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_cycle_secure_get;
*swig_power_cycle_secure_set = *rmcpc::rmcp_asf_capabilities_t_capabilities_power_cycle_secure_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_capabilities_t_capabilities(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_capabilities_t_capabilities($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_capabilities_t_commands ##############

package rmcp::rmcp_asf_capabilities_t_commands;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_force_cd_boot_get = *rmcpc::rmcp_asf_capabilities_t_commands_force_cd_boot_get;
*swig_force_cd_boot_set = *rmcpc::rmcp_asf_capabilities_t_commands_force_cd_boot_set;
*swig_force_diag_boot_get = *rmcpc::rmcp_asf_capabilities_t_commands_force_diag_boot_get;
*swig_force_diag_boot_set = *rmcpc::rmcp_asf_capabilities_t_commands_force_diag_boot_set;
*swig_force_hdd_safe_boot_get = *rmcpc::rmcp_asf_capabilities_t_commands_force_hdd_safe_boot_get;
*swig_force_hdd_safe_boot_set = *rmcpc::rmcp_asf_capabilities_t_commands_force_hdd_safe_boot_set;
*swig_force_hdd_boot_get = *rmcpc::rmcp_asf_capabilities_t_commands_force_hdd_boot_get;
*swig_force_hdd_boot_set = *rmcpc::rmcp_asf_capabilities_t_commands_force_hdd_boot_set;
*swig_force_pxe_boot_get = *rmcpc::rmcp_asf_capabilities_t_commands_force_pxe_boot_get;
*swig_force_pxe_boot_set = *rmcpc::rmcp_asf_capabilities_t_commands_force_pxe_boot_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_capabilities_t_commands(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_capabilities_t_commands($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : rmcp::rmcp_asf_sysstate_t ##############

package rmcp::rmcp_asf_sysstate_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( rmcp );
%OWNER = ();
%ITERATORS = ();
*swig_sysstate_get = *rmcpc::rmcp_asf_sysstate_t_sysstate_get;
*swig_sysstate_set = *rmcpc::rmcp_asf_sysstate_t_sysstate_set;
*swig_wdstate_get = *rmcpc::rmcp_asf_sysstate_t_wdstate_get;
*swig_wdstate_set = *rmcpc::rmcp_asf_sysstate_t_wdstate_set;
sub new {
    my $pkg = shift;
    my $self = rmcpc::new_rmcp_asf_sysstate_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        rmcpc::delete_rmcp_asf_sysstate_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package rmcp;

*RMCP_COMPAT_PORT = *rmcpc::RMCP_COMPAT_PORT;
*RMCP_SECURE_PORT = *rmcpc::RMCP_SECURE_PORT;
*RMCP_ROLE_OP = *rmcpc::RMCP_ROLE_OP;
*RMCP_ROLE_ADM = *rmcpc::RMCP_ROLE_ADM;
*RMCP_SUCCESS = *rmcpc::RMCP_SUCCESS;
*RMCP_ERR_ALREADY_CONNECTED = *rmcpc::RMCP_ERR_ALREADY_CONNECTED;
*RMCP_ERR_SOCKET = *rmcpc::RMCP_ERR_SOCKET;
*RMCP_ERR_BAD_CLIENT_NAME = *rmcpc::RMCP_ERR_BAD_CLIENT_NAME;
*RMCP_ERR_NO_DATA = *rmcpc::RMCP_ERR_NO_DATA;
*RMCP_ERR_UNKNOWN_CLASS = *rmcpc::RMCP_ERR_UNKNOWN_CLASS;
*RMCP_ERR_NO_ACK_NEEDED = *rmcpc::RMCP_ERR_NO_ACK_NEEDED;
*RMCP_ERR_NO_ACK_INVALID = *rmcpc::RMCP_ERR_NO_ACK_INVALID;
*RMCP_ERR_TIMEOUT = *rmcpc::RMCP_ERR_TIMEOUT;
*RMCP_ERR_MAX_RETRIES = *rmcpc::RMCP_ERR_MAX_RETRIES;
*RMCP_ERR_PROTOCOL = *rmcpc::RMCP_ERR_PROTOCOL;
*RMCP_ERR_NO_SECURE_CREDS = *rmcpc::RMCP_ERR_NO_SECURE_CREDS;
*RMCP_FLAG_SECURE_B = *rmcpc::RMCP_FLAG_SECURE_B;
*RMCP_FLAG_MAX_B = *rmcpc::RMCP_FLAG_MAX_B;
*RMCP_FLAG_SECURE = *rmcpc::RMCP_FLAG_SECURE;
*RMCP_FLAG_MAX = *rmcpc::RMCP_FLAG_MAX;
*RMCP_ASF_SYSSTATE_S0_G0 = *rmcpc::RMCP_ASF_SYSSTATE_S0_G0;
*RMCP_ASF_SYSSTATE_S1 = *rmcpc::RMCP_ASF_SYSSTATE_S1;
*RMCP_ASF_SYSSTATE_S2 = *rmcpc::RMCP_ASF_SYSSTATE_S2;
*RMCP_ASF_SYSSTATE_S3 = *rmcpc::RMCP_ASF_SYSSTATE_S3;
*RMCP_ASF_SYSSTATE_S4 = *rmcpc::RMCP_ASF_SYSSTATE_S4;
*RMCP_ASF_SYSSTATE_S5_G2 = *rmcpc::RMCP_ASF_SYSSTATE_S5_G2;
*RMCP_ASF_SYSSTATE_S4_S5 = *rmcpc::RMCP_ASF_SYSSTATE_S4_S5;
*RMCP_ASF_SYSSTATE_G3 = *rmcpc::RMCP_ASF_SYSSTATE_G3;
*RMCP_ASF_SYSSTATE_S1_S2_S3 = *rmcpc::RMCP_ASF_SYSSTATE_S1_S2_S3;
*RMCP_ASF_SYSSTATE_G1 = *rmcpc::RMCP_ASF_SYSSTATE_G1;
*RMCP_ASF_SYSSTATE_S5_OVERRIDE = *rmcpc::RMCP_ASF_SYSSTATE_S5_OVERRIDE;
*RMCP_ASF_SYSSTATE_LON = *rmcpc::RMCP_ASF_SYSSTATE_LON;
*RMCP_ASF_SYSSTATE_LOFF = *rmcpc::RMCP_ASF_SYSSTATE_LOFF;
*RMCP_ASF_SYSSTATE_UNKNOWN = *rmcpc::RMCP_ASF_SYSSTATE_UNKNOWN;
*RMCP_ASF_SYSSTATE_RESERVED = *rmcpc::RMCP_ASF_SYSSTATE_RESERVED;
*RMCP_ASF_WDSTATEMASK_EXPIRED = *rmcpc::RMCP_ASF_WDSTATEMASK_EXPIRED;
1;
