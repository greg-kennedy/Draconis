#!/usr/bin/env perl
use strict;
use warnings;

use autodie;

sub trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

print "-- Draconis character creation script --\n";
print "Enter username:\n";
my $username = <STDIN>;

# trim username, strip non-word (a-zA-Z0-9_)
$username =~ s/\W//ga;

print "Enter password:\n";
my $password = <STDIN>;

# trim password, printable ASCII only
$password =~ s/[^\x21-\x7e]//ga;

print "Avatar number (0-7):\n";
my $avatar = <STDIN>;
$avatar = trim($avatar) || 0;

#
print "You are creating a character named '$username' with password '$password', are you sure? (y/N)\n";
my $confirm = <STDIN>;

$confirm = trim($confirm);
if ($confirm eq 'y' || $confirm eq 'Y') {
  if (-e $username . '.txt') {
    print "ERROR: user '$username' already exists.\n";
    exit(1);
  } else {
    open my $fp, '>', $username . '.txt';
    # password
    print $fp $password . "\n";
    # stating area, start X, start Y
    print $fp "1\n1\n1\n";
    # HP and MP
    print $fp "20\n20\n5\n5\n";
    # avatar
    print $fp ($avatar * 8 + 1) . "\n";
    # speed, XP, gold, level
    print $fp "10\n0\n0\n1\n";
    # stats
    for (0 .. 10) {
      print $fp "10\n";
    }
    # abilities
    print $fp "1\n";
    for (1 .. 23) {
      print $fp "0\n";
    }
    # items
    for (1 .. 60) {
      print $fp "0\n";
    }
    close $fp;

    print "Success!\n";
  }
}
