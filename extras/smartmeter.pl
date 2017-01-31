#!/usr/bin/perl -w

#
# Copyright (c) 2017 Jan Fredrik Leversund
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 

use POSIX "setsid";
use Net::MQTT::Simple;
use Time::HiRes qw(time);
use DBI;
use File::Path qw(make_path);

use constant MQTT_SERVER => "autolycus.radical.org";
use constant SMARTMETER_HOME => "$ENV{HOME}/.smartmeter";
use constant DATABASE_FILE => "smartmeter.db";
use constant LOG_FILE => "smartmeter.log";
use constant VOLTAGE => 230; # I happen to know that this is the voltage in my house.

sub daemonize;
sub create_tables;

# Globals
my $last_count = undef;
my $last_time = undef;


make_path(SMARTMETER_HOME);

-d SMARTMETER_HOME and -w SMARTMETER_HOME or
	die "Unable to access smartmeter data dir '" . SMARTMETER_HOME . "'";

my $dbh = DBI->connect("dbi:SQLite:dbname=" . SMARTMETER_HOME . "/" . DATABASE_FILE , "", "")
	or die $DBI::errstr;

create_tables;

my $sth = $dbh->prepare(q{
	insert into
		meter_reading
	(
		meter_id,
		read_time,
		read_period,
		delta_count,
		watts,
		ampere
	)
	values
	(
		?,
		?,
		?,
		?,
		?,
		?
	)
}) or die $DBI::errstr;

my $mqtt = Net::MQTT::Simple->new(MQTT_SERVER) ;

print "Everything seems okay; daemonizing...\n";
daemonize;

$mqtt->run(
	"smartmeter/+/Wh" => sub {
		my ($time, $topic, $count) = (time, @_);

		unless (defined $last_count)
		{
			# The first time we always seem to get a count of 0, so we'll ignore it.
			if ($count)
			{
				# The second time around we get a real reading from the smartmeter, so we save the count and value
				$last_count = $count;
				$last_time = $time;
			}
			return;
		}

		# Grab the meter id.
		$topic =~ /smartmeter\/(\d+)\//gio;
		my $id = $1;

		# Calculate the delta count and time difference between this and the previous reading.
		my $dc = $count - $last_count;
		my $dt = $time - $last_time;

		# Keep the current readings around until the next time.
		$last_count = $count;
		$last_time = $time;

		# A negative delta count probably means rollover or smartmeter reset. We'll ignore this reading.
		return if ($dc < 0);

		# Calculate the average wattage per second over the previous period.
		my $watts = sprintf("%.f", 3600 * $dc / $dt); 
		my $ampere = sprintf("%.1f", (3600 * $dc / $dt) / VOLTAGE);

#		print '#' x 40 . "\n";
#		print "Topic:            $topic\n";
#		print "Count:            $count\n";
#		print "Period count:     $dc\n";
#		print "Measuring period: $dt\n";
#		print "Watts:            $watts\n";
#		print "Ampere:           $ampere\n";
#		print "Meter ID:         $id\n\n";

		$mqtt->publish("smartmeter/$id/W" => $watts);
		$mqtt->publish("smartmeter/$id/A" => $ampere);

		$sth->execute($id, $time, $dt, $dc, $watts, $ampere)
			or die $DBI::errstr;
	}
);


sub create_tables
{
	$dbh->do(q{
		create table if not exists
			meter_reading
		(
				meter_reading_id	integer primary key asc,
				meter_id			integer not null,
				read_time			integer not null,
				read_period			real not null,
				delta_count			integer not null,
				watts				integer not null,
				ampere				real not null
		);
		
		create index if not exists read_time_idx on meter_reading (read_time);
	}) or die $DBI::errstr;
}


# Straight from the perlipc doc.

sub daemonize
{
	chdir("/")                  || die "can't chdir to /: $!";
	open(STDIN,  "< /dev/null") || die "can't read /dev/null: $!";
	open(STDOUT, "> /dev/null") || die "can't write to /dev/null: $!";
	defined(my $pid = fork())   || die "can't fork: $!";
	exit if $pid;               # non-zero now means I am the parent
	(setsid() != -1)            || die "Can't start a new session: $!";
	open(STDERR, ">&STDOUT")    || die "can't dup stdout: $!";
}

