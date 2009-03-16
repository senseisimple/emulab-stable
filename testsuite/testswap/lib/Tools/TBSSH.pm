#!/usr/bin/perl
use Modern::Perl;
use Net::SSH::Perl;
use Net::SFTP;
use Data::UUID;

package Tools::TBSSH;
use Data::Dumper;

sub uuid {
  my $ug = new Data::UUID;
  my $uuid = $ug->create_hex();
 $uuid =~ s/^0x//;
 say $uuid;
 $uuid;
}

sub path_to_last_part {
  my ($volume,$directories,$file) = File::Spec->splitpath( $_[0] );
  my @dirs = grep {/\S+/} File::Spec->splitdir( $directories );
  if ($file eq '') {
    return $dirs[$#dirs];
  }
  else {
    return $file;
  }
}


sub sshhostname {
  my ($host, $user) = @_;
  my $ssh = Net::SSH::Perl->new($host, protocol => "2", options => [ "ForwardAgent yes" ]);
  $ssh->login($user);
  print [$ssh->cmd('uname -a')]->[0];
  return $ssh
}

sub pulldirastar {
  my ($host, $user, $dir) = @_;
  my $dirpart = path_to_last_part($dir);
  my $uuid = uuid();
  my $remotename = "U$uuid";
  my $localname = "${host}_${user}_${dirpart}.tgz";
  my $ssh = Net::SSH::Perl->new($host, protocol => "2", options => [ "ForwardAgent yes" ]);
  $ssh->login($user);
  [$ssh->cmd_debug("tar zcf $remotename $dir")]->[2] && die "tar of $dir failed";

  my $sftp = Net::SFTP->new($host);
  $sftp->get($remotename, $localname);

  [$ssh->cmd("rm $remotename")]->[2] && die "rm of remote tmp file failed";
}

package Net::SSH::Perl::SSH2;
use strict;
use Net::SSH::Perl::Constants qw( :protocol :msg2 CHAN_INPUT_CLOSED CHAN_INPUT_WAIT_DRAIN );

sub cmd_debug {
    my $ssh = shift;
    my($cmd, $stdin) = @_;
    my $cmgr = $ssh->channel_mgr;
    my $channel = $ssh->_session_channel;
    $channel->open;

    $channel->register_handler(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION, sub {
        my($channel, $packet) = @_;
        $channel->{ssh}->debug("Sending command: $cmd");
        my $r_packet = $channel->request_start("exec", 0);
        $r_packet->put_str($cmd);
        $r_packet->send;

        if (defined $stdin) {
            $channel->send_data($stdin);

            $channel->drain_outgoing;
            $channel->{istate} = CHAN_INPUT_WAIT_DRAIN;
            $channel->send_eof;
            $channel->{istate} = CHAN_INPUT_CLOSED;
        }
    });

    my($exit);
    $channel->register_handler(SSH2_MSG_CHANNEL_REQUEST,
        _make_input_channel_req(\$exit));

    my $h = $ssh->{client_handlers};
    my($stdout, $stderr);
    if (my $r = $h->{stdout}) {
        $channel->register_handler("_output_buffer",
            $r->{code}, @{ $r->{extra} });
    }
    else {
        $channel->register_handler("_output_buffer", sub {
            $stdout .= $_[1]->bytes;
            syswrite STDOUT, $_[1]->bytes;
        });
    }
    if (my $r = $h->{stderr}) {
        $channel->register_handler("_extended_buffer",
            $r->{code}, @{ $r->{extra} });
    }
    else {
        $channel->register_handler("_extended_buffer", sub {
            $stderr .= $_[1]->bytes;
            syswrite STDERR, $_[1]->bytes;
        });
    }

    $ssh->debug("Entering interactive session.");
    $ssh->client_loop;

    ($stdout, $stderr, $exit);
}

1;
