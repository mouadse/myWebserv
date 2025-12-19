#!/usr/bin/perl
use strict;
use warnings;
use POSIX qw(strftime);

print "Content-Type: text/html\r\n\r\n";
print "<html><body>";
print "<h1>Hello from Perl CGI!</h1>";
print "<p>Time is: " . strftime("%Y-%m-%d %H:%M:%S", localtime) . "</p>";
print "</body></html>";
