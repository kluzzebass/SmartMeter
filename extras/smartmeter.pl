#!/usr/bin/perl -w

use Net::MQTT::Simple;
use Time::HiRes qw(time);

$last_count = undef;
$last_time = undef;

my $mqtt = Net::MQTT::Simple->new("autolycus.radical.org");

$mqtt->publish("topic/here" => "Message here");
$mqtt->retain( "topic/here" => "Message here");

$mqtt->run(
	"smartmeter/+/Wh" => sub {
		my ($time, $topic, $count) = (time, @_);

		unless (defined $last_count)
		{
			$last_count = $count;
			$last_time = $time;
			return;
		}

		$topic =~ /smartmeter\/(\d+)\//gio;
		$id = $1;
		my $dc = $count - $last_count;
		my $dt = $time - $last_time;
		$last_count = $count;
		$last_time = $time;
		$watt = sprintf("%.f", 3600 * $dc / $dt); 

#		print "Topic:       $topic\n";
#		print "Count:       $count\n";
#		print "Delta count: $dc\n";
#		print "Delta time:  $dt\n";
		print '#' x 20 . "\n";
		print "Watt:        $watt\n";
		print "Meter ID:    $id\n";

		$mqtt->retain("smartmeter/$id/W" => $watt);
	}
);

