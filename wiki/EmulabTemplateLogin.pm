# Module of TWiki Enterprise Collaboration Platform, http://TWiki.org/
#
# Copyright (C) 2005-2006 TWiki Contributors. All Rights Reserved.
# TWiki Contributors
# are listed in the AUTHORS file in the root of this distribution.
# NOTE: Please extend that file, not this notice.
#
# Additional copyrights apply to some or all of the code in this
# file as follows:
# Copyright (C) 2005 Greg Abbas, twiki@abbas.org
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version. For
# more details read LICENSE in the root of this distribution.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# As per the GPL, removal of this notice is prohibited.

=pod

---+ package TWiki::Client::EmulabTemplateLogin

This is a login manager that you can specify in the security setup section of [[%SCRIPTURL%/configure%SCRIPTSUFFIX%][configure]]. It provides users with a template-based form to enter usernames and passwords, and works with the PasswordManager that you specify to verify those passwords.

Subclass of TWiki::Client; see that class for documentation of the
methods of this class.

=cut

package TWiki::Client::EmulabTemplateLogin;

use strict;
use Assert;
use TWiki::Client;

@TWiki::Client::EmulabTemplateLogin::ISA = ( 'TWiki::Client' );

sub new {
    my( $class, $session ) = @_;

    my $this = bless( $class->SUPER::new($session), $class );
    $session->enterContext( 'can_login' );
    return $this;
}

# Triggered on auth fail
sub forceAuthentication {
    my $this = shift;
    my $twiki = $this->{twiki};

    unless( $twiki->inContext( 'authenticated' )) {
        my $query = $twiki->{cgiQuery};
        # SMELL CGI::url drops the anchor. Report as bug against CGI::?
        $query->param( origurl => $query->url( -path => 1, -query => 1 ));
        $this->login( $query, $twiki );
        return 1;
    }
    return undef;
}

# Content of a login link
sub loginUrl {
    my $this = shift;
    my $twiki = $this->{twiki};
    my $topic = $twiki->{topicName};
    my $web = $twiki->{webName};
    my $query = $twiki->{cgiQuery};
    return $twiki->getScriptUrl( 0, 'login', $web, $topic,
                                 origurl => $query->url( -path => 1,
                                                         -query => 1 ), @_ );
}

sub findwikiname {
    my ($loginUser) = @_;
    
    if (!$loginUser) {
        return "";
    }
    my $wikiname;
    my $datadir = $TWiki::cfg{DataDir};
    my %MAPDB   = ();
	
    if (dbmopen(%MAPDB, "${datadir}/.usermap", 0644)) {
	if (defined($MAPDB{$loginUser})) {
	    $wikiname = $MAPDB{$loginUser};
	}
	dbmclose(%MAPDB);
    }
    return $loginUser
	if (!defined($wikiname));

    return $wikiname;
}

=pod

---++ ObjectMethod login( $query, $twiki )

If a login name and password have been passed in the query, it
validates these and if authentic, redirects to the original
script. If there is no username in the query or the username/password is
invalid (validate returns non-zero) then it prompts again.

The password handler is expected to return a perl true value if the password
is valid. This return value is stored in a session variable called
VALIDATION. This is so that password handlers can return extra information
about the user, such as a list of TWiki groups stored in a separate
database, that can then be displayed by referring to
%<nop>SESSION_VARIABLE{"VALIDATION"}%

=cut

sub login {
    my( $this, $query, $twikiSession ) = @_;
    my $twiki = $this->{twiki};

    my $origurl = $query->param( 'origurl' );
    my $loginName = $query->param( 'username' );
    my $loginPass = $query->param( 'password' );
    my $bosscred  = $query->param( 'bosscred' );

    my $tmpl = $twiki->{templates}->readTemplate(
        'login', $twiki->getSkin() );

    my $banner = $twiki->{templates}->expandTemplate( 'LOG_IN_BANNER' );
    my $note = '';
    my $topic = $twiki->{topicName};
    my $web = $twiki->{webName};

    my $cgisession = $this->{cgisession};

    if( $cgisession && $cgisession->param( 'AUTHUSER' ) &&
          $loginName ne $cgisession->param( 'AUTHUSER' )) {
        $banner = $twiki->{templates}->expandTemplate( 'LOGGED_IN_BANNER' );
        $note = $twiki->{templates}->expandTemplate( 'NEW_USER_NOTE' );
    }

    #
    # If bosscred provided, boss is trying to autologin the user using
    # a key that it sent across via a backend script and stashed in the
    # cookie dir. Find that file, compare the keys and if the key is not
    # too terribly old, give the user the nod.
    #
    if ($bosscred) {
	if (!$loginName || $loginName eq "") {
	    return undef;
	}
	$banner = $twiki->{templates}->expandTemplate('UNRECOGNISED_USER');
	my $CREDDIR = "/var/db/cgisess";
	my $file    = "${CREDDIR}/$loginName";

	if (! -e $file) {
	    $twiki->writeWarning("Cred file $file does not exist!");
	    goto denied;
	}
	if (!open(COK, $file)) {
	    $twiki->writeWarning("Cannot open cred file $file!");
	    goto denied;
	}
	my $cred  = <COK>;
	my $stamp = <COK>;
	close(COK);

	# Compare credentials.
	if ($cred) {
	    chomp($cred);
	    if ($cred eq $bosscred) {
		#
		# Find the wikiname.
		#
		my $wikiname = findwikiname($loginName);
		my $redurl   = $query->param('redurl');
		
		$this->userLoggedIn($wikiname);
		$cgisession->param( 'VALIDATION', 1 ) if $cgisession;
		
		if( !$redurl || $redurl eq $query->url() ) {
		    $redurl = $twiki->getScriptUrl( 0, 'view', $web, $topic );
		}
		else {
		    $redurl = $twiki->getScriptUrl( 0, 'view', $web, $redurl );
		}
		
		$this->redirectCgiQuery( $query, $redurl );
		return;
	    }
	}
	# Does not match.
	$twiki->writeWarning("cred mismatch for $loginName; $cred,$bosscred!");
	goto denied;
    }

    if( $loginName ) {
        my $passwordHandler = $twiki->{users}->{passwords};
        my $validation = $passwordHandler->checkPassword( $loginName, $loginPass );

        if( $validation ) {
            $this->userLoggedIn( $loginName );
            $cgisession->param( 'VALIDATION', $validation ) if $cgisession;
            if( !$origurl || $origurl eq $query->url() ) {
                $origurl = $twiki->getScriptUrl( 0, 'view', $web, $topic );
            }
            $this->redirectCgiQuery( $query, $origurl );
            return;
        } else {
            $banner = $twiki->{templates}->expandTemplate('UNRECOGNISED_USER');
        }
    }
  denied:

    # TODO: add JavaScript password encryption in the template
    # to use a template)
    $origurl ||= '';
    $tmpl =~ s/%ORIGURL%/$origurl/g;
    $tmpl =~ s/%BANNER%/$banner/g;
    $tmpl =~ s/%NOTE%/$note/g;

    $tmpl = $twiki->handleCommonTags( $tmpl, $web, $topic );
    $tmpl = $twiki->{renderer}->getRenderedVersion( $tmpl, '' );
    $tmpl =~ s/<nop>//g;
    $twiki->writePageHeader( $query );
    print $tmpl;
}

1;
